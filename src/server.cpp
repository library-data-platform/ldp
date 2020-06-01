#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <vector>

#include "../etymoncpp/include/odbc.h"
#include "../etymoncpp/include/postgres.h"
#include "config.h"
#include "dbtype.h"
#include "init.h"
#include "log.h"
#include "timer.h"
#include "update.h"
#include "util.h"

// Temporary
#include "names.h"

static const char* option_help =
"Usage:  ldp <command> <options>\n"
"  e.g.  ldp init -D /usr/local/ldp/data --profile folio\n"
"Commands:\n"
"  init                - Initialize a new LDP database\n"
"  server              - Run the LDP server\n"
"  upgrade-database    - Upgrade the LDP database to the current version\n"
"  update              - Run a full update and exit\n"
"  help                - Display help information\n"
"Options:\n"
"  -D <path>           - Use <path> as the data directory\n"
"  --trace             - Enable detailed logging\n"
"  --quiet             - Reduce console output\n"
"Initialization (init) options\n"
"  --profile <prof>    - Initialize the LDP database with profile <prof>\n"
"                        (required)\n"
"Development options (supported only for \"development\" environments):\n"
//"  --unsafe            - Enable functions used for development and testing\n"
"  --extract-only      - Extract data in the data directory, but do not\n"
"                        update them in the database\n"
"  --savetemps         - Disable deletion of temporary files containing\n"
"                        extracted data\n"
"  --sourcedir <path>  - Update data from directory <path> instead of\n"
"                        from source\n";

void debugNoticeProcessor(void *arg, const char *message)
{
    const options* opt = (const options*) arg;
    print(Print::debug, *opt,
            string("database response: ") + string(message));
}

//const char* sslmode(bool nossl)
//{
//    return nossl ? "disable" : "require";
//}

static void vacuumAnalyzeTable(const options& opt, const TableSchema& table,
        etymon::Postgres* db)
{
    string sql = "VACUUM " + table.tableName + ";\n";
    printSQL(Print::debug, opt, sql);
    { etymon::PostgresResult result(db, sql); }

    sql = "ANALYZE " + table.tableName + ";\n";
    printSQL(Print::debug, opt, sql);
    { etymon::PostgresResult result(db, sql); }
}

void vacuumAnalyzeAll(const options& opt, Schema* schema, etymon::Postgres* db)
{
    print(Print::verbose, opt, "vacuum/analyze");
    for (auto& table : schema->tables) {
        if (table.skip)
            continue;
        vacuumAnalyzeTable(opt, table, db);
    }
}

// Check for obvious problems that could show up later in the loading
// process.
/*
static void run_preload_tests(const options& opt, etymon::odbc_env* odbc)
{
    //print(Print::verbose, opt, "running pre-load checks");

    // Check database connection.
    etymon::odbc_conn conn(odbc, opt.db);
    // TODO Check if a time-out is used here, for example if the client
    // connection hangs due to a firewall.  Non-verbose output does not
    // communicate any problem while frozen.

    {
        etymon::odbc_tx tx(&conn);
        // Check that ldpUser is a valid user.
        string sql = "GRANT SELECT ON ALL TABLES IN SCHEMA public TO " +
            opt.ldpUser + ";";
        printSQL(Print::debug, opt, sql);
        conn.exec(sql);
        tx.rollback();
    }
}
*/

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
bool time_for_full_update(const options& opt, etymon::odbc_conn* conn,
        dbtype* dbt, log* lg)
{
    string sql =
        "SELECT enable_full_updates,\n"
        "       (next_full_update <= " +
        string(dbt->current_timestamp()) + ") AS update_now\n"
        "    FROM ldpconfig.general;";
    lg->write(level::detail, "", "", sql, -1);
    etymon::odbc_stmt stmt(conn);
    conn->exec_direct(&stmt, sql);
    if (conn->fetch(&stmt) == false) {
        string e = "No rows could be read from table: ldpconfig.general";
        lg->write(level::error, "", "", e, -1);
        throw runtime_error(e);
    }
    string fullUpdateEnabled, updateNow;
    conn->get_data(&stmt, 1, &fullUpdateEnabled);
    conn->get_data(&stmt, 2, &updateNow);
    if (conn->fetch(&stmt)) {
        string e = "Too many rows in table: ldpconfig.general";
        lg->write(level::error, "", "", e, -1);
        throw runtime_error(e);
    }
    return fullUpdateEnabled == "1" && updateNow == "1";
}

