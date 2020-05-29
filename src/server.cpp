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
#include "config_json.h"
#include "dbtype.h"
#include "init.h"
#include "log.h"
#include "options.h"
#include "timer.h"
#include "update.h"
#include "util.h"

// Temporary
#include "names.h"

static const char* optionHelp =
"Usage:  ldp <command> <options>\n"
"  e.g.  ldp server -D /usr/local/ldp/data\n"
"Commands:\n"
"  server              - Run the LDP server\n"
"  upgrade-database    - Upgrade the LDP database to the current version\n"
"  update              - Run a full update and exit\n"
"  help                - Display help information\n"
"Options:\n"
"  -D <path>           - Store data and configuration in directory <path>\n"
"  --trace             - Enable detailed logging\n"
"Development options:\n"
"  --unsafe            - Enable functions used for development and testing\n"
"  --extract-only      - Extract data in the data directory, but do not\n"
"                        update them in the database (unsafe)\n"
"  --savetemps         - Disable deletion of temporary files containing\n"
"                        extracted data (unsafe)\n"
"  --sourcedir <path>  - Update data from directory <path> instead of\n"
"                        from source (unsafe)\n";

void debugNoticeProcessor(void *arg, const char *message)
{
    const Options* opt = (const Options*) arg;
    print(Print::debug, *opt,
            string("database response: ") + string(message));
}

//const char* sslmode(bool nossl)
//{
//    return nossl ? "disable" : "require";
//}

static void vacuumAnalyzeTable(const Options& opt, const TableSchema& table,
        etymon::Postgres* db)
{
    string sql = "VACUUM " + table.tableName + ";\n";
    printSQL(Print::debug, opt, sql);
    { etymon::PostgresResult result(db, sql); }

    sql = "ANALYZE " + table.tableName + ";\n";
    printSQL(Print::debug, opt, sql);
    { etymon::PostgresResult result(db, sql); }
}

void vacuumAnalyzeAll(const Options& opt, Schema* schema, etymon::Postgres* db)
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
static void runPreloadTests(const Options& opt, etymon::odbc_env* odbc)
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
        conn.execDirect(nullptr, sql);
        tx.rollback();
    }
}
*/

/**
 * \brief Check configuration in the LDP database to determine if it
 * is time to run a full update.
 *
 * \param[in] opt
 * \param[in] conn
 * \param[in] dbt
 * \param[in] log
 * \retval true The full update should be run as soon as possible.
 * \retval false The full update should not be run at this time.
 */
bool timeForFullUpdate(const Options& opt, etymon::odbc_conn* conn, DBType* dbt,
        Log* log)
{
    string sql =
        "SELECT enable_full_updates,\n"
        "       (next_full_update <= " +
        string(dbt->currentTimestamp()) + ") AS update_now\n"
        "    FROM ldpconfig.general;";
    log->log(Level::detail, "", "", sql, -1);
    etymon::odbc_stmt stmt(conn);
    conn->execDirect(&stmt, sql);
    if (conn->fetch(&stmt) == false) {
        string e = "No rows could be read from table: ldpconfig.general";
        log->log(Level::error, "", "", e, -1);
        throw runtime_error(e);
    }
    string fullUpdateEnabled, updateNow;
    conn->getData(&stmt, 1, &fullUpdateEnabled);
    conn->getData(&stmt, 2, &updateNow);
    if (conn->fetch(&stmt)) {
        string e = "Too many rows in table: ldpconfig.general";
        log->log(Level::error, "", "", e, -1);
        throw runtime_error(e);
    }
    return fullUpdateEnabled == "1" && updateNow == "1";
}

/**
 * \brief Reschedule the configured full load time to the next day,
 * retaining the same time.
 *
 * \param[in] opt
 * \param[in] conn
 * \param[in] dbt
 * \param[in] log
 */
