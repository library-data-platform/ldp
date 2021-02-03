#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <signal.h>
#include <stdexcept>
#include <string>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <vector>

#include "../etymoncpp/include/odbc.h"
#include "../etymoncpp/include/postgres.h"
#include "dbtype.h"
#include "init.h"
#include "ldp.h"
#include "log.h"
#include "timer.h"
#include "update.h"
#include "util.h"

static const char* option_help =
"Usage:  ldp <command> <options>\n"
"  e.g.  ldp server -D /usr/local/ldp/data\n"
"Commands:\n"
"  server              - Run the LDP server\n"
"  init-database       - Initialize a new LDP database\n"
"  upgrade-database    - Upgrade an LDP database to the current version\n"
"  update              - Run a full update and exit\n"
"  help                - Display help information\n"
"Options:\n"
"  -D <path>           - Use <path> as the data directory\n"
"  --trace             - Enable detailed logging\n"
"  --quiet             - Reduce console output\n"
"Options for init-database:\n"
"  --profile <prof>    - Initialize the LDP database with profile <prof>\n"
"                        (required)\n"
/*
"  --no-update         - Set update_all_tables and enable_full_updates in\n"
"                        table dbconfig.general to FALSE, so that tables\n"
"                        will not be updated by default; this option nearly\n"
"                        always should be used for consortial deployments\n"
"                        to prevent overly broad requests for data\n"
*/
"Development/testing options:\n"
"  --extract-only      - Extract data in the data directory, but do not\n"
"                        update them in the database\n"
"  --savetemps         - Disable deletion of temporary files containing\n"
"                        extracted data\n"
"  --sourcedir <path>  - Update data from directory <path> instead of\n"
"                        from source\n";

class server_lock {
public:
    server_lock(etymon::odbc_env* odbc, const string& db, log_level lg_level,
            FILE* err, const string& program);
    ~server_lock();
private:
    etymon::odbc_conn* conn = nullptr;
    etymon::odbc_tx* tx = nullptr;
};

server_lock::server_lock(etymon::odbc_env* odbc, const string& db,
        log_level lg_level, FILE* err, const string& program)
{
    if (lg_level == log_level::trace || lg_level == log_level::detail)
        fprintf(err, "%s: Acquiring server lock\n", program.data());

    {
        etymon::odbc_conn tmp_conn(odbc, db);
        string sql = "CREATE SCHEMA IF NOT EXISTS dbinternal;";
        tmp_conn.exec(sql);
        sql = "CREATE TABLE IF NOT EXISTS dbinternal.server_lock (b BOOLEAN);";
        tmp_conn.exec(sql);
    }

    conn = new etymon::odbc_conn(odbc, db);
    try {
        tx = new etymon::odbc_tx(conn);
        try {
            string sql = "LOCK dbinternal.server_lock;";
            conn->exec(sql);
        } catch (runtime_error& e) {
            delete tx;
            throw;
        }
    } catch (runtime_error& e) {
        delete conn;
        throw;
    }
}

server_lock::~server_lock()
{
    delete tx;
    delete conn;
}

static void sigint_handler(int signum)
{
    // NOP
}

static void sigquit_handler(int signum)
{
    // NOP
}

static void sigterm_handler(int signum)
{
    // NOP
}

static void setup_signal_handler(int sig, void (*handler)(int))
{
    struct sigaction sa;
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(sig, &sa, NULL);
}

static void disable_termination_signals()
{
    setup_signal_handler(SIGINT, sigint_handler);
    setup_signal_handler(SIGQUIT, sigquit_handler);
    setup_signal_handler(SIGTERM, sigterm_handler);
}

/**
 * \brief Check configuration in the LDP database to determine if it
 * is time to run a full update.
 *
 * param[in] opt
 * param[in] conn
 * param[in] dbt
 * retval true The full update should be run as soon as possible.
 * retval false The full update should not be run at this time.
 */
