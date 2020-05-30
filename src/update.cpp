#include <cstdint>
#include <curl/curl.h>
#include <experimental/filesystem>
#include <iostream>
#include <map>
#include <stdexcept>
#include <sys/stat.h>
#include <sys/types.h>

#include "../etymoncpp/include/curl.h"
#include "extract.h"
#include "init.h"
#include "log.h"
#include "merge.h"
#include "stage.h"
#include "timer.h"
#include "update.h"

using namespace etymon;
namespace fs = std::experimental::filesystem;

void makeUpdateTmpDir(const options& opt, string* loaddir)
{
    fs::path datadir = opt.datadir;
    fs::path tmp = datadir / "tmp";
    fs::path tmppath = tmp / "update";
    fs::remove_all(tmppath);
    fs::create_directories(tmppath);
    *loaddir = tmppath;
}

bool is_foreign_key(etymon::odbc_conn* conn, Log* log,
        const TableSchema& table2, const ColumnSchema& column2,
        const TableSchema& table1)
{
    string sql =
        "SELECT 1\n"
        "    FROM " + table2.tableName + " AS r2\n"
        "        JOIN " + table1.tableName + " AS r1\n"
        "            ON r2." + column2.columnName + " = r1.id\n"
        "    LIMIT 1;";
    log->logDetail(sql);
    etymon::odbc_stmt stmt(conn);
    try {
        conn->execDirect(&stmt, sql);
    } catch (runtime_error& e) {
        return false;
    }
    return conn->fetch(&stmt);
}

class reference {
public:
    string referencing_table;
    string referencing_column;
    string referenced_table;
    string referenced_column;
    string constraint_name;
};

void search_table_foreign_keys(etymon::odbc_env* odbc, const string& dbName,
        etymon::odbc_conn* conn, Log* log, const Schema& schema,
        const TableSchema& table, bool detectForeignKeys,
        map<string, vector<reference>>* refs)
{
    etymon::odbc_conn queryDBC(odbc, dbName);
    log->logDetail("Searching for foreign keys in table: " + table.tableName);
    //printf("Table: %s\n", table.tableName.c_str());
    for (auto& column : table.columns) {
        if (column.columnType != ColumnType::id)
            continue;
        if (column.columnName == "id")
            continue;
        //printf("    Column: %s\n", column.columnName.c_str());
        for (auto& table1 : schema.tables) {
            if (is_foreign_key(&queryDBC, log, table, column, table1)) {

                string key = table.tableName + "." + column.columnName;
                reference ref = {
                    table.tableName,
                    column.columnName,
                    table1.tableName,
                    "id"
                };

                //fprintf(stderr, "%s(%s) -> %s(%s)\n",
                //        ref.referencing_table.c_str(),
                //        ref.referencing_column.c_str(),
                //        ref.referenced_table.c_str(),
                //        ref.referenced_column.c_str());

                (*refs)[key].push_back(ref);

            }
        }
    }
}

void select_foreign_key_constraints(odbc_conn* conn, Log* lg,
        vector<reference>* refs)
{
    refs->clear();
    string sql =
        "SELECT referencing_table,\n"
        "       referencing_column,\n"
        "       referenced_table,\n"
        "       referenced_column,\n"
        "       constraint_name\n"
        "    FROM ldpsystem.foreign_key_constraints;";
    lg->detail(sql);
    etymon::odbc_stmt stmt(conn);
    conn->execDirect(&stmt, sql);
    while (conn->fetch(&stmt)) {
        reference ref;
        conn->getData(&stmt, 1, &(ref.referencing_table));
        conn->getData(&stmt, 2, &(ref.referencing_column));
        conn->getData(&stmt, 3, &(ref.referenced_table));
        conn->getData(&stmt, 4, &(ref.referenced_column));
        conn->getData(&stmt, 5, &(ref.constraint_name));
        refs->push_back(ref);
    }
}

void remove_foreign_key_constraints(odbc_conn* conn, Log* lg)
{
    etymon::odbc_tx tx(conn);
    vector<reference> refs;
    select_foreign_key_constraints(conn, lg, &refs);
    for (auto& ref : refs) {
        string sql =
            "ALTER TABLE " + ref.referencing_table + "\n"
            "    DROP CONSTRAINT " + ref.constraint_name + " CASCADE;";
        lg->detail(sql);
        conn->exec(sql);
    }
    string sql = "DELETE FROM ldpsystem.foreign_key_constraints;";
    lg->detail(sql);
    conn->exec(sql);
    tx.commit();
}

