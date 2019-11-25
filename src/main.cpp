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

static const char* optionHelp =
"Usage:  ldp <command> <options>\n"
"  e.g.  ldp load --source folio --target ldp\n"
"Commands:\n"
"  load              - Load data into the LDP database\n"
"  help              - Display help information\n"
"Options:\n"
"  --source <name>   - Extract data from source <name>, which refers to\n"
"                      the name of an object under \"sources\" in the\n"
"                      configuration file that describes connection\n"
"                      parameters for an Okapi instance\n"
"  --target <name>   - Load data into target <name>, which refers to\n"
"                      the name of an object under \"targets\" in the\n"
"                      configuration file that describes connection\n"
"                      parameters for a database\n"
"  --config <file>   - Specify the location of the configuration file,\n"
"                      overriding the LDPCONFIG environment variable\n"
"  --unsafe          - Enable functions used for testing/debugging\n"
"  --nossl           - Disable SSL in the database connection (unsafe)\n"
"  --savetemps       - Disable deletion of temporary files containing\n"
"                      extracted data (unsafe)\n"
"  --sourcedir       - Load data from a directory instead of extracting\n"
"                      from Okapi (unsafe)\n"
"  --verbose, -v     - Enable verbose output\n"
"  --debug           - Enable extremely verbose debugging output\n";

void debugNoticeProcessor(void *arg, const char *message)
{
    const Options* opt = (const Options*) arg;
    print(Print::debug, *opt,
            string("database response: ") + string(message));
}