bool time_for_full_update(const ldp_options& opt, etymon::odbc_conn* conn,
        dbtype* dbt, ldp_log* lg)
{
    string sql =
        "SELECT enable_full_updates,\n"
        "       (next_full_update <= " +
        string(dbt->current_timestamp()) + ") AS update_now\n"
        "    FROM dbconfig.general;";
    lg->write(log_level::detail, "", "", sql, -1);
    etymon::odbc_stmt stmt(conn);
    conn->exec_direct(&stmt, sql);
    if (conn->fetch(&stmt) == false) {
        string e = "No rows could be read from table: dbconfig.general";
        lg->write(log_level::error, "", "", e, -1);
        throw runtime_error(e);
    }
    string full_update_enabled, update_now;
    conn->get_data(&stmt, 1, &full_update_enabled);
    conn->get_data(&stmt, 2, &update_now);
    if (conn->fetch(&stmt)) {
        string e = "Too many rows in table: dbconfig.general";
        lg->write(log_level::error, "", "", e, -1);
        throw runtime_error(e);
    }
    return full_update_enabled == "1" && update_now == "1";
}

/**
 * \brief Reschedule the configured full load time to the next day,
 * retaining the same time.
 *
 * param[in] opt
 * param[in] conn
 * param[in] dbt
 */
void reschedule_next_daily_load(const ldp_options& opt, etymon::odbc_conn* conn,
                                dbtype* dbt, ldp_log* lg)
{
    string update_in_future;
    do {
        // Increment next_full_update.
        string sql =
            "UPDATE dbconfig.general SET next_full_update =\n"
            "    next_full_update + INTERVAL '1 day';";
        lg->write(log_level::detail, "", "", sql, -1);
        conn->exec(sql);
        // Check if next_full_update is now in the future.
        sql =
            "SELECT (next_full_update > " + string(dbt->current_timestamp()) +
            ") AS update_in_future\n"
            "    FROM dbconfig.general;";
        lg->write(log_level::detail, "", "", sql, -1);
        etymon::odbc_stmt stmt(conn);
        conn->exec_direct(&stmt, sql);
        if (conn->fetch(&stmt) == false) {
            string e = "No rows could be read from table: dbconfig.general";
            lg->write(log_level::error, "", "", e, -1);
            throw runtime_error(e);
        }
        conn->get_data(&stmt, 1, &update_in_future);
        if (conn->fetch(&stmt)) {
            string e = "Too many rows in table: dbconfig.general";
            lg->write(log_level::error, "", "", e, -1);
            throw runtime_error(e);
        }
    } while (update_in_future == "0");
}

static void set_dbsystem_main_anonymize(etymon::odbc_env* odbc,
                                        const ldp_options& opt)
{
    etymon::odbc_conn conn(odbc, opt.db);
    string sql = string() +
        "UPDATE dbsystem.main\n"
        "    SET anonymize = " + (opt.anonymize ? "TRUE" : "FALSE") + ";";
    conn.exec(sql);
}

void server_loop(const ldp_options& opt, etymon::odbc_env* odbc)
{
    // Check that database version is up to date.
    validate_database_latest_version(odbc, opt.db);

    set_dbsystem_main_anonymize(odbc, opt);

    etymon::odbc_conn log_conn(odbc, opt.db);
    ldp_log lg(&log_conn, opt.lg_level, opt.console, opt.quiet, opt.prog);

    lg.write(log_level::info, "server", "",
            string("Server started") + (opt.cli_mode ? " (CLI mode)" : ""), -1);

    etymon::odbc_conn conn(odbc, opt.db);
    dbtype dbt(&conn);

    do {
        if (opt.cli_mode || time_for_full_update(opt, &conn, &dbt, &lg) ) {
            if (!opt.cli_mode)
                reschedule_next_daily_load(opt, &conn, &dbt, &lg);
            if (!opt.single_process) {  // forked process
                pid_t pid = fork();
                if (pid == 0)
                    run_update_process(opt);
                if (pid > 0) {
                    int stat;
                    waitpid(pid, &stat, 0);
                    if (WIFEXITED(stat))
                        lg.write(log_level::trace, "", "",
                                 "Status code of full update: " +
                                 to_string(WEXITSTATUS(stat)), -1);
                    else
                        lg.write(log_level::trace, "", "",
                                 "Full update did not terminate normally", -1);
                }
                if (pid < 0)
                    throw runtime_error("Error starting child process");
            } else {  // single process
                try {
                    run_update(opt);
                } catch (runtime_error& e) {
                    string s = e.what();
                    if ( !(s.empty()) && s.back() == '\n' )
                        s.pop_back();
                    etymon::odbc_env odbc;
                    etymon::odbc_conn log_conn(&odbc, opt.db);
                    ldp_log lg(&log_conn, opt.lg_level, opt.console, opt.quiet, opt.prog);
                    lg.write(log_level::error, "server", "", s, -1);
                }
            }
        }

        if (!opt.cli_mode)
            std::this_thread::sleep_for(std::chrono::seconds(60));
    } while (!opt.cli_mode);

    lg.write(log_level::info, "server", "",
            string("Server stopped") + (opt.cli_mode ? " (CLI mode)" : ""), -1);
}

