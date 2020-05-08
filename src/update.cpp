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

bool isForeignKey(etymon::OdbcDbc* dbc, Log* log, const TableSchema& table2,
        const ColumnSchema& column2, const TableSchema& table1)
{
    string sql =
        "SELECT 1\n"
        "    FROM " + table2.tableName + " AS r2\n"
        "        JOIN " + table1.tableName + " AS r1\n"
        "            ON r2." + column2.columnName + "_sk = r1.sk\n"
        "    LIMIT 1;";
    log->logDetail(sql);
    etymon::OdbcStmt stmt(dbc);
    try {
        dbc->execDirect(&stmt, sql);
    } catch (runtime_error& e) {
        return false;
    }
    return dbc->fetch(&stmt);
}

void forceConstrainReferencedTable(/*etymon::OdbcEnv* odbc, const string& dbName,*/
        etymon::OdbcDbc* dbc, Log* log, const TableSchema& table2,
        const ColumnSchema& column2, const TableSchema& table1,
        bool deleteRecords)
{
    string sql =
        "SELECT " + column2.columnName + "_sk AS fkey_sk,\n"
        "       " + column2.columnName + " AS fkey_id\n"
        "    FROM " + table2.tableName + "\n"
        "    WHERE " + column2.columnName + "_sk NOT IN (\n"
        "        SELECT sk FROM " + table1.tableName + "\n"
        "    );";
    log->logDetail(sql);
    // Assume the tables exist.
    //etymon::OdbcDbc deleteDBC(odbc, dbName);
    vector<string> fkeys;
    {
        etymon::OdbcStmt stmt(dbc);
        dbc->execDirect(&stmt, sql);
        while (dbc->fetch(&stmt)) {
            string fkeySK, fkeyID;
            dbc->getData(&stmt, 1, &fkeySK);
            dbc->getData(&stmt, 2, &fkeyID);
            if (deleteRecords)
                fkeys.push_back(fkeySK);
            log->log(Level::debug, "constraint", table2.tableName,
                    "Nonexistent key in referential path:\n"
                    "    Referencing table: " + table2.tableName + "\n"
                    "    Referencing column: " + column2.columnName + "\n"
                    "    Referencing column (sk): " + fkeySK + "\n"
                    "    Referencing column (id): " + fkeyID + "\n"
                    "    Referenced table: " + table1.tableName + "\n"
                    "    Action: " +
                    (deleteRecords ? "Deleted (cascading)" : "Ignored"),
                    -1);
        }
    }
    if (deleteRecords) {
        for (auto& fkey : fkeys) {
            sql =
                "DELETE FROM \n"
                "    " + table2.tableName + "\n"
                "    WHERE " + column2.columnName + "_sk = '" + fkey + "';";
            log->logDetail(sql);
            dbc->execDirect(nullptr, sql);
        }
        // TODO Do the same for _id -> id
        sql =
            "ALTER TABLE\n"
            "    " + table2.tableName + "\n"
            "    ADD CONSTRAINT\n"
            "        " + table2.tableName + "_" + column2.columnName +
            "_sk_fkey\n"
            "        FOREIGN KEY (" + column2.columnName + "_sk)\n"
            "        REFERENCES\n"
            "        " + table1.tableName + "\n"
            "        (sk) ON DELETE CASCADE;";
        log->logDetail(sql);
        dbc->execDirect(nullptr, sql);
    }
}

void analyzeForeignKeys(etymon::OdbcEnv* odbc, const string& dbName,
        etymon::OdbcDbc* dbc, Log* log, const Schema& schema,
        const TableSchema& table)
{
    etymon::OdbcDbc queryDBC(odbc, dbName);
    log->logDetail("Searching for foreign keys in table: " + table.tableName);
    //printf("Table: %s\n", table.tableName.c_str());
    for (auto& column : table.columns) {
        if (column.columnType != ColumnType::id)
            continue;
        if (column.columnName == "id")
            continue;
        //printf("Column: %s\n", column.columnName.c_str());
        for (auto& table1 : schema.tables) {
            if (isForeignKey(&queryDBC, log, table, column, table1)) {
                //printf("-> %s\n", table1.tableName.c_str());
                forceConstrainReferencedTable(/*odbc, dbName,*/ dbc, log, table,
                        column, table1, false);
                break;
            }
        }
    }
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
        initUpgrade(&odbc, opt.db, &db, opt.ldpUser);
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

    {
        etymon::OdbcDbc dbc(&odbc, opt.db);
        //PQsetNoticeProcessor(db.conn, debugNoticeProcessor, (void*) &opt);
        DBType dbt(&dbc);

        //etymon::OdbcTx tx(&dbc);

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

            log.log(Level::trace, "", "",
                    "Staging table: " + table.tableName, -1);
            stageTable(opt, &log, &table, &odbc, &dbc, dbt, loadDir);

            log.log(Level::trace, "", "",
                    "Merging table: " + table.tableName, -1);
            mergeTable(opt, &log, table, &odbc, &dbc, dbt);

            log.log(Level::trace, "", "",
                    "Replacing table: " + table.tableName, -1);
            dropTable(opt, &log, table.tableName, &dbc);
            placeTable(opt, &log, table, &dbc);
            //updateStatus(opt, table, &dbc);

            log.log(Level::trace, "", "",
                    "Updating database permissions", -1);
            //updateDBPermissions(opt, &log, &dbc);

            //vacuumAnalyzeTable(opt, table, &dbc);

            log.log(Level::debug, "update", table.tableName,
                    "Updated table: " + table.tableName,
                    loadTimer.elapsedTime());

            //if (opt.logLevel == Level::trace)
            //    loadTimer.print("load time");
        } // for

        for (auto& table : schema.tables)
            analyzeForeignKeys(&odbc, opt.db, &dbc, &log, schema, table);

        //tx.commit();

    }

    {
        //etymon::OdbcDbc dbc(&odbc, opt.db);

        //{
        //    etymon::OdbcTx tx(&dbc);
        //    dropOldTables(opt, &log, &dbc);
        //    tx.commit();
        //}
    }

    // TODO Check if needed for history tables; if so, move into loop above.
    //vacuumAnalyzeAll(opt, &schema, &db);

    curl_global_cleanup();  // Clean-up after curl_global_init().
}

void runUpdateProcess(const Options& opt)
{
    try {
        runUpdate(opt);
        exit(0);
    } catch (runtime_error& e) {
        string s = e.what();
        if ( !(s.empty()) && s.back() == '\n' )
            s.pop_back();
        etymon::OdbcEnv odbc;
        etymon::OdbcDbc logDbc(&odbc, opt.db);
        Log log(&logDbc, opt.logLevel, opt.prog);
        log.log(Level::error, "server", "", s, -1);
        exit(1);
    }
}

