#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <curl/curl.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <vector>

#include "../etymoncpp/include/postgres.h"
#include "config_json.h"
#include "extract.h"
#include "merge.h"
#include "options.h"
#include "stage_json.h"
#include "timer.h"
#include "util.h"

// Temporary
#include "names.h"

static const char* optionHelp =
"Usage:  ldp <command> <options>\n"
"  e.g.  ldp load --source folio --target ldp\n"
"Commands:\n"
"  load                - Load data into the LDP database\n"
"  help                - Display help information\n"
"Options:\n"
"  --source <name>     - Extract data from source <name>, which refers to\n"
"                        the name of an object under \"sources\" in the\n"
"                        configuration file that describes connection\n"
"                        parameters for an Okapi instance\n"
"  --target <name>     - Load data into target <name>, which refers to\n"
"                        the name of an object under \"targets\" in the\n"
"                        configuration file that describes connection\n"
"                        parameters for a database\n"
"  --config <path>     - Specify the location of the configuration file,\n"
"                        overriding the LDPCONFIG environment variable\n"
"  --unsafe            - Enable functions used for testing/debugging\n"
"  --nossl             - Disable SSL in the database connection (unsafe)\n"
"  --savetemps         - Disable deletion of temporary files containing\n"
"                        extracted data (unsafe)\n"
"  --sourcedir <path>  - Load data from a directory instead of extracting\n"
"                        from Okapi (unsafe)\n"
"  --verbose, -v       - Enable verbose output\n"
"  --debug             - Enable extremely verbose debugging output\n";

void debugNoticeProcessor(void *arg, const char *message)
{
    const Options* opt = (const Options*) arg;
    print(Print::debug, *opt,
            string("database response: ") + string(message));
}

static void initDB(const Options& opt, etymon::Postgres* db)
{
    string sql;

    sql = "CREATE SCHEMA IF NOT EXISTS ldp_catalog;";
    printSQL(Print::debug, opt, sql);
    { etymon::PostgresResult result(db, sql); }

    //sql =
    //    "CREATE TABLE IF NOT EXISTS ldp_catalog.table_updates (\n"
    //    "    table_name VARCHAR(65535) NOT NULL,\n"
    //    "    updated TIMESTAMPTZ NOT NULL,\n"
    //    "    tenant_id SMALLINT NOT NULL,\n"
    //    "    PRIMARY KEY (table_name, tenant_id)\n"
    //    ");";
    //printSQL(Print::debug, opt, sql);
    //{ etymon::PostgresResult result(db, sql); }

    sql = "CREATE SCHEMA IF NOT EXISTS history;";
    printSQL(Print::debug, opt, sql);
    { etymon::PostgresResult result(db, sql); }

    sql = "CREATE SCHEMA IF NOT EXISTS local;";
    printSQL(Print::debug, opt, sql);
    { etymon::PostgresResult result(db, sql); }
}

static void updateDBPermissions(const Options& opt, etymon::Postgres* db)
{
    string sql;

    sql = "GRANT USAGE ON SCHEMA ldp_catalog TO " + opt.ldpUser + ";";
    printSQL(Print::debug, opt, sql);
    { etymon::PostgresResult result(db, sql); }

    sql = "GRANT SELECT ON ALL TABLES IN SCHEMA ldp_catalog TO " +
        opt.ldpUser + ";";
    printSQL(Print::debug, opt, sql);
    { etymon::PostgresResult result(db, sql); }

    sql = "GRANT SELECT ON ALL TABLES IN SCHEMA public TO " +
        opt.ldpUser + ";";
    printSQL(Print::debug, opt, sql);
    { etymon::PostgresResult result(db, sql); }

    sql = "GRANT USAGE ON SCHEMA history TO " + opt.ldpUser + ";";
    printSQL(Print::debug, opt, sql);
    { etymon::PostgresResult result(db, sql); }

    sql = "GRANT SELECT ON ALL TABLES IN SCHEMA history TO " +
        opt.ldpUser + ";";
    printSQL(Print::debug, opt, sql);
    { etymon::PostgresResult result(db, sql); }

    sql = "GRANT CREATE, USAGE ON SCHEMA local TO " + opt.ldpUser + ";";
    printSQL(Print::debug, opt, sql);
    { etymon::PostgresResult result(db, sql); }
}

void makeTmpDir(const Options& opt, string* loaddir)
{
    *loaddir = opt.extractDir;
    string filename = "tmp_ldp_" + to_string(time(nullptr));
    etymon::join(loaddir, filename);
    print(Print::debug, opt,
            string("creating directory: ") + *loaddir);
    mkdir(loaddir->c_str(), S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP |
            S_IROTH | S_IXOTH);
}