/**
 * \brief Reschedule the configured full load time to the next day,
 * retaining the same time.
 *
 * param[in] opt
 * param[in] conn
 * param[in] dbt
 */
void reschedule_next_daily_load(const options& opt, etymon::odbc_conn* conn,
        dbtype* dbt, log* lg)
{
    string updateInFuture;
    do {
        // Increment next_full_update.
        string sql =
            "UPDATE ldpconfig.general SET next_full_update =\n"
            "    next_full_update + INTERVAL '1 day';";
        lg->write(level::detail, "", "", sql, -1);
        conn->exec(sql);
        // Check if next_full_update is now in the future.
        sql =
            "SELECT (next_full_update > " + string(dbt->current_timestamp()) +
            ") AS update_in_future\n"
            "    FROM ldpconfig.general;";
        lg->write(level::detail, "", "", sql, -1);
        etymon::odbc_stmt stmt(conn);
        conn->exec_direct(&stmt, sql);
        if (conn->fetch(&stmt) == false) {
            string e = "No rows could be read from table: ldpconfig.general";
            lg->write(level::error, "", "", e, -1);
            throw runtime_error(e);
        }
        conn->get_data(&stmt, 1, &updateInFuture);
        if (conn->fetch(&stmt)) {
            string e = "Too many rows in table: ldpconfig.general";
            lg->write(level::error, "", "", e, -1);
            throw runtime_error(e);
        }
    } while (updateInFuture == "0");
}

void server(const options& opt, etymon::odbc_env* odbc)
{
    init_upgrade(odbc, opt.db, opt.ldp_user, opt.ldpconfig_user, opt.datadir,
            opt.err, opt.prog, opt.quiet,
            (opt.command == ldp_command::upgrade_database));
    if (opt.command == ldp_command::upgrade_database)
        return;
    if (opt.command == ldp_command::init)
        return;

    etymon::odbc_conn log_conn(odbc, opt.db);
    log lg(&log_conn, opt.log_level, opt.console, opt.quiet, opt.prog);

    lg.write(level::info, "server", "",
            string("Server started") + (opt.cli_mode ? " (CLI mode)" : ""), -1);

    etymon::odbc_conn conn(odbc, opt.db);
    dbtype dbt(&conn);

    do {
        if (opt.cli_mode || time_for_full_update(opt, &conn, &dbt, &lg) ) {
            reschedule_next_daily_load(opt, &conn, &dbt, &lg);
            pid_t pid = fork();
            if (pid == 0)
                run_update_process(opt);
            if (pid > 0) {
                int stat;
                waitpid(pid, &stat, 0);
                if (WIFEXITED(stat))
                    lg.write(level::trace, "", "",
                            "Status code of full update: " +
                            to_string(WEXITSTATUS(stat)), -1);
                else
                    lg.write(level::trace, "", "",
                            "Full update did not terminate normally", -1);
            }
            if (pid < 0)
                throw runtime_error("Error starting child process");
        }

        if (!opt.cli_mode)
            std::this_thread::sleep_for(std::chrono::seconds(60));
    } while (!opt.cli_mode);

    lg.write(level::info, "server", "",
            string("Server stopped") + (opt.cli_mode ? " (CLI mode)" : ""), -1);
}

void run_server(const options& opt)
{
    etymon::odbc_env odbc;

    //run_preload_tests(opt, odbc);

    etymon::odbc_conn lock_conn(&odbc, opt.db);
    string sql = "CREATE SCHEMA IF NOT EXISTS ldpsystem;";
    lock_conn.exec(sql);
    sql = "CREATE TABLE IF NOT EXISTS ldpsystem.server_lock (b BOOLEAN);";
    lock_conn.exec(sql);

    {
        if (opt.log_level == level::trace || opt.log_level == level::detail)
            fprintf(opt.err, "%s: Acquiring server lock\n", opt.prog);

        etymon::odbc_tx tx(&lock_conn);
        sql = "LOCK ldpsystem.server_lock;";
        lock_conn.exec(sql);

        if (opt.log_level == level::trace || opt.log_level == level::detail)
            fprintf(opt.err, "%s: Initializing\n", opt.prog);

        server(opt, &odbc);
    }
}

