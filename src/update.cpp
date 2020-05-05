#include <curl/curl.h>
#include <stdexcept>
#include <sys/stat.h>
#include <sys/types.h>

#include "dbcontext.h"
#include "extract.h"
#include "init.h"
#include "log.h"
#include "merge.h"
#include "stage_json.h"
#include "timer.h"
#include "update.h"

void makeTmpDir(const Options& opt, string* loaddir)
{
    *loaddir = opt.extractDir;
    string filename = "tmp_ldp_" + to_string(time(nullptr));
    etymon::join(loaddir, filename);
    //if (opt.logLevel == Level::trace)
    //    fprintf(opt.err, "%s: Creating directory: %s\n",
    //            opt.prog, loaddir->c_str());
    mkdir(loaddir->c_str(), S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP |
            S_IROTH | S_IXOTH);
}

static void updateDBPermissions(const Options& opt, Log* log,
        etymon::OdbcDbc* dbc)
{
    string sql;

    sql = "GRANT USAGE ON SCHEMA ldp_system TO " + opt.ldpUser + ";";
    log->log(Level::trace, "", "", sql, -1);
    dbc->execDirect(nullptr, sql);

    sql = "GRANT SELECT ON ALL TABLES IN SCHEMA ldp_system TO " +
        opt.ldpUser + ";";
    log->log(Level::trace, "", "", sql, -1);
    dbc->execDirect(nullptr, sql);

    sql = "GRANT SELECT ON ALL TABLES IN SCHEMA public TO " +
        opt.ldpUser + ";";
    log->log(Level::trace, "", "", sql, -1);
    dbc->execDirect(nullptr, sql);

    sql = "GRANT USAGE ON SCHEMA history TO " + opt.ldpUser + ";";
    log->log(Level::trace, "", "", sql, -1);
    dbc->execDirect(nullptr, sql);

    sql = "GRANT SELECT ON ALL TABLES IN SCHEMA history TO " +
        opt.ldpUser + ";";
    log->log(Level::trace, "", "", sql, -1);
    dbc->execDirect(nullptr, sql);

    sql = "GRANT CREATE, USAGE ON SCHEMA local TO " + opt.ldpUser + ";";
    log->log(Level::trace, "", "", sql, -1);
    dbc->execDirect(nullptr, sql);
}

void runUpdate(const Options& opt)
{
    // TODO Wrap curl_global_init() in a class.
    CURLcode cc = curl_global_init(CURL_GLOBAL_ALL);
    if (cc) {
        throw runtime_error(string("Error initializing curl: ") +
                curl_easy_strerror(cc));
    }

    etymon::OdbcEnv odbc;

    etymon::OdbcDbc logDbc(&odbc, opt.db);
    Log log(&logDbc, opt.logLevel, opt.prog);

    Schema schema;
    Schema::MakeDefaultSchema(&schema);

    {
        etymon::OdbcDbc dbc(&odbc, opt.db);
        DBType dbt(&dbc);
        DBContext db(&dbc, &dbt, &log);
        initUpgrade(&db);
    }

    ExtractionFiles extractionDir(opt);

    string loadDir;

    Curl c;
    //if (!c.curl) {
    //    // throw?
    //}
    string token, tenantHeader, tokenHeader;

    if (opt.loadFromDir != "") {
        //if (opt.logLevel == Level::trace)
        //    fprintf(opt.err, "%s: Reading data from directory: %s\n",
        //            opt.prog, opt.loadFromDir.c_str());
        loadDir = opt.loadFromDir;
    } else {
        log.log(Level::trace, "", "", "Logging in to Okapi service", -1);

        okapiLogin(opt, &log, &token);

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

        log.log(Level::trace, "", "",
                "Updating table: " + table.tableName, -1);

        Timer loadTimer(opt);

        if (opt.loadFromDir == "") {
            log.log(Level::trace, "", "",
                    "Extracting: " + table.sourcePath, -1);
            bool foundData = directOverride(opt, table.sourcePath) ?
                retrieveDirect(opt, &log, table, loadDir, &extractionFiles) :
                retrievePages(c, opt, &log, token, table, loadDir,
                        &extractionFiles);
            if (!foundData)
                table.skip = true;
        }

        if (table.skip)
            continue;

        etymon::OdbcDbc dbc(&odbc, opt.db);
        //PQsetNoticeProcessor(db.conn, debugNoticeProcessor, (void*) &opt);
        DBType dbt(&dbc);

        {
            etymon::OdbcTx tx(&dbc);

            log.log(Level::trace, "", "",
                    "Staging table: " + table.tableName, -1);
            stageTable(opt, &log, &table, &dbc, dbt, loadDir);

            log.log(Level::trace, "", "",
                    "Merging table: " + table.tableName, -1);
            mergeTable(opt, &log, table, &dbc, dbt);

            log.log(Level::trace, "", "",
                    "Replacing table: " + table.tableName, -1);
            dropTable(opt, &log, table.tableName, &dbc);
            placeTable(opt, &log, table, &dbc);
            //updateStatus(opt, table, &dbc);

            log.log(Level::trace, "", "",
                    "Updating database permissions", -1);
            updateDBPermissions(opt, &log, &dbc);

            tx.commit();
        }

        //vacuumAnalyzeTable(opt, table, &dbc);

        log.log(Level::debug, "update", table.tableName,
                "Updated table: " + table.tableName,
                loadTimer.elapsedTime());

        //if (opt.logLevel == Level::trace)
        //    loadTimer.print("load time");
    }

    {
        etymon::OdbcDbc dbc(&odbc, opt.db);
        //PQsetNoticeProcessor(db.conn, debugNoticeProcessor, (void*) &opt);

        {
            etymon::OdbcTx tx(&dbc);
            dropOldTables(opt, &log, &dbc);
            tx.commit();
        }
    }

    // TODO Check if needed for history tables; if so, move into loop above.
    //vacuumAnalyzeAll(opt, &schema, &db);

    curl_global_cleanup();  // Clean-up after curl_global_init().

    exit(0);
}