const char* sslmode(bool nossl)
{
    return nossl ? "disable" : "require";
}

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

void beginTxn(const Options& opt, etymon::Postgres* db)
{
    string sql = "BEGIN ISOLATION LEVEL READ COMMITTED;";
    printSQL(Print::debug, opt, sql);
    { etymon::PostgresResult result(db, sql); }
}

void commitTxn(const Options& opt, etymon::Postgres* db)
{
    string sql = "COMMIT;";
    printSQL(Print::debug, opt, sql);
    { etymon::PostgresResult result(db, sql); }
}

void rollbackTxn(const Options& opt, etymon::Postgres* db)
{
    string sql = "ROLLBACK;";
    printSQL(Print::debug, opt, sql);
    { etymon::PostgresResult result(db, sql); }
}

// Check for obvious problems that could show up later in the loading
// process.
static void runPreloadTests(const Options& opt)
{
    //print(Print::verbose, opt, "running pre-load checks");

    // Check database connection.
    etymon::Postgres db(opt.databaseHost, opt.databasePort, opt.ldpAdmin,
            opt.ldpAdminPassword, opt.databaseName, sslmode(opt.nossl));
    // TODO Check if a time-out is used here, for example if the client
    // connection hangs due to a firewall.  Non-verbose output does not
    // communicate any problem while frozen.

    beginTxn(opt, &db);

    // Check that ldpUser is a valid user.
    string sql = "GRANT SELECT ON ALL TABLES IN SCHEMA public TO " +
        opt.ldpUser + ";";
    printSQL(Print::debug, opt, sql);
    { etymon::PostgresResult result(&db, sql); }

    rollbackTxn(opt, &db);
}