static void no_update_by_default(etymon::odbc_env* odbc, const string& db)
{
    etymon::odbc_conn conn(odbc, db);
    string sql =
        "UPDATE dbconfig.general\n"
        "    SET update_all_tables = FALSE,\n"
        "        enable_full_updates = FALSE;";
    conn.exec(sql);
}

void cmd_init_database(const ldp_options& opt)
{
    if (opt.set_profile == profile::none)
        throw runtime_error("Profile not specified");

    etymon::odbc_env odbc;
    server_lock svrlock(&odbc, opt.db, opt.lg_level, opt.err, opt.prog);
    disable_termination_signals();
    init_database(&odbc, opt.db, opt.ldp_user, opt.ldpconfig_user,
            opt.err, opt.prog);
    set_dbsystem_main_anonymize(&odbc, opt);

    if (opt.no_update)
        no_update_by_default(&odbc, opt.db);
}

void cmd_upgrade_database(const ldp_options& opt)
{
    etymon::odbc_env odbc;
    server_lock svrlock(&odbc, opt.db, opt.lg_level, opt.err, opt.prog);
    disable_termination_signals();
    upgrade_database(&odbc, opt.db, opt.ldp_user, opt.ldpconfig_user,
            opt.datadir,
            opt.err, opt.prog, opt.quiet);
    set_dbsystem_main_anonymize(&odbc, opt);
}

void cmd_server(const ldp_options& opt)
{
    etymon::odbc_env odbc;
    server_lock svrlock(&odbc, opt.db, opt.lg_level, opt.err, opt.prog);
    if (opt.lg_level == log_level::trace || opt.lg_level == log_level::detail)
        fprintf(opt.err, "%s: Starting server\n", opt.prog);
    server_loop(opt, &odbc);
}

//void config_direct_options(const ldp_config& conf, const string& base,
//        ldp_options* opt)
//{
//    int x = 0;
//    string direct_tables = base + "direct_tables/";
//    while (true) {
//        string t;
//        if (!conf.get(direct_tables + to_string(x), &t))
//            break;
//        opt->direct.table_names.push_back(t);
//        x++;
//    }
//    conf.get(base + "direct_database_name", &(opt->direct.database_name));
//    conf.get(base + "direct_database_host", &(opt->direct.database_host));
//    int port = 0;
//    conf.old_get_int(base + "direct_database_port", &port);
//    opt->direct.database_port = to_string(port);
//    conf.get(base + "direct_database_user", &(opt->direct.database_user));
//    conf.get(base + "direct_database_password",
//            &(opt->direct.database_password));
//}

void config_options(const ldp_config& conf, ldp_options* opt)
{
    string deploy_env;
    conf.get_required("/deployment_environment", &deploy_env);
    config_set_environment(deploy_env, &(opt->deploy_env));

    string target = "/ldp_database/";
    conf.get_required(target + "odbc_database", &(opt->db));
    conf.get(target + "ldpconfig_user", &(opt->ldpconfig_user));
    conf.get(target + "ldp_user", &(opt->ldp_user));

    ///////////////////////////////////////////////////////////////////////////
    // NEW SOURCE CONFIG
    if (opt->load_from_dir == "")
        conf.get_enable_sources(&(opt->enable_sources));
    ///////////////////////////////////////////////////////////////////////////
    // OLD SOURCE CONFIG
    //if (opt->load_from_dir == "") {
    //    string enable_source;
    //    conf.get_required("/enable_sources/0", &enable_source);
    //    string second_source;
    //    conf.get("/enable_sources/1", &second_source);
    //    if (second_source != "")
    //        throw runtime_error(
    //            "Multiple sources not currently supported in "
    //            "configuration:\n"
    //            "    Attribute: enable_sources\n"
    //            "    Value: " + second_source);
    //    string source = "/sources/" + enable_source + "/";
    //    conf.get_required(source + "okapi_url", &(opt->okapi_url));
    //    conf.get_required(source + "okapi_tenant", &(opt->okapi_tenant));
    //    conf.get_required(source + "okapi_user", &(opt->okapi_user));
    //    conf.get_required(source + "okapi_password", &(opt->okapi_password));

    //    int tenant_id = 1;
    //    conf.old_get_int(source + "tenant_id", &tenant_id);
    //    if (0 < tenant_id && tenant_id < 32768)
    //        opt->tenant_id = (int16_t) tenant_id;
    //    else
    //        throw runtime_error(
    //            "Value for configuration setting is out of range:\n"
    //            "    Attribute: tenant_id\n"
    //            "    Value: " + to_string(tenant_id) + "\n"
    //            "    Range: 0 < tenant_id < 32768");

    //    config_direct_options(conf, source, opt);
    //}
    ///////////////////////////////////////////////////////////////////////////

    bool found_disable_anonymization;
    bool b;
    found_disable_anonymization = conf.get_bool("/disable_anonymization", &b);
    if (found_disable_anonymization) {
        throw runtime_error(
            "The configuration setting \"disable_anonymization\" is not\n"
            "supported in LDP 1.1.  Please see the LDP Administrator Guide\n"
            "for information on how to disable anonymization.");
    }

    conf.get_bool("/anonymize", &(opt->anonymize));

    conf.get_bool("/allow_destructive_tests", &(opt->allow_destructive_tests));
}