void select_enabled_foreign_keys(odbc_conn* conn, Log* lg,
        vector<reference>* refs)
{
    refs->clear();
    string sql =
        "SELECT referencing_table,\n"
        "       referencing_column,\n"
        "       referenced_table,\n"
        "       referenced_column\n"
        "    FROM ldpconfig.foreign_keys\n"
        "    WHERE enable_constraint = TRUE;";
    lg->detail(sql);
    etymon::odbc_stmt stmt(conn);
    conn->execDirect(&stmt, sql);
    while (conn->fetch(&stmt)) {
        reference ref;
        conn->getData(&stmt, 1, &(ref.referencing_table));
        conn->getData(&stmt, 2, &(ref.referencing_column));
        conn->getData(&stmt, 3, &(ref.referenced_table));
        conn->getData(&stmt, 4, &(ref.referenced_column));
        refs->push_back(ref);
    }
}

void make_foreign_key_constraint_name(const string& referencing_table,
        const string& referencing_column, string* constraint_name)
{
    const char* p = referencing_table.c_str();
    while (*p != '\0' && *p != '_')
        p++;
    if (*p == '_')
        p++;
    *constraint_name = string(p) + "_" + referencing_column + "_fk";
}

void create_foreign_key_constraints(odbc_conn* conn, Log* lg)
{
    vector<reference> refs;
    select_enabled_foreign_keys(conn, lg, &refs);
    for (auto& ref : refs) {
        string sql;
        try {
            sql =
                "DELETE FROM\n"
                "    " + ref.referencing_table + "\n"
                "    WHERE " + ref.referencing_column + "\n"
                "    NOT IN (\n"
                "        SELECT " + ref.referenced_column + "\n"
                "            FROM " + ref.referenced_table + "\n"
                "    );";
            lg->detail(sql);
            conn->exec(sql);
        } catch (runtime_error& e) {
            // TODO Log warning
        }
        string constraint_name;
        make_foreign_key_constraint_name(ref.referencing_table,
                ref.referencing_column, &constraint_name);
        try {
            sql =
                "ALTER TABLE " + ref.referencing_table + "\n"
                "    ADD CONSTRAINT\n"
                "    " + constraint_name + "\n"
                "    FOREIGN KEY (" + ref.referencing_column + ")\n"
                "    REFERENCES " + ref.referenced_table + " (" +
                ref.referenced_column + ");";
            lg->detail(sql);
            conn->exec(sql);
            sql =
                "INSERT INTO ldpsystem.foreign_key_constraints\n"
                "    (referencing_table, referencing_column,\n"
                "     referenced_table, referenced_column, constraint_name)\n"
                "    VALUES\n"
                "    ('" + ref.referencing_table + "',\n"
                "     '" + ref.referencing_column + "',\n"
                "     '" + ref.referenced_table + "',\n"
                "     '" + ref.referenced_column + "',\n"
                "     '" + constraint_name + "');";
            lg->detail(sql);
            conn->exec(sql);
        } catch (runtime_error& e) {
            // TODO Log warning
        }
    }
}

void select_config_general(etymon::odbc_conn* conn, Log* log,
        bool* detect_foreign_keys, bool* force_foreign_key_constraints,
        bool* enable_foreign_key_warnings)
{
    string sql =
        "SELECT detect_foreign_keys,\n"
        "       force_foreign_key_constraints,\n"
        "       enable_foreign_key_warnings\n"
        "    FROM ldpconfig.general;";
    log->logDetail(sql);
    etymon::odbc_stmt stmt(conn);
    conn->execDirect(&stmt, sql);
    conn->fetch(&stmt);
    string s1, s2, s3;
    conn->getData(&stmt, 1, &s1);
    conn->getData(&stmt, 2, &s2);
    conn->getData(&stmt, 3, &s3);
    conn->fetch(&stmt);
    *detect_foreign_keys = (s1 == "1");
    *force_foreign_key_constraints = (s2 == "1");
    *enable_foreign_key_warnings = (s3 == "1");
}

