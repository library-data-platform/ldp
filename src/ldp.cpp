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

#include "../etymoncpp/include/postgres.h"
#include "dbtype.h"
#include "init.h"
#include "ldp.h"
#include "log.h"
#include "schema.h"
#include "timer.h"
#include "update.h"
#include "util.h"

static const char* option_help =
"Usage:  ldp <command> <options>\n"
"  e.g.  ldp update -D /usr/local/ldp/data\n"
"Commands:\n"
"  update              - Run a full update\n"
"  init-database       - Initialize a new LDP database\n"
"  upgrade-database    - Upgrade an LDP database to the current version\n"
"  update-users        - Set permissions for configured user accounts\n"
"  list-tables         - Print a list of LDP tables in schema public\n"
"  help                - Display help information\n"
"Options:\n"
"  -D <path>           - Use <path> as the data directory\n"
"  --okapi-timeout     - Timeout in seconds for Okapi requests (default: 60)\n"
"  --trace             - Enable detailed logging\n"
"  --quiet             - Reduce console output\n"
"Development/testing options:\n"
"  --direct-extraction-no-ssl\n"
"                      - Disable SSL for direct extraction\n"
"  --extract-only      - Extract data in the data directory, but do not\n"
"                        update them in the database\n"
"  --savetemps         - Disable deletion of temporary files containing\n"
"                        extracted data\n"
"  --sourcedir <path>  - Update data from directory <path> instead of\n"
"                        from source\n";

/*
class server_lock {
public:
    server_lock(etymon::odbc_env* odbc, const string& db, log_level lg_level);
    ~server_lock();
private:
    etymon::odbc_conn* conn = nullptr;
    etymon::odbc_tx* tx = nullptr;
};

server_lock::server_lock(etymon::odbc_env* odbc, const string& db, log_level lg_level)
{
    if (lg_level == log_level::trace || lg_level == log_level::detail)
        fprintf(stderr, "ldp: acquiring server lock\n");

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
*/

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
bool time_for_full_update(const ldp_options& opt, etymon::pgconn* conn,
        dbtype* dbt, ldp_log* lg)
{
    string sql =
        "SELECT enable_full_updates,\n"
        "       (next_full_update <= " +
        string(dbt->current_timestamp()) + ") AS update_now\n"
        "    FROM dbconfig.general;";
    lg->write(log_level::detail, "", "", sql, -1);
    etymon::pgconn_result r(conn, sql);
    if (PQntuples(r.result) != 1) {
        string e = "wrong number of rows in table: dbconfig.general";
        lg->write(log_level::error, "", "", e, -1);
        throw runtime_error(e);
    }
    string full_update_enabled = PQgetvalue(r.result, 0, 0);
    string update_now = PQgetvalue(r.result, 0, 1);
    return full_update_enabled == "t" && update_now == "t";
}

/**
 * \brief Reschedule the configured full load time to the next day,
 * retaining the same time.
 *
 * param[in] opt
 * param[in] conn
 * param[in] dbt
 */
/*
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
*/

static void set_dbsystem_main_anonymize(const ldp_options& opt)
{
    etymon::pgconn conn(opt.dbinfo);
    string sql = string() +
        "UPDATE dbsystem.main\n"
        "    SET anonymize = " + (opt.anonymize ? "TRUE" : "FALSE") + ";";
    { etymon::pgconn_result r(&conn, sql); }
}

void server_loop(const ldp_options& opt, bool update_users)
{
    // Check that database version is up to date.
    validate_database_latest_version(opt);

    set_dbsystem_main_anonymize(opt);

    etymon::pgconn log_conn(opt.dbinfo);
    ldp_log lg(&log_conn, opt.lg_level, opt.console, opt.quiet);

    //lg.write(log_level::info, "server", "", string("server started"), -1);
    if (opt.direct_extraction_no_ssl) {
        lg.write(log_level::info, "server", "", "SSL disabled for direct extraction", -1);
    }

    etymon::pgconn conn(opt.dbinfo);
    dbtype dbt(&conn);

    do {
        if (opt.cli_mode || time_for_full_update(opt, &conn, &dbt, &lg) ) {
            //if (!opt.cli_mode)
            //    reschedule_next_daily_load(opt, &conn, &dbt, &lg);
            if (!opt.single_process && !update_users) {  // forked process
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
                    run_update(opt, update_users);
                } catch (runtime_error& e) {
                    string s = e.what();
                    if ( !(s.empty()) && s.back() == '\n' )
                        s.pop_back();
                    etymon::pgconn log_conn(opt.dbinfo);
                    ldp_log lg(&log_conn, opt.lg_level, opt.console, opt.quiet);
                    lg.write(log_level::error, "server", "", s, -1);
                }
            }
        }

        if (!opt.cli_mode)
            std::this_thread::sleep_for(std::chrono::seconds(60));
    } while (!opt.cli_mode);

    //lg.write(log_level::info, "server", "", string("server stopped"), -1);
}