void runLoad(const Options& opt)
{
    string ct;
    getCurrentTime(&ct);
    if (opt.verbose)
        fprintf(opt.err, "%s: start time: %s\n", opt.prog, ct.c_str());

    runPreloadTests(opt);

    Schema schema;
    Schema::MakeDefaultSchema(&schema);

    ExtractionFiles extractionDir(opt);

    string loadDir;

    {
        print(Print::debug, opt, "connecting to database");
        etymon::Postgres db(opt.databaseHost, opt.databasePort, opt.ldpAdmin,
                opt.ldpAdminPassword, opt.databaseName, sslmode(opt.nossl));
        PQsetNoticeProcessor(db.conn, debugNoticeProcessor, (void*) &opt);

        print(Print::debug, opt, "initializing database");
        beginTxn(opt, &db);
        initDB(opt, &db);
        commitTxn(opt, &db);
    }

    Curl c;
    //if (!c.curl) {
    //    // throw?
    //}
    string token, tenantHeader, tokenHeader;

    if (opt.loadFromDir != "") {
        if (opt.verbose)
            fprintf(opt.err, "%s: reading data from directory: %s\n",
                    opt.prog, opt.loadFromDir.c_str());
        loadDir = opt.loadFromDir;
    } else {
        CURLcode cc = curl_global_init(CURL_GLOBAL_ALL);
        if (cc) {
            throw runtime_error(string("initializing curl: ") +
                    curl_easy_strerror(cc));
        }

        print(Print::debug, opt, "logging in to okapi service");

        okapiLogin(opt, &token);

        makeTmpDir(opt, &loadDir);
        extractionDir.dir = loadDir;

        tenantHeader = "X-Okapi-Tenant: ";
        tenantHeader + opt.okapiTenant;
        tokenHeader = "X-Okapi-Token: ";
        tokenHeader += token;
        c.headers = curl_slist_append(c.headers, tenantHeader.c_str());
        c.headers = curl_slist_append(c.headers, tokenHeader.c_str());
        c.headers = curl_slist_append(c.headers,
                "Accept: application/json,text/plain");
        curl_easy_setopt(c.curl, CURLOPT_HTTPHEADER, c.headers);
    }

    for (auto& table : schema.tables) {

        ExtractionFiles extractionFiles(opt);

        print(Print::verbose, opt, "loading table: " + table.tableName);

        Timer loadTimer(opt);

        if (opt.loadFromDir == "") {
            print(Print::debug, opt, "extracting: " + table.sourcePath);
            bool foundData = directOverride(opt, table.sourcePath) ?
                retrieveDirect(opt, table, loadDir, &extractionFiles) :
                retrievePages(c, opt, token, table, loadDir, &extractionFiles);
            if (!foundData)
                table.skip = true;
        }

        if (table.skip)
            continue;

        print(Print::debug, opt, "connecting to database");
        etymon::Postgres db(opt.databaseHost, opt.databasePort, opt.ldpAdmin,
                opt.ldpAdminPassword, opt.databaseName, sslmode(opt.nossl));
        PQsetNoticeProcessor(db.conn, debugNoticeProcessor, (void*) &opt);

        beginTxn(opt, &db);

        print(Print::debug, opt, "staging table: " + table.tableName);
        stageTable(opt, &table, &db, loadDir);

        print(Print::debug, opt, "merging table: " + table.tableName);
        mergeTable(opt, table, &db);

        print(Print::debug, opt, "replacing table: " + table.tableName);
        dropTable(opt, table.tableName, &db);
        placeTable(opt, table, &db);
        //updateStatus(opt, table, &db);

        if (opt.debug)
            fprintf(opt.err, "%s: updating database permissions\n", opt.prog);
        updateDBPermissions(opt, &db);

        commitTxn(opt, &db);

        vacuumAnalyzeTable(opt, table, &db);

        if (opt.verbose)
            loadTimer.print("load time");
    }

    {
        print(Print::debug, opt, "connecting to database");
        etymon::Postgres db(opt.databaseHost, opt.databasePort, opt.ldpAdmin,
                opt.ldpAdminPassword, opt.databaseName, sslmode(opt.nossl));
        PQsetNoticeProcessor(db.conn, debugNoticeProcessor, (void*) &opt);

        beginTxn(opt, &db);
        dropOldTables(opt, &db);
        commitTxn(opt, &db);
    }

    // TODO Check if needed for history tables; if so, move into loop above.
    //vacuumAnalyzeAll(opt, &schema, &db);

    getCurrentTime(&ct);
    if (opt.verbose)
        fprintf(opt.err, "%s: end time: %s\n", opt.prog, ct.c_str());

    curl_global_cleanup();  // Clean-up after curl_global_init().
    // TODO Wrap curl_global_init() in a class.
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

void checkForOldParameters(const Options& opt, const Config& config,
        const string& target)
{
    string s;
    if (!config.get(target + "ldpAdmin", &s) &&
            config.get(target + "databaseUser", &s))
        fprintf(opt.err, "\n"
                "The target configuration parameter \"databaseUser\" "
                "is no longer supported;\n"
                "it has been renamed to \"ldpAdmin\".\n"
                "Please make this change in your configuration file.\n\n");
    if (!config.get(target + "ldpAdminPassword", &s) &&
            config.get(target + "databasePassword", &s))
        fprintf(opt.err, "\n"
                "The target configuration parameter \"databasePassword\" "
                "is no longer supported;\n"
                "it has been renamed to \"ldpAdminPassword\".\n"
                "Please make this change in your configuration file.\n\n");
}

void fillOptions(const Config& config, Options* opt)
{
    if (opt->loadFromDir == "") {
        string source = "/sources/";
        source += opt->source;
        source += "/";
        config.getRequired(source + "okapiURL", &(opt->okapiURL));
        config.getRequired(source + "okapiTenant", &(opt->okapiTenant));
        config.getRequired(source + "okapiUser", &(opt->okapiUser));
        config.getRequired(source + "okapiPassword", &(opt->okapiPassword));
        config.getRequired(source + "extractDir", &(opt->extractDir));
        fillDirectOptions(config, source, opt);
    }

    string target = "/targets/";
    target += opt->target;
    target += "/";
    config.getRequired(target + "databaseName", &(opt->databaseName));
    config.getRequired(target + "databaseType", &(opt->databaseType));
    config.getRequired(target + "databaseHost", &(opt->databaseHost));
    config.getRequired(target + "databasePort", &(opt->databasePort));
    checkForOldParameters(*opt, config, target);
    config.getRequired(target + "ldpAdmin", &(opt->ldpAdmin));
    config.getRequired(target + "ldpAdminPassword", &(opt->ldpAdminPassword));
    config.get(target + "ldpUser", &(opt->ldpUser));
    opt->dbtype.setType(opt->databaseType);
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

    if (opt.debug)
        debugOptions(opt);

    if (opt.command == "load") {
        Timer t(opt);
        runLoad(opt);
        if (opt.verbose)
            t.print("total time");
        return;
    }
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
        fprintf(stderr, "ldp: error: %s\n", s.c_str());
        return 1;
    }
    return 0;
}