void fill_direct_options(const config& conf, const string& base, options* opt)
{
    int x = 0;
    string directTables = base + "direct_tables/";
    while (true) {
        string t;
        if (!conf.get(directTables + to_string(x), &t))
            break;
        opt->direct.table_names.push_back(t);
        x++;
    }
    conf.get(base + "direct_database_name", &(opt->direct.database_name));
    conf.get(base + "direct_database_host", &(opt->direct.database_host));
    int port = 0;
    conf.get_int(base + "direct_database_port", &port);
    opt->direct.database_port = to_string(port);
    conf.get(base + "direct_database_user", &(opt->direct.database_user));
    conf.get(base + "direct_database_password",
            &(opt->direct.database_password));
}

void fill_options(const config& conf, options* opt)
{
    string environment;
    ///////////////////////////////////////////////////////////////////////////
    //conf.get_required("/environment", &environment);
    ///////////////////////////////////////////////////////////////////////////
    conf.get("/environment", &environment);
    if (environment == "") {
        fprintf(stderr, "ldp: ");
        print_banner_line(stderr, '=', 74);
        fprintf(stderr,
                "ldp: Warning: Deployment environment should be set in "
                "ldpconf.json\n"
                "ldp: Defaulting to: production\n");
        fprintf(stderr, "ldp: ");
        print_banner_line(stderr, '=', 74);
        environment = "production";
    }
    ///////////////////////////////////////////////////////////////////////////
    config_set_environment(environment, &(opt->environment));

    string target = "/ldp_database/";
    conf.get_required(target + "odbc_database", &(opt->db));
    conf.get(target + "ldpconfig_user", &(opt->ldpconfig_user));
    conf.get(target + "ldp_user", &(opt->ldp_user));

    if (opt->load_from_dir == "") {
        string enableSource;
        conf.get_required("/enable_sources/0", &enableSource);
        string secondSource;
        conf.get("/enable_sources/1", &secondSource);
        if (secondSource != "")
            throw runtime_error(
                    "Multiple sources not currently supported in "
                    "configuration:\n"
                    "    Attribute: enable_sources\n"
                    "    Value: " + secondSource);
        string source = "/sources/" + enableSource + "/";
        conf.get_required(source + "okapi_url", &(opt->okapi_url));
        conf.get_required(source + "okapi_tenant", &(opt->okapi_tenant));
        conf.get_required(source + "okapi_user", &(opt->okapi_user));
        conf.get_required(source + "okapi_password", &(opt->okapi_password));
        fill_direct_options(conf, source, opt);
    }

    bool disable_anonymization;
    conf.get_bool("/disable_anonymization", &disable_anonymization);
    opt->disable_anonymization = disable_anonymization;
}

void run_opt(options* opt)
{
    config conf(opt->datadir + "/ldpconf.json");
    fill_options(conf, opt);
    if (opt->command == ldp_command::update)
        opt->console = true;

    if (opt->command == ldp_command::server) {
        do {
            timer error_timer(*opt);
            try {
                run_server(*opt);
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
                    long long int waitTime = 300;
                    fprintf(stderr, "ldp: Waiting for %lld seconds\n",
                            waitTime);
                    std::this_thread::sleep_for(std::chrono::seconds(waitTime));
                }
                fprintf(stderr, "ldp: Restarting server\n");
            }
        } while (true);
        return;
    }

    if (opt->command == ldp_command::upgrade_database) {
        run_server(*opt);
        return;
    }

    if (opt->command == ldp_command::init) {
        if (opt->set_profile == profile::none)
            throw runtime_error("Profile not specified");
        run_server(*opt);
        return;
    }

    if (opt->command == ldp_command::update) {
        run_server(*opt);
        return;
    }
}

void run(const etymon::command_args& cargs)
{
    options opt;

    if (evalopt(cargs, &opt) < 0)
        throw runtime_error("unable to parse command line options");

    if (cargs.argc < 2 || opt.command == ldp_command::help) {
        printf("%s", option_help);
        return;
    }

    run_opt(&opt);
}

int main_cli(int argc, char* const argv[])
{
    etymon::command_args cargs(argc, argv);
    try {
        run(cargs);
    } catch (runtime_error& e) {
        string s = e.what();
        if ( !(s.empty()) && s.back() == '\n' )
            s.pop_back();
        fprintf(stderr, "ldp: Error: %s\n", s.c_str());
        return 1;
    }
    return 0;
}