void rescheduleNextDailyLoad(const Options& opt, etymon::odbc_conn* conn,
        DBType* dbt, Log* log)
{
    string updateInFuture;
    do {
        // Increment next_full_update.
        string sql =
            "UPDATE ldpconfig.general SET next_full_update =\n"
            "    next_full_update + INTERVAL '1 day';";
        log->log(Level::detail, "", "", sql, -1);
        conn->execDirect(nullptr, sql);
        // Check if next_full_update is now in the future.
        sql =
            "SELECT (next_full_update > " + string(dbt->currentTimestamp()) +
            ") AS update_in_future\n"
            "    FROM ldpconfig.general;";
        log->log(Level::detail, "", "", sql, -1);
        etymon::odbc_stmt stmt(conn);
        conn->execDirect(&stmt, sql);
        if (conn->fetch(&stmt) == false) {
            string e = "No rows could be read from table: ldpconfig.general";
            log->log(Level::error, "", "", e, -1);
            throw runtime_error(e);
        }
        conn->getData(&stmt, 1, &updateInFuture);
        if (conn->fetch(&stmt)) {
            string e = "Too many rows in table: ldpconfig.general";
            log->log(Level::error, "", "", e, -1);
            throw runtime_error(e);
        }
    } while (updateInFuture == "0");
}

void server(const Options& opt, etymon::odbc_env* odbc)
{
    init_upgrade(odbc, opt.db, opt.ldpUser, opt.ldpconfigUser, opt.datadir,
            opt.err, opt.prog);
    if (opt.upgradeDatabase)
        return;

    etymon::odbc_conn logConn(odbc, opt.db);
    Log lg(&logConn, opt.logLevel, opt.console, opt.prog);

    lg.log(Level::info, "server", "",
            string("Server started") + (opt.cliMode ? " (CLI mode)" : ""), -1);

    etymon::odbc_conn conn(odbc, opt.db);
    DBType dbt(&conn);

    do {
        if (opt.cliMode || timeForFullUpdate(opt, &conn, &dbt, &lg) ) {
            rescheduleNextDailyLoad(opt, &conn, &dbt, &lg);
            pid_t pid = fork();
            if (pid == 0)
                runUpdateProcess(opt);
            if (pid > 0) {
                int stat;
                waitpid(pid, &stat, 0);
                if (WIFEXITED(stat))
                    lg.log(Level::trace, "", "",
                            "Status code of full update: " +
                            to_string(WEXITSTATUS(stat)), -1);
                else
                    lg.log(Level::trace, "", "",
                            "Full update did not terminate normally", -1);
            }
            if (pid < 0)
                throw runtime_error("Error starting child process");
        }

        if (!opt.cliMode)
            std::this_thread::sleep_for(std::chrono::seconds(60));
    } while (!opt.cliMode);

    lg.log(Level::info, "server", "",
            string("Server stopped") + (opt.cliMode ? " (CLI mode)" : ""), -1);
}

void runServer(const Options& opt)
{
    etymon::odbc_env odbc;

    //runPreloadTests(opt, odbc);

    etymon::odbc_conn lockConn(&odbc, opt.db);
    string sql = "CREATE SCHEMA IF NOT EXISTS ldpsystem;";
    lockConn.execDirect(nullptr, sql);
    sql = "CREATE TABLE IF NOT EXISTS ldpsystem.server_lock (b BOOLEAN);";
    lockConn.execDirect(nullptr, sql);

    {
        if (opt.logLevel == Level::trace || opt.logLevel == Level::detail)
            fprintf(opt.err, "%s: Acquiring server lock\n", opt.prog);

        etymon::odbc_tx tx(&lockConn);
        sql = "LOCK ldpsystem.server_lock;";
        lockConn.execDirect(nullptr, sql);

        if (opt.logLevel == Level::trace || opt.logLevel == Level::detail)
            fprintf(opt.err, "%s: Initializing\n", opt.prog);

        server(opt, &odbc);
    }
}

void fillDirectOptions(const Config& config, const string& base, Options* opt)
{
    int x = 0;
    string directTables = base + "directTables/";
    while (true) {
        string t;
        if (!config.get(directTables + to_string(x), &t))
            break;
        opt->direct.tableNames.push_back(t);
        x++;
    }
    config.get(base + "directDatabaseName", &(opt->direct.databaseName));
    config.get(base + "directDatabaseHost", &(opt->direct.databaseHost));
    int port = 0;
    config.getInt(base + "directDatabasePort", &port);
    opt->direct.databasePort = to_string(port);
    config.get(base + "directDatabaseUser", &(opt->direct.databaseUser));
    config.get(base + "directDatabasePassword",
            &(opt->direct.databasePassword));
}