void run_update(const options& opt)
{
    CURLcode cc;
    curl_global curl_env(CURL_GLOBAL_ALL, &cc);
    if (cc) {
        throw runtime_error(string("Error initializing curl: ") +
                curl_easy_strerror(cc));
    }

    etymon::odbc_env odbc;

    etymon::odbc_conn logDbc(&odbc, opt.db);
    Log log(&logDbc, opt.log_level, opt.console, opt.quiet, opt.prog);

    log.log(Level::debug, "server", "", "Starting full update", -1);
    Timer fullUpdateTimer(opt);

    Schema schema;
    Schema::MakeDefaultSchema(&schema);

    ExtractionFiles extractionDir(opt);

    string loadDir;

    Curl c;
    //if (!c.curl) {
    //    // throw?
    //}
    string token, tenantHeader, tokenHeader;

    if (opt.load_from_dir != "") {
        //if (opt.logLevel == Level::trace)
        //    fprintf(opt.err, "%s: Reading data from directory: %s\n",
        //            opt.prog, opt.loadFromDir.c_str());
        loadDir = opt.load_from_dir;
    } else {
        log.log(Level::trace, "", "", "Logging in to Okapi service", -1);

        okapiLogin(opt, &log, &token);

        makeUpdateTmpDir(opt, &loadDir);
        extractionDir.dir = loadDir;

        tenantHeader = "X-Okapi-Tenant: ";
        tenantHeader + opt.okapi_tenant;
        tokenHeader = "X-Okapi-Token: ";
        tokenHeader += token;
        c.headers = curl_slist_append(c.headers, tenantHeader.c_str());
        c.headers = curl_slist_append(c.headers, tokenHeader.c_str());
        c.headers = curl_slist_append(c.headers,
                "Accept: application/json,text/plain");
        curl_easy_setopt(c.curl, CURLOPT_HTTPHEADER, c.headers);
    }

    string ldpconfigDisableAnonymization;
    {
        etymon::odbc_conn conn(&odbc, opt.db);
        string sql = "SELECT disable_anonymization FROM ldpconfig.general;";
        log.logDetail(sql);
        {
            etymon::odbc_stmt stmt(&conn);
            conn.execDirect(&stmt, sql);
            conn.fetch(&stmt);
            conn.getData(&stmt, 1, &ldpconfigDisableAnonymization);
        }
    }

    for (auto& table : schema.tables) {

        if (opt.table != "" && opt.table != table.tableName)
            continue;

        bool anonymizeTable = ( table.anonymize &&
                (!opt.disable_anonymization ||
                 ldpconfigDisableAnonymization != "1") );

        //printf("anonymize=%d\tfile_disable=%d\tdb_disable=%s\tA=%d\n",
        //        table.anonymize, opt.disableAnonymization,
        //        ldpconfigDisableAnonymization.c_str(), anonymizeTable);

        if (anonymizeTable)
            continue;

        log.log(Level::trace, "", "",
                "Updating table: " + table.tableName, -1);

        Timer updateTimer(opt);

        ExtractionFiles extractionFiles(opt);

        if (opt.load_from_dir == "") {
            log.log(Level::trace, "", "",
                    "Extracting: " + table.sourcePath, -1);
            bool foundData = directOverride(opt, table.tableName) ?
                retrieveDirect(opt, &log, table, loadDir, &extractionFiles) :
                retrievePages(c, opt, &log, token, table, loadDir,
                        &extractionFiles);
            if (!foundData)
                table.skip = true;
        }

        if (table.skip || opt.extract_only)
            continue;

        etymon::odbc_conn conn(&odbc, opt.db);
        //PQsetNoticeProcessor(db.conn, debugNoticeProcessor, (void*) &opt);
        DBType dbt(&conn);

        {
            etymon::odbc_tx tx(&conn);

            log.log(Level::trace, "", "",
                    "Staging table: " + table.tableName, -1);
            bool ok = stageTable(opt, &log, &table, &odbc, &conn, &dbt,
                    loadDir);
            if (!ok)
                break;

            log.log(Level::trace, "", "",
                    "Merging table: " + table.tableName, -1);
            mergeTable(opt, &log, table, &odbc, &conn, dbt);

            log.log(Level::trace, "", "",
                    "Replacing table: " + table.tableName, -1);

            remove_foreign_key_constraints(&conn, &log);
            dropTable(opt, &log, table.tableName, &conn);

            placeTable(opt, &log, table, &conn);
            //updateStatus(opt, table, &conn);

            //updateDBPermissions(opt, &log, &conn);

            tx.commit();
        }

        //vacuumAnalyzeTable(opt, table, &conn);

        string sql = 
            "SELECT COUNT(*) FROM\n"
            "    " + table.tableName + ";";
        log.logDetail(sql);
        string rowCount;
        {
            etymon::odbc_stmt stmt(&conn);
            conn.execDirect(&stmt, sql);
            conn.fetch(&stmt);
            conn.getData(&stmt, 1, &rowCount);
        }
        sql = 
            "SELECT COUNT(*) FROM\n"
            "    history." + table.tableName + ";";
        log.logDetail(sql);
        string historyRowCount;
        {
            etymon::odbc_stmt stmt(&conn);
            conn.execDirect(&stmt, sql);
            conn.fetch(&stmt);
            conn.getData(&stmt, 1, &historyRowCount);
        }
        sql =
            "UPDATE ldpsystem.tables\n"
            "    SET updated = " + string(dbt.currentTimestamp()) + ",\n"
            "        row_count = " + rowCount + ",\n"
            "        history_row_count = " + historyRowCount + ",\n"
            "        documentation = '" + table.sourcePath + " in "
            + table.moduleName + "',\n"
            "        documentation_url = 'https://dev.folio.org/reference/api/#"
            + table.moduleName + "'\n"
            "    WHERE table_name = '" + table.tableName + "';";
        log.logDetail(sql);
        conn.execDirect(nullptr, sql);

        log.log(Level::debug, "update", table.tableName,
                "Updated table: " + table.tableName,
                updateTimer.elapsedTime());

        //if (opt.logLevel == Level::trace)
        //    loadTimer.print("load time");
    } // for

    //{
    //    etymon::odbc_conn conn(&odbc, opt.db);
    //    {
    //        etymon::odbc_tx tx(&conn);
    //        dropOldTables(opt, &log, &conn);
    //        tx.commit();
    //    }
    //}

    log.log(Level::debug, "server", "", "Completed full update",
            fullUpdateTimer.elapsedTime());

    // TODO Move analysis and constraints out of update process.
    {
        etymon::odbc_conn conn(&odbc, opt.db);

        bool detect_foreign_keys = false;
        bool force_foreign_key_constraints = false;
        bool enable_foreign_key_warnings = false;
        select_config_general(&conn, &log, &detect_foreign_keys,
                &force_foreign_key_constraints, &enable_foreign_key_warnings);

        if (detect_foreign_keys) {

            log.log(Level::debug, "server", "",
                    "Detecting foreign keys", -1);

            Timer refTimer(opt);

            etymon::odbc_tx tx(&conn);

            map<string, vector<reference>> refs;
            for (auto& table : schema.tables)
                search_table_foreign_keys(&odbc, opt.db, &conn, &log, schema,
                        table, detect_foreign_keys, &refs);

            string sql = "DELETE FROM ldpconfig.foreign_keys;";
            log.detail(sql);
            conn.exec(sql);

            for (pair<string, vector<reference>> p : refs) {
                bool enable = (p.second.size() == 1);
                for (auto& r : p.second) {
                    sql =
                        "INSERT INTO ldpconfig.foreign_keys\n"
                        "    (enable_constraint,\n"
                        "        referencing_table, referencing_column,\n"
                        "        referenced_table, referenced_column)\n"
                        "VALUES\n"
                        "    (" + string(enable ? "TRUE" : "FALSE") + ",\n"
                        "        '" + r.referencing_table + "',\n"
                        "        '" + r.referencing_column + "',\n"
                        "        '" + r.referenced_table + "',\n"
                        "        '" + r.referenced_column + "');";
                    log.detail(sql);
                    conn.exec(sql);
                }
            }

            tx.commit();

            log.log(Level::debug, "server", "",
                    "Completed foreign key detection",
                    refTimer.elapsedTime());
        }

        if (force_foreign_key_constraints) {

            log.log(Level::debug, "server", "",
                    "Creating foreign key constraints", -1);

            Timer refTimer(opt);

            create_foreign_key_constraints(&conn, &log);

            log.log(Level::debug, "server", "",
                    "Foreign key constraints created",
                    refTimer.elapsedTime());
        }

    }

}

void run_update_process(const options& opt)
{
#ifdef GPROF
    string updateDir = "./update-gprof";
    fs::create_directories(updateDir);
    chdir(updateDir.c_str());
#endif
    try {
        run_update(opt);
        exit(0);
    } catch (runtime_error& e) {
        string s = e.what();
        if ( !(s.empty()) && s.back() == '\n' )
            s.pop_back();
        etymon::odbc_env odbc;
        etymon::odbc_conn logDbc(&odbc, opt.db);
        Log log(&logDbc, opt.log_level, opt.console, opt.quiet, opt.prog);
        log.log(Level::error, "server", "", s, -1);
        exit(1);
    }
}

