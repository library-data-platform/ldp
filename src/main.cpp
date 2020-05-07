#include <chrono>
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
#include "config_json.h"
#include "dbcontext.h"
#include "dbtype.h"
#include "init.h"
#include "log.h"
#include "options.h"
#include "update.h"
#include "util.h"

// Temporary
#include "names.h"

static const char* optionHelp =
"Usage:  ldp <command> <options>\n"
"  e.g.  ldp update --config ldpconf.json --source folio\n"
//"  e.g.  ldp server --config ldpconfig.json\n"
"Commands:\n"
//"  server              - Run LDP server\n"
"  update              - Load data into the LDP database\n"
"  help                - Display help information\n"
"Options:\n"
"  --config <path>     - Specify the path to the configuration file\n"
"  --source <name>     - Extract data from source <name>, which refers to\n"
"                        the name of an object under \"sources\" in the\n"
"                        configuration file that describes connection\n"
"                        parameters for an Okapi instance\n"
"  --unsafe            - Enable functions used for testing/debugging\n"
//"  --nossl             - Disable SSL in the database connection (unsafe)\n"
"  --savetemps         - Disable deletion of temporary files containing\n"
"                        extracted data (unsafe)\n"
"  --sourcedir <path>  - Load data from a directory instead of extracting\n"
"                        from Okapi (unsafe)\n"
"  --trace             - Enable very detailed logging\n";

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
static void runPreloadTests(const Options& opt, etymon::OdbcEnv* odbc)
{
    //print(Print::verbose, opt, "running pre-load checks");

    // Check database connection.
    etymon::OdbcDbc dbc(odbc, opt.db);
    // TODO Check if a time-out is used here, for example if the client
    // connection hangs due to a firewall.  Non-verbose output does not
    // communicate any problem while frozen.

    {
        etymon::OdbcTx tx(&dbc);
        // Check that ldpUser is a valid user.
        string sql = "GRANT SELECT ON ALL TABLES IN SCHEMA public TO " +
            opt.ldpUser + ";";
        printSQL(Print::debug, opt, sql);
        dbc.execDirect(nullptr, sql);
        tx.rollback();
    }
}
*/

/**
 * \brief Check configuration in the LDP database to determine if it
 * is time to run a full update.
 *
 * \param[in] opt
 * \param[in] dbc
 * \param[in] dbt
 * \param[in] log
 * \retval true The full update should be run as soon as possible.
 * \retval false The full update should not be run at this time.
 */