//void checkForOldParameters(const Options& opt, const Config& config,
//        const string& target)
//{
//    string s;
//    if (!config.get(target + "ldpAdmin", &s) &&
//            config.get(target + "databaseUser", &s))
//        fprintf(opt.err, "\n"
//                "The target configuration parameter \"databaseUser\" "
//                "is no longer supported;\n"
//                "it has been renamed to \"ldpAdmin\".\n"
//                "Please make this change in your configuration file.\n\n");
//    if (!config.get(target + "ldpAdminPassword", &s) &&
//            config.get(target + "databasePassword", &s))
//        fprintf(opt.err, "\n"
//                "The target configuration parameter \"databasePassword\" "
//                "is no longer supported;\n"
//                "it has been renamed to \"ldpAdminPassword\".\n"
//                "Please make this change in your configuration file.\n\n");
//}

void fillOptions(const Config& config, Options* opt)
{
    string target = "/ldpDatabase/";
    config.getRequired(target + "odbcDatabase", &(opt->db));
    config.get(target + "ldpconfigUser", &(opt->ldpconfigUser));
    config.get(target + "ldpUser", &(opt->ldpUser));

    if (opt->loadFromDir == "") {
        string enableSource;
        config.getRequired("/enableSources/0", &enableSource);
        string secondSource;
        config.get("/enableSources/1", &secondSource);
        if (secondSource != "")
            throw runtime_error(
                    "Multiple sources not currently supported in "
                    "configuration:\n"
                    "    Attribute: enableSources\n"
                    "    Value: " + secondSource);
        string source = "/sources/" + enableSource + "/";
        config.getRequired(source + "okapiURL", &(opt->okapiURL));
        config.getRequired(source + "okapiTenant", &(opt->okapiTenant));
        config.getRequired(source + "okapiUser", &(opt->okapiUser));
        config.getRequired(source + "okapiPassword", &(opt->okapiPassword));
        //config.getRequired(source + "extractDir", &(opt->extractDir));
        fillDirectOptions(config, source, opt);
    }

    //string disableAnonymization;
    bool disableAnonymization;
    config.getBool("/disableAnonymization", &disableAnonymization);
    //if (disableAnonymization != "") {
    //    etymon::toLower(&disableAnonymization);
    //    opt->disableAnonymization = (disableAnonymization == "true");
    //}
    opt->disableAnonymization = disableAnonymization;
}

void run(const etymon::CommandArgs& cargs)
{
    Options opt;

    if (evalopt(cargs, &opt) < 0)
        throw runtime_error("unable to parse command line options");

    if (cargs.argc < 2 || opt.command == "help") {
        printf("%s", optionHelp);
        return;
    }

    Config config(opt.datadir + "/ldpconf.json");
    fillOptions(config, &opt);
    if (opt.command == "update")
        opt.console = true;

    if (opt.command == "server") {
        do {
            Timer timer(opt);
            try {
                runServer(opt);
            } catch (runtime_error& e) {
                string s = e.what();
                if ( !(s.empty()) && s.back() == '\n' )
                    s.pop_back();
                fprintf(stderr, "ldp: Error: %s\n", s.c_str());
                double elapsedTime = timer.elapsedTime();
                if (elapsedTime < 60) {
                    fprintf(stderr,
                            "ldp: Server error occurred after %.4f seconds\n",
                            elapsedTime);
                    long long int waitTime = 60;
                    fprintf(stderr, "ldp: Waiting for %lld seconds\n",
                            waitTime);
                    std::this_thread::sleep_for(std::chrono::seconds(waitTime));
                }
                fprintf(stderr, "ldp: Restarting server\n");
            }
        } while (true);
        return;
    }

    if (opt.command == "update") {
        runServer(opt);
        return;
    }

    if (opt.command == "upgrade-database") {
        runServer(opt);
        return;
    }
}

static void sigint_handler(int signum)
{
    // NOP
}

static void setup_signal_handlers()
{
    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa, NULL);
}

int cli(int argc, char* argv[])
{
    setup_signal_handlers();
    etymon::CommandArgs cargs(argc, argv);
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

