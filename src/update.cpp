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

void make_update_tmp_dir(const options& opt, string* loaddir)
{
    fs::path datadir = opt.datadir;
    fs::path tmp = datadir / "tmp";
    fs::path tmppath = tmp / "update";
    fs::remove_all(tmppath);
    fs::create_directories(tmppath);
    *loaddir = tmppath;
}

bool is_foreign_key(etymon::odbc_conn* conn, log* lg,
        const TableSchema& table2, const ColumnSchema& column2,
        const TableSchema& table1)
{
    string sql =
        "SELECT 1\n"
        "    FROM " + table2.tableName + " AS r2\n"
        "        JOIN " + table1.tableName + " AS r1\n"
        "            ON r2." + column2.columnName + " = r1.id\n"
        "    LIMIT 1;";
    lg->detail(sql);
    etymon::odbc_stmt stmt(conn);
    try {
        conn->exec_direct(&stmt, sql);
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
        etymon::odbc_conn* conn, log* lg, const Schema& schema,
        const TableSchema& table, bool detectForeignKeys,
        map<string, vector<reference>>* refs)
{
    etymon::odbc_conn queryDBC(odbc, dbName);
    lg->detail("Searching for foreign keys in table: " + table.tableName);
    //printf("Table: %s\n", table.tableName.c_str());
    for (auto& column : table.columns) {
        if (column.columnType != ColumnType::id)
            continue;
        if (column.columnName == "id")
            continue;
        //printf("    Column: %s\n", column.columnName.c_str());
        for (auto& table1 : schema.tables) {
            if (is_foreign_key(&queryDBC, lg, table, column, table1)) {

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

void select_foreign_key_constraints(odbc_conn* conn, log* lg,
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
    conn->exec_direct(&stmt, sql);
    while (conn->fetch(&stmt)) {
        reference ref;
        conn->get_data(&stmt, 1, &(ref.referencing_table));
        conn->get_data(&stmt, 2, &(ref.referencing_column));
        conn->get_data(&stmt, 3, &(ref.referenced_table));
        conn->get_data(&stmt, 4, &(ref.referenced_column));
        conn->get_data(&stmt, 5, &(ref.constraint_name));
        refs->push_back(ref);
    }
}

void remove_foreign_key_constraints(odbc_conn* conn, log* lg)
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

void select_enabled_foreign_keys(odbc_conn* conn, log* lg,
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
    conn->exec_direct(&stmt, sql);
    while (conn->fetch(&stmt)) {
        reference ref;
        conn->get_data(&stmt, 1, &(ref.referencing_table));
        conn->get_data(&stmt, 2, &(ref.referencing_column));
        conn->get_data(&stmt, 3, &(ref.referenced_table));
        conn->get_data(&stmt, 4, &(ref.referenced_column));
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

void create_foreign_key_constraints(odbc_conn* conn, log* lg)
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

void select_config_general(etymon::odbc_conn* conn, log* lg,
        bool* detect_foreign_keys, bool* force_foreign_key_constraints,
        bool* enable_foreign_key_warnings)
{
    string sql =
        "SELECT detect_foreign_keys,\n"
        "       force_foreign_key_constraints,\n"
        "       enable_foreign_key_warnings\n"
        "    FROM ldpconfig.general;";
    lg->detail(sql);
    etymon::odbc_stmt stmt(conn);
    conn->exec_direct(&stmt, sql);
    conn->fetch(&stmt);
    string s1, s2, s3;
    conn->get_data(&stmt, 1, &s1);
    conn->get_data(&stmt, 2, &s2);
    conn->get_data(&stmt, 3, &s3);
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

    etymon::odbc_conn log_conn(&odbc, opt.db);
    log lg(&log_conn, opt.log_level, opt.console, opt.quiet, opt.prog);

    lg.write(level::debug, "server", "", "Starting full update", -1);
    timer full_update_timer(opt);

    Schema schema;
    Schema::make_default_schema(&schema);

    extraction_files ext_dir(opt);

    string loadDir;

    Curl c;
    //if (!c.curl) {
    //    // throw?
    //}
    string token, tenant_header, token_header;

    if (opt.load_from_dir != "") {
        //if (opt.logLevel == level::trace)
        //    fprintf(opt.err, "%s: Reading data from directory: %s\n",
        //            opt.prog, opt.loadFromDir.c_str());
        loadDir = opt.load_from_dir;
    } else {
        lg.write(level::trace, "", "", "Logging in to Okapi service", -1);

        okapi_login(opt, &lg, &token);

        make_update_tmp_dir(opt, &loadDir);
        ext_dir.dir = loadDir;

        tenant_header = "X-Okapi-Tenant: ";
        tenant_header + opt.okapi_tenant;
        token_header = "X-Okapi-Token: ";
        token_header += token;
        c.headers = curl_slist_append(c.headers, tenant_header.c_str());
        c.headers = curl_slist_append(c.headers, token_header.c_str());
        c.headers = curl_slist_append(c.headers,
                "Accept: application/json,text/plain");
        curl_easy_setopt(c.curl, CURLOPT_HTTPHEADER, c.headers);
    }

    string ldpconfig_disable_anonymization;
    {
        etymon::odbc_conn conn(&odbc, opt.db);
        string sql = "SELECT disable_anonymization FROM ldpconfig.general;";
        lg.detail(sql);
        {
            etymon::odbc_stmt stmt(&conn);
            conn.exec_direct(&stmt, sql);
            conn.fetch(&stmt);
            conn.get_data(&stmt, 1, &ldpconfig_disable_anonymization);
        }
    }

    for (auto& table : schema.tables) {

        if (opt.table != "" && opt.table != table.tableName)
            continue;

        bool anonymizeTable = ( table.anonymize &&
                (!opt.disable_anonymization ||
                 ldpconfig_disable_anonymization != "1") );

        //printf("anonymize=%d\tfile_disable=%d\tdb_disable=%s\tA=%d\n",
        //        table.anonymize, opt.disableAnonymization,
        //        ldpconfig_disable_anonymization.c_str(), anonymizeTable);

        if (anonymizeTable)
            continue;

        lg.write(level::trace, "", "",
                "Updating table: " + table.tableName, -1);

        timer update_timer(opt);

        extraction_files ext_files(opt);

        if (opt.load_from_dir == "") {
            lg.write(level::trace, "", "",
                    "Extracting: " + table.sourcePath, -1);
            bool found_data = directOverride(opt, table.tableName) ?
                retrieveDirect(opt, &lg, table, loadDir, &ext_files) :
                retrievePages(c, opt, &lg, token, table, loadDir,
                        &ext_files);
            if (!found_data)
                table.skip = true;
        }

        if (table.skip || opt.extract_only)
            continue;

        etymon::odbc_conn conn(&odbc, opt.db);
        //PQsetNoticeProcessor(db.conn, debugNoticeProcessor, (void*) &opt);
        dbtype dbt(&conn);

        {
            etymon::odbc_tx tx(&conn);

            lg.write(level::trace, "", "",
                    "Staging table: " + table.tableName, -1);
            bool ok = stageTable(opt, &lg, &table, &odbc, &conn, &dbt,
                    loadDir);
            if (!ok)
                continue;

            lg.write(level::trace, "", "",
                    "Merging table: " + table.tableName, -1);
            mergeTable(opt, &lg, table, &odbc, &conn, dbt);

            lg.write(level::trace, "", "",
                    "Replacing table: " + table.tableName, -1);

            remove_foreign_key_constraints(&conn, &lg);
            dropTable(opt, &lg, table.tableName, &conn);

            placeTable(opt, &lg, table, &conn);
            //updateStatus(opt, table, &conn);

            //updateDBPermissions(opt, &lg, &conn);

            tx.commit();
        }

        //vacuumAnalyzeTable(opt, table, &conn);

        string sql =
            "SELECT COUNT(*) FROM\n"
            "    " + table.tableName + ";";
        lg.detail(sql);
        string rowCount;
        {
            etymon::odbc_stmt stmt(&conn);
            conn.exec_direct(&stmt, sql);
            conn.fetch(&stmt);
            conn.get_data(&stmt, 1, &rowCount);
        }
        sql =
            "SELECT COUNT(*) FROM\n"
            "    history." + table.tableName + ";";
        lg.detail(sql);
        string history_row_count;
        {
            etymon::odbc_stmt stmt(&conn);
            conn.exec_direct(&stmt, sql);
            conn.fetch(&stmt);
            conn.get_data(&stmt, 1, &history_row_count);
        }
        sql =
            "UPDATE ldpsystem.tables\n"
            "    SET updated = " + string(dbt.current_timestamp()) + ",\n"
            "        row_count = " + rowCount + ",\n"
            "        history_row_count = " + history_row_count + ",\n"
            "        documentation = '" + table.sourcePath + " in "
            + table.moduleName + "',\n"
            "        documentation_url = 'https://dev.folio.org/reference/api/#"
            + table.moduleName + "'\n"
            "    WHERE table_name = '" + table.tableName + "';";
        lg.detail(sql);
        conn.exec(sql);

        lg.write(level::debug, "update", table.tableName,
                "Updated table: " + table.tableName,
                update_timer.elapsed_time());

        //if (opt.logLevel == level::trace)
        //    loadTimer.print("load time");
    } // for

    //{
    //    etymon::odbc_conn conn(&odbc, opt.db);
    //    {
    //        etymon::odbc_tx tx(&conn);
    //        dropOldTables(opt, &lg, &conn);
    //        tx.commit();
    //    }
    //}

    lg.write(level::debug, "server", "", "Completed full update",
            full_update_timer.elapsed_time());

    // TODO Move analysis and constraints out of update process.
    {
        etymon::odbc_conn conn(&odbc, opt.db);

        bool detect_foreign_keys = false;
        bool force_foreign_key_constraints = false;
        //bool enable_foreign_key_warnings = false;

        ///////////////////////////////////////////////////////////////////////
        // Temporarily removed until 1.0 release.
        //select_config_general(&conn, &lg, &detect_foreign_keys,
        //        &force_foreign_key_constraints, &enable_foreign_key_warnings);
        ///////////////////////////////////////////////////////////////////////

        if (detect_foreign_keys) {

            lg.write(level::debug, "server", "",
                    "Detecting foreign keys", -1);

            timer ref_timer(opt);

            etymon::odbc_tx tx(&conn);

            map<string, vector<reference>> refs;
            for (auto& table : schema.tables)
                search_table_foreign_keys(&odbc, opt.db, &conn, &lg, schema,
                        table, detect_foreign_keys, &refs);

            string sql = "DELETE FROM ldpconfig.foreign_keys;";
            lg.detail(sql);
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
                    lg.detail(sql);
                    conn.exec(sql);
                }
            }

            tx.commit();

            lg.write(level::debug, "server", "",
                    "Completed foreign key detection",
                    ref_timer.elapsed_time());
        }

        if (force_foreign_key_constraints) {

            lg.write(level::debug, "server", "",
                    "Creating foreign key constraints", -1);

            timer ref_timer(opt);

            create_foreign_key_constraints(&conn, &lg);

            lg.write(level::debug, "server", "",
                    "Foreign key constraints created",
                    ref_timer.elapsed_time());
        }

    }

}

void run_update_process(const options& opt)
{
#ifdef GPROF
    string update_dir = "./update-gprof";
    fs::create_directories(update_dir);
    chdir(update_dir.c_str());
#endif
    try {
        run_update(opt);
        exit(0);
    } catch (runtime_error& e) {
        string s = e.what();
        if ( !(s.empty()) && s.back() == '\n' )
            s.pop_back();
        etymon::odbc_env odbc;
        etymon::odbc_conn log_conn(&odbc, opt.db);
        log lg(&log_conn, opt.log_level, opt.console, opt.quiet, opt.prog);
        lg.write(level::error, "server", "", s, -1);
        exit(1);
    }
}