void validate_options_in_deployment(const ldp_options& opt)
{
    if (opt.extract_only) {
        if (opt.deploy_env != deployment_environment::testing &&
                opt.deploy_env != deployment_environment::development)
            throw runtime_error(
                    "Extract-only option requires testing or development "
                    "environment");
    }
    if (opt.savetemps) {
        if (opt.deploy_env != deployment_environment::testing &&
                opt.deploy_env != deployment_environment::development)
            throw runtime_error(
                    "Savetemps option requires testing or development "
                    "environment");
    }
    if (opt.load_from_dir != "") {
        if (opt.deploy_env != deployment_environment::testing &&
                opt.deploy_env != deployment_environment::development)
            throw runtime_error(
                    "Loading from directory requires testing or development "
                    "environment");
        if (!opt.allow_destructive_tests)
            throw runtime_error(
                    "Loading from directory requires the "
                    "allow_destructive_tests\nconfiguration setting");
    }
}

void ldp_exec(ldp_options* opt)
{
    ldp_config conf(opt->datadir + "/ldpconf.json");
    config_options(conf, opt);

    validate_options_in_deployment(*opt);

    if (opt->command == ldp_command::update)
        opt->console = true;

    if (opt->command == ldp_command::server) {
        do {
            timer error_timer(*opt);
            try {
                cmd_server(*opt);
            } catch (runtime_error& e) {
                string s = e.what();
                if ( !(s.empty()) && s.back() == '\n' )
                    s.pop_back();
                fprintf(stderr, "ldp: Error: %s\n", s.c_str());
                double elapsed_time = error_timer.elapsed_time();
                if (elapsed_time < 300) {
                    fprintf(stderr,
                            "ldp: Server error occurred after %.4f seconds\n",
                            elapsed_time);
                    long long int wait_time = 300;
                    fprintf(stderr, "ldp: Waiting for %lld seconds\n",
                            wait_time);
                    std::this_thread::sleep_for(
                        std::chrono::seconds(wait_time) );
                }
                fprintf(stderr, "ldp: Restarting server\n");
            }
        } while (true);
        return;
    }

    if (opt->command == ldp_command::update) {
        cmd_server(*opt);
        return;
    }

    if (opt->command == ldp_command::upgrade_database) {
        cmd_upgrade_database(*opt);
        return;
    }

    if (opt->command == ldp_command::init_database) {
        cmd_init_database(*opt);
        return;
    }
}

static void setup_ldp_exec(const etymon::command_args& cargs)
{
    ldp_options opt;

    if (evalopt(cargs, &opt) < 0)
        throw runtime_error("unable to parse command line options");

    if (cargs.argc < 2 || opt.command == ldp_command::help) {
        printf("%s", option_help);
        return;
    }

    ldp_exec(&opt);
}

int main_ldp(int argc, char* const argv[])
{
    etymon::command_args cargs(argc, argv);
    try {
        setup_ldp_exec(cargs);
    } catch (runtime_error& e) {
        string s = e.what();
        if ( !(s.empty()) && s.back() == '\n' )
            s.pop_back();
        fprintf(stderr, "ldp: Error: %s\n", s.c_str());
        return 1;
    }
    return 0;
}