bool timeForFullUpdate(const Options& opt, etymon::OdbcDbc* dbc, DBType* dbt,
        Log* log)
{
    string sql =
        "SELECT full_update_enabled,\n"
        "       (next_full_update <= " +
        string(dbt->currentTimestamp()) + ") AS update_now\n"
        "    FROM ldpconfig.general;";
    log->log(Level::detail, "", "", sql, -1);
    etymon::OdbcStmt stmt(dbc);
    dbc->execDirect(&stmt, sql);
    if (dbc->fetch(&stmt) == false) {
        string e = "No rows could be read from table: ldpconfig.general";
        log->log(Level::error, "", "", e, -1);
        throw runtime_error(e);
    }
    string fullUpdateEnabled, updateNow;
    dbc->getData(&stmt, 1, &fullUpdateEnabled);
    dbc->getData(&stmt, 2, &updateNow);
    if (dbc->fetch(&stmt)) {
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
 * \param[in] dbc
 * \param[in] dbt
 * \param[in] log
 */
void rescheduleNextDailyLoad(const Options& opt, etymon::OdbcDbc* dbc,
        DBType* dbt, Log* log)
{
    string updateInFuture;
    do {
        // Increment next_full_update.
        string sql =
            "UPDATE ldpconfig.general SET next_full_update =\n"
            "    next_full_update + INTERVAL '1 day';";
        log->log(Level::detail, "", "", sql, -1);
        dbc->execDirect(nullptr, sql);
        // Check if next_full_update is now in the future.
        sql =
            "SELECT (next_full_update > " + string(dbt->currentTimestamp()) +
            ") AS update_in_future\n"
            "    FROM ldpconfig.general;";
        log->log(Level::detail, "", "", sql, -1);
        etymon::OdbcStmt stmt(dbc);
        dbc->execDirect(&stmt, sql);
        if (dbc->fetch(&stmt) == false) {
            string e = "No rows could be read from table: ldpconfig.general";
            log->log(Level::error, "", "", e, -1);
            throw runtime_error(e);
        }
        dbc->getData(&stmt, 1, &updateInFuture);
        if (dbc->fetch(&stmt)) {
            string e = "Too many rows in table: ldpconfig.general";
            log->log(Level::error, "", "", e, -1);
            throw runtime_error(e);
        }
    } while (updateInFuture == "0");
}

void runServer(const Options& opt)
{
    etymon::OdbcEnv odbc;

    //runPreloadTests(opt, &odbc);

    etymon::OdbcDbc logDbc(&odbc, opt.db);
    Log log(&logDbc, opt.logLevel, opt.prog);

    {
        etymon::OdbcDbc dbc(&odbc, opt.db);
        DBType dbt(&dbc);
        DBContext db(&dbc, &dbt, &log);
        initUpgrade(&odbc, opt.db, &db, opt.ldpUser);
    }

    log.log(Level::info, "server", "",
            string("Server started") + (opt.cliMode ? " (CLI mode)" : ""), -1);

    etymon::OdbcDbc dbc(&odbc, opt.db);
    DBType dbt(&dbc);

    do {
        if (opt.cliMode || timeForFullUpdate(opt, &dbc, &dbt, &log) ) {
            rescheduleNextDailyLoad(opt, &dbc, &dbt, &log);
            log.log(Level::trace, "", "", "Starting full update", -1);
            pid_t pid = fork();
            if (pid == 0)
                runUpdateProcess(opt);
            if (pid > 0) {
                int stat;
                waitpid(pid, &stat, 0);
                if (WIFEXITED(stat))
                    log.log(Level::trace, "", "",
                            "Status code of full update: " +
                            to_string(WEXITSTATUS(stat)), -1);
                else
                    log.log(Level::trace, "", "",
                            "Full update did not terminate normally", -1);
            }
            if (pid < 0)
                throw runtime_error("Error starting child process");
        }

        if (!opt.cliMode)
            std::this_thread::sleep_for(std::chrono::seconds(60));
    } while (!opt.cliMode);

    log.log(Level::info, "server", "",
            string("Server stopped") + (opt.cliMode ? " (CLI mode)" : ""), -1);

    if (opt.cliMode)
        fprintf(opt.err, "%s: Update completed\n", opt.prog);
}

void fillDirectOptions(const Config& config, const string& base, Options* opt)
{
    int x = 0;
    string directInterfaces = base + "directInterfaces/";
    while (true) {
        string interface;
        if (!config.get(directInterfaces + to_string(x), &interface))
            break;
        opt->direct.interfaces.push_back(interface);
        x++;
    }
    config.get(base + "directDatabaseName", &(opt->direct.databaseName));
    config.get(base + "directDatabaseHost", &(opt->direct.databaseHost));
    config.get(base + "directDatabasePort", &(opt->direct.databasePort));
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
    if (opt->loadFromDir == "") {
        string source = "/dataSources/";
        source += opt->source;
        source += "/";
        config.getRequired(source + "okapiURL", &(opt->okapiURL));
        config.getRequired(source + "okapiTenant", &(opt->okapiTenant));
        config.getRequired(source + "okapiUser", &(opt->okapiUser));
        config.getRequired(source + "okapiPassword", &(opt->okapiPassword));
        config.getRequired(source + "extractDir", &(opt->extractDir));
        fillDirectOptions(config, source, opt);
    }

    string target = "/ldpDatabase/";
    config.getRequired(target + "odbcDataSourceName",
            &(opt->db));
    config.get(target + "ldpUser", &(opt->ldpUser));
    //config.getRequired(target + "databaseType", &(opt->databaseType));
    //config.getRequired(target + "databaseHost", &(opt->databaseHost));
    //config.getRequired(target + "databasePort", &(opt->databasePort));
    //checkForOldParameters(*opt, config, target);
    //config.getRequired(target + "ldpAdmin", &(opt->ldpAdmin));
    //config.getRequired(target + "ldpAdminPassword", &(opt->ldpAdminPassword));
    //config.get(target + "ldpUser", &(opt->ldpUser));
    //opt->dbtype.setType(opt->databaseType);
    //opt->dbtype.setType("PostgreSQL"); ////////////////// Set DBType
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

    Config config(opt.config);
    fillOptions(config, &opt);

    //if (opt.logLevel == Level::trace)
    //    debugOptions(opt);

    if (opt.command == "server" || opt.command == "update") {
        runServer(opt);
        return;
    }

    //if (opt.command == "update") {
    //    Timer t(opt);
    //    runLoad(opt);
    //    if (opt.logLevel == Level::trace)
    //        t.print("total time");
    //    return;
    //}
}

int main(int argc, char* argv[])
{
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