static void initDB(const Options& opt, etymon::Postgres* db)
{
    string sql;
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
    sql = "GRANT SELECT ON ALL TABLES IN SCHEMA public TO ldp;";
    printSQL(Print::debug, opt, sql);
    { etymon::PostgresResult result(db, sql); }

    sql = "GRANT USAGE ON SCHEMA history TO ldp;";
    printSQL(Print::debug, opt, sql);
    { etymon::PostgresResult result(db, sql); }

    sql = "GRANT SELECT ON ALL TABLES IN SCHEMA history TO ldp;";
    printSQL(Print::debug, opt, sql);
    { etymon::PostgresResult result(db, sql); }

    sql = "GRANT CREATE, USAGE ON SCHEMA local TO ldp;";
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

void runLoad(const Options& opt)
{
    //string ct;
    //getCurrentTime(&ct);
    //if (opt.verbose)
    //    fprintf(opt.err, "%s: start time: %s\n", opt.prog, ct.c_str());

    // TODO Check if a time-out is used here, for example if the client
    // connection hangs due to a firewall.  Non-verbose output does not
    // communicate any problem while frozen.
    print(Print::verbose, opt, "testing database connection");
    { etymon::Postgres db(opt.databaseHost, opt.databasePort, opt.databaseUser,
            opt.databasePassword, opt.databaseName, sslmode(opt.nossl)); }

    Schema schema;
    Schema::MakeDefaultSchema(&schema);

    ExtractionFiles extractionFiles(opt);
    string loadDir;

    if (opt.loadFromDir != "") {

        if (opt.verbose)
            fprintf(opt.err, "%s: reading data from directory: %s\n",
                    opt.prog, opt.loadFromDir.c_str());
        loadDir = opt.loadFromDir;

    } else {

        extractionFiles.savetemps = opt.savetemps;

        if (opt.verbose)
            fprintf(opt.err, "%s: starting data extraction\n", opt.prog);
        Timer extractionTimer(opt);

        CURLcode cc = curl_global_init(CURL_GLOBAL_ALL);
        if (cc) {
            throw runtime_error(string("initializing curl: ") +
                    curl_easy_strerror(cc));
        }

        if (opt.verbose)
            fprintf(opt.err, "%s: logging in to Okapi service\n", opt.prog);

        string token;
        okapiLogin(opt, &token);

        makeTmpDir(opt, &loadDir);
        extractionFiles.dir = loadDir;

        extract(opt, &schema, token, loadDir, &extractionFiles);
        if (opt.verbose)
            extractionTimer.print("total data extraction time");

    }

    print(Print::verbose, opt, "connecting to database");
    etymon::Postgres db(opt.databaseHost, opt.databasePort, opt.databaseUser,
            opt.databasePassword, opt.databaseName, sslmode(opt.nossl));
    PQsetNoticeProcessor(db.conn, debugNoticeProcessor, (void*) &opt);

    string sql;
    sql = "BEGIN ISOLATION LEVEL READ COMMITTED;";
    printSQL(Print::debug, opt, sql);
    { etymon::PostgresResult result(&db, sql); }

    if (opt.verbose)
        fprintf(opt.err, "%s: initializing database\n", opt.prog);
    initDB(opt, &db);

    if (opt.verbose)
        fprintf(opt.err, "%s: starting load to database\n", opt.prog);
    Timer loadTimer(opt);

    stageAll(opt, &schema, &db, loadDir);

    mergeAll(opt, &schema, &db);

    if (opt.verbose)
        fprintf(opt.err, "%s: updating database permissions\n", opt.prog);
    updateDBPermissions(opt, &db);

    print(Print::verbose, opt, "committing changes");
    sql = "COMMIT;";
    printSQL(Print::debug, opt, sql);
    { etymon::PostgresResult result(&db, sql); }
    print(Print::verbose, opt, "all changes committed");

    //vacuumAnalyzeAll();

    if (opt.verbose) {
        fprintf(opt.err, "%s: data loading complete\n", opt.prog);
        loadTimer.print("total load time");
    }

    //getCurrentTime(&ct);
    //if (opt.verbose)
    //    fprintf(opt.err, "%s: end time: %s\n", opt.prog, ct.c_str());

    curl_global_cleanup();  // Clean-up after curl_global_init().
    // TODO Wrap curl_global_init() in a class.
}

void fillOpt(const Config& config, const string& basePointer, const string& key,
        string* result)
{
    string p = basePointer;
    p += key;
    config.getRequired(p, result);
}

void fillDirectOptions(const Config& config, const string& basePointer,
        Options* opt)
{
    int x = 0;
    string directInterfaces = basePointer + "directInterfaces/";
    while (true) {
        string interface;
        config.get(directInterfaces + to_string(x), &interface);
        if (interface == "")
            break;
        opt->direct.interfaces.push_back(interface);
        x++;
    }
    config.get(basePointer + "directDatabaseName", &(opt->direct.databaseName));
    config.get(basePointer + "directDatabaseHost", &(opt->direct.databaseHost));
    config.get(basePointer + "directDatabasePort", &(opt->direct.databasePort));
    config.get(basePointer + "directDatabaseUser", &(opt->direct.databaseUser));
    config.get(basePointer + "directDatabasePassword",
            &(opt->direct.databasePassword));
}

void fillOptions(const Config& config, Options* opt)
{
    if (opt->loadFromDir == "") {
        string source = "/sources/";
        source += opt->source;
        source += "/";
        fillOpt(config, source, "okapiURL", &(opt->okapiURL));
        fillOpt(config, source, "okapiTenant", &(opt->okapiTenant));
        fillOpt(config, source, "okapiUser", &(opt->okapiUser));
        fillOpt(config, source, "okapiPassword", &(opt->okapiPassword));
        fillOpt(config, source, "extractDir", &(opt->extractDir));
        fillDirectOptions(config, source, opt);
    }

    string target = "/targets/";
    target += opt->target;
    target += "/";
    fillOpt(config, target, "databaseName", &(opt->databaseName));
    fillOpt(config, target, "databaseType", &(opt->databaseType));
    fillOpt(config, target, "databaseHost", &(opt->databaseHost));
    fillOpt(config, target, "databasePort", &(opt->databasePort));
    fillOpt(config, target, "databaseUser", &(opt->databaseUser));
    fillOpt(config, target, "databasePassword", &(opt->databasePassword));
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