/*
static void no_update_by_default(etymon::odbc_env* odbc, const string& db)
{
    etymon::odbc_conn conn(odbc, db);
    string sql =
        "UPDATE dbconfig.general\n"
        "    SET update_all_tables = FALSE,\n"
        "        enable_full_updates = FALSE;";
    conn.exec(sql);
}
*/

void cmd_init_database(const ldp_options& opt)
{
    if (opt.set_profile != profile::folio)
        throw runtime_error("Unknown profile");

    //server_lock svrlock(&odbc, opt.db, opt.lg_level);
    disable_termination_signals();
    init_database(opt, opt.ldp_user, opt.ldpconfig_user);
    set_dbsystem_main_anonymize(opt);

    //if (opt.no_update)
    //    no_update_by_default(&odbc, opt.db);
}

void cmd_upgrade_database(const ldp_options& opt)
{
    //server_lock svrlock(&odbc, opt.db, opt.lg_level);
    disable_termination_signals();
    upgrade_database(opt, opt.ldp_user, opt.ldpconfig_user, opt.datadir, opt.quiet);
    set_dbsystem_main_anonymize(opt);
}

void cmd_list_tables(const ldp_options& opt)
{
    ldp_schema schema;
    ldp_schema::make_default_schema(&schema);
    for (auto& table : schema.tables) {
        printf("public.%s\n", table.name.data());
    }
}

void cmd_server(const ldp_options& opt)
{
    //server_lock svrlock(&odbc, opt.db, opt.lg_level);
    if (opt.lg_level == log_level::trace || opt.lg_level == log_level::detail)
        fprintf(stderr, "ldp: starting server\n");
    server_loop(opt, false);
}

void cmd_update_users(const ldp_options& opt)
{
    //server_lock svrlock(&odbc, opt.db, opt.lg_level);
    server_loop(opt, true);
}

void config_options(const ldp_config& conf, ldp_options* opt)
{
    string deploy_env;
    conf.get_required("/deployment_environment", &deploy_env);
    config_set_environment(deploy_env, &(opt->deploy_env));

    string target = "/ldp_database/";
    conf.get_required(target + "database_name", &(opt->dbinfo.dbname));
    conf.get_required(target + "database_host", &(opt->dbinfo.dbhost));
    int port = 0;
    bool found = conf.get_int(target + "database_port", true, &port);
    if (found) {
        if (1 <= port && port <= 65535) {
            opt->dbinfo.dbport = port;
        } else {
            throw_value_out_of_range(target + "database_port",
                                     to_string(port), "1 to 65535");
        }
    } else {
        opt->dbinfo.dbport = 5432;
    }

    conf.get_int(target + "database_port", true, &(opt->dbinfo.dbport));
    conf.get_required(target + "database_user", &(opt->dbinfo.dbuser));
    conf.get_required(target + "database_password", &(opt->dbinfo.dbpasswd));
    conf.get_required(target + "database_sslmode", &(opt->dbinfo.dbsslmode));
    conf.get(target + "database_super_user", &(opt->superuser));
    conf.get(target + "database_super_password", &(opt->superpassword));
    conf.get(target + "ldpconfig_user", &(opt->ldpconfig_user));
    conf.get(target + "ldp_user", &(opt->ldp_user));

    if (opt->load_from_dir == "")
        conf.get_enable_sources(&(opt->enable_sources));

    conf.get_bool("/anonymize", &(opt->anonymize));

    conf.get_bool("/index_large_varchar", &(opt->index_large_varchar));

    conf.get_bool("/record_history", &(opt->record_history));

    conf.get_bool("/parallel_vacuum", &(opt->parallel_vacuum));

    conf.get_bool("/parallel_update", &(opt->parallel_update));

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
            timer error_timer;
            try {
                cmd_server(*opt);
            } catch (runtime_error& e) {
                string s = e.what();
                if ( !(s.empty()) && s.back() == '\n' )
                    s.pop_back();
                fprintf(stderr, "ldp: %s\n", s.c_str());
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

    if (opt->command == ldp_command::update_users) {
        cmd_update_users(*opt);
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

    if (opt.command == ldp_command::list_tables) {
        cmd_list_tables(opt);
        return;
    }

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
        fprintf(stderr, "ldp: %s\n", s.c_str());
        return 1;
    }
    return 0;
}
