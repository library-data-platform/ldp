#define _LIBCPP_NO_EXPERIMENTAL_DEPRECATION_WARNING_FILESYSTEM

#include <cstdint>
#include <curl/curl.h>
#include <experimental/filesystem>
#include <iostream>
#include <map>
#include <stdexcept>
#include <sys/stat.h>
#include <sys/types.h>

#include "../etymoncpp/include/curl.h"
#include "addcolumns.h"
#include "dropfields.h"
#include "extract.h"
#include "init.h"
#include "log.h"
#include "merge.h"
#include "stage.h"
#include "timer.h"
#include "update.h"

namespace fs = std::experimental::filesystem;

void make_update_tmp_dir(const ldp_options& opt, string* loaddir)
{
    fs::path datadir = opt.datadir;
    fs::path tmp = datadir / "tmp";
    fs::path tmppath = tmp / "update";
    fs::remove_all(tmppath);
    fs::create_directories(tmppath);
    *loaddir = tmppath;
}

bool is_foreign_key(etymon::odbc_conn* conn, ldp_log* lg,
        const table_schema& table2, const column_schema& column2,
        const table_schema& table1)
{
    string sql =
        "SELECT 1\n"
        "    FROM " + table2.name + " AS r2\n"
        "        JOIN " + table1.name + " AS r1\n"
        "            ON r2." + column2.name + " = r1.id\n"
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
                               etymon::odbc_conn* conn, ldp_log* lg,
                               const ldp_schema& schema,
                               const table_schema& table,
                               bool detectForeignKeys,
                               map<string, vector<reference>>* refs)
{
    etymon::odbc_conn query_conn(odbc, dbName);
    lg->detail("Searching for foreign keys in table: " + table.name);
    //printf("Table: %s\n", table.name.c_str());
    for (auto& column : table.columns) {
        if (column.type != column_type::id)
            continue;
        if (column.name == "id")
            continue;
        //printf("    Column: %s\n", column.name.c_str());
        for (auto& table1 : schema.tables) {
            if (is_foreign_key(&query_conn, lg, table, column, table1)) {

                string key = table.name + "." + column.name;
                reference ref = {
                    table.name,
                    column.name,
                    table1.name,
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

void select_foreign_key_constraints(etymon::odbc_conn* conn, ldp_log* lg,
        vector<reference>* refs)
{
    refs->clear();
    string sql =
        "SELECT referencing_table,\n"
        "       referencing_column,\n"
        "       referenced_table,\n"
        "       referenced_column,\n"
        "       constraint_name\n"
        "    FROM dbsystem.foreign_key_constraints;";
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

void remove_foreign_key_constraints(etymon::odbc_conn* conn, ldp_log* lg)
{
    vector<reference> refs;
    select_foreign_key_constraints(conn, lg, &refs);
    for (auto& ref : refs) {
        string sql =
            "ALTER TABLE " + ref.referencing_table + "\n"
            "    DROP CONSTRAINT " + ref.constraint_name + " CASCADE;";
        lg->detail(sql);
        conn->exec(sql);
    }
    string sql = "DELETE FROM dbsystem.foreign_key_constraints;";
    lg->detail(sql);
    conn->exec(sql);
}

void select_enabled_foreign_keys(etymon::odbc_conn* conn, ldp_log* lg,
        vector<reference>* refs)
{
    refs->clear();
    string sql =
        "SELECT referencing_table,\n"
        "       referencing_column,\n"
        "       referenced_table,\n"
        "       referenced_column\n"
        "    FROM dbconfig.foreign_keys\n"
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

void log_foreign_key_warnings(const reference& ref,
                              bool force_foreign_key_constraints,
                              etymon::odbc_conn* conn, ldp_log* lg)
{
    string sql;
    try {
        sql =
            "SELECT referencing_pkey,\n"
            "       referencing_fkey\n"
            "    FROM temp_foreign_key_exceptions;";
        lg->detail(sql);
        etymon::odbc_stmt stmt(conn);
        conn->exec_direct(&stmt, sql);
        while (conn->fetch(&stmt)) {
            string pkey, fkey;
            conn->get_data(&stmt, 1, &pkey);
            conn->get_data(&stmt, 2, &fkey);
            lg->write(log_level::warning, "foreign_key", ref.referenced_table,
                "Foreign key is not present in referenced table:\n"
                "    Referencing table: " + ref.referencing_table + "\n"
                "    Referencing table primary key: " + pkey + "\n"
                "    Referencing column: " + ref.referencing_column + "\n"
                "    Referencing column foreign key: " + fkey + "\n"
                "    Referenced table: " + ref.referenced_table + "\n"
                "    Referenced column: " + ref.referenced_column + "\n"
                "    Action: " +
                ( force_foreign_key_constraints ?
                  "Deleting row in referencing table" : "None" ), -1 );
        }
        
    } catch (runtime_error& e) {
        string s = e.what();
        if ( !(s.empty()) && s.back() == '\n' )
            s.pop_back();
        lg->detail(s);
    }
}

void process_foreign_keys(const ldp_options& opt, bool enable_foreign_key_warnings, bool force_foreign_key_constraints, etymon::odbc_conn* conn, ldp_log* lg)
{
    vector<reference> refs;
    select_enabled_foreign_keys(conn, lg, &refs);
    for (auto& ref : refs) {
        try {
            string sql =
                "CREATE TEMPORARY TABLE temp_foreign_key_exceptions AS\n"
                "SELECT id AS referencing_pkey,\n"
                "       \"" + ref.referencing_column + "\"\n"
                "               AS referencing_fkey\n"
                "    FROM " + ref.referencing_table + "\n"
                "    WHERE \"" + ref.referencing_column + "\"\n"
                "    NOT IN (\n"
                "        SELECT \"" + ref.referenced_column + "\"\n"
                "            FROM " + ref.referenced_table + "\n"
                "    );";
            lg->detail(sql);
            conn->exec(sql);
            sql =
                "CREATE INDEX ON temp_foreign_key_exceptions\n"
                "    (referencing_fkey);";
            lg->detail(sql);
            conn->exec(sql);
            string v;
            vacuum_sql(opt, &v);
            sql = v + "temp_foreign_key_exceptions;";
            lg->detail(sql);
            conn->exec(sql);
            sql = "ANALYZE temp_foreign_key_exceptions;";
            lg->detail(sql);
            conn->exec(sql);
        } catch (runtime_error& e) {
            string s = e.what();
            if ( !(s.empty()) && s.back() == '\n' )
                s.pop_back();
            lg->detail(s);
        }

        if (enable_foreign_key_warnings)
            log_foreign_key_warnings(ref, force_foreign_key_constraints, conn,
                                     lg);

        if (force_foreign_key_constraints) {
            string sql;
            try {
                sql =
                    "DELETE\n"
                    "    FROM " + ref.referencing_table + "\n"
                    "    WHERE \"" + ref.referencing_column + "\"\n"
                    "    IN (\n"
                    "        SELECT referencing_fkey\n"
                    "            FROM temp_foreign_key_exceptions\n"
                    "    );";
                lg->detail(sql);
                conn->exec(sql);
            } catch (runtime_error& e) {
                string s = e.what();
                if ( !(s.empty()) && s.back() == '\n' )
                    s.pop_back();
                lg->detail(s);
            }
        }

        string sql = "DROP TABLE IF EXISTS temp_foreign_key_exceptions;";
        lg->detail(sql);
        conn->exec(sql);

        if (!force_foreign_key_constraints)
            continue;
        
        string constraint_name;
        make_foreign_key_constraint_name(ref.referencing_table,
                ref.referencing_column, &constraint_name);
        try {
            string sql =
                "ALTER TABLE " + ref.referencing_table + "\n"
                "    ADD CONSTRAINT\n"
                "    " + constraint_name + "\n"
                "    FOREIGN KEY (\"" + ref.referencing_column + "\")\n"
                "    REFERENCES " + ref.referenced_table + " (" +
                ref.referenced_column + ");";
            lg->detail(sql);
            conn->exec(sql);
            sql =
                "INSERT INTO dbsystem.foreign_key_constraints\n"
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
            string s = e.what();
            if ( !(s.empty()) && s.back() == '\n' )
                s.pop_back();
            lg->detail(s);
        }
    }
}

void select_config_general(etymon::odbc_conn* conn, ldp_log* lg,
        bool* detect_foreign_keys, bool* force_foreign_key_constraints,
        bool* enable_foreign_key_warnings)
{
    string sql =
        "SELECT detect_foreign_keys,\n"
        "       force_foreign_key_constraints,\n"
        "       enable_foreign_key_warnings\n"
        "    FROM dbconfig.general;";
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

void run_update(const ldp_options& opt)
{
    CURLcode cc;
    etymon::curl_global curl_env(CURL_GLOBAL_ALL, &cc);
    if (cc) {
        throw runtime_error(string("error initializing curl: ") +
                            curl_easy_strerror(cc));
    }

    etymon::odbc_env odbc;

    etymon::odbc_conn log_conn(&odbc, opt.db);
    ldp_log lg(&log_conn, opt.lg_level, opt.console, opt.quiet);

    if (!opt.record_history) {
        lg.write(log_level::info, "server", "", "recording history is disabled", -1);
    }
    if (!opt.parallel_vacuum) {
        lg.write(log_level::info, "server", "", "parallel vacuum is disabled", -1);
    }

    lg.write(log_level::debug, "server", "", "starting full update", -1);
    timer full_update_timer;

    lg.write(log_level::detail, "", "", "okapi timeout: " + to_string(opt.okapi_timeout), -1);

    ldp_schema schema;
    ldp_schema::make_default_schema(&schema);

    extraction_files ext_dir(opt, &lg);

    string load_dir;

    vector<source_state> source_states;
    //string token, tenant_header, token_header;

    if (opt.load_from_dir != "") {
        //if (opt.logLevel == log_level::trace)
        //    fprintf(opt.err, "%s: Reading data from directory: %s\n",
        //            opt.prog, opt.loadFromDir.c_str());
        load_dir = opt.load_from_dir;
        data_source source;
        source_state state(source);
        source_states.push_back(state);
    } else {

        for (auto& source : opt.enable_sources) {

            source_state state(source);

            okapi_login(opt, source, &lg, &state.token);

            make_update_tmp_dir(opt, &load_dir);
            ext_dir.dir = load_dir;

            source_states.push_back(state);
        }
    }

    string current_module = "";

    for (auto& table : schema.tables) {

        try {

            // Skip this table if the --table option is specified and does not
            // match this table.
            if (opt.table != "" && opt.table != table.name)
                continue;

            // Enable anonymization of the entire table.
            bool anonymize_table = opt.anonymize && table.anonymize;
            field_set drop_fields;
            if (opt.anonymize) {
                load_anonymize_field_list(&drop_fields);
            }
            read_drop_fields(opt, &lg, &drop_fields);

            // Skip this table if the entire table should be anonymized.
            if (anonymize_table)
                continue;

            if (table.module_name != current_module) {
                current_module = table.module_name;
                lg.write(log_level::debug, "update", "", "module: " + current_module, -1);
            }

            lg.write(log_level::detail, "", "", "updating table: " + table.name, -1);

            timer update_timer;

            extraction_files ext_files(opt, &lg);

            for (auto& state : source_states) {

                curl_wrapper curlw;
                //if (!c.curl) {
                //    // throw?
                //}
                string tenant_header = "X-Okapi-Tenant: ";
                tenant_header + state.source.okapi_tenant;
                string token_header = "X-Okapi-Token: ";
                token_header += state.token;
                curlw.headers = curl_slist_append(curlw.headers,
                                                  tenant_header.c_str());
                curlw.headers = curl_slist_append(curlw.headers,
                                                  token_header.c_str());
                curlw.headers = curl_slist_append(
                    curlw.headers, "Accept: application/json,text/plain");
                curl_easy_setopt(curlw.curl, CURLOPT_HTTPHEADER,
                                 curlw.headers);

                if (opt.load_from_dir == "") {
                    lg.write(log_level::trace, "", "", "reading: " + table.source_spec, -1);
                    bool found_data = false;
                    if (direct_override(state.source, table.name)) {
                        found_data = retrieve_direct(state.source, &lg, table, load_dir, &ext_files, opt.direct_extraction_no_ssl);
                    } else {
                        if (table.source_type != data_source_type::srs_marc_records && table.source_type != data_source_type::srs_records) {
                            found_data = retrieve_pages(curlw, opt, state.source, &lg, state.token, table, load_dir, &ext_files);
                        } else {
                            lg.write(log_level::debug, "", "", "table not updated: " + table.name + ": requires direct extraction", -1);
                        }
                    }
                    if (!found_data) {
                        table.skip = true;
                    }
                }
            } // for

            if (table.skip || opt.extract_only)
                continue;

            etymon::odbc_conn conn(&odbc, opt.db);
            //PQsetNoticeProcessor(db.conn, debugNoticeProcessor, (void*) &opt);
            dbtype dbt(&conn);

            if (opt.record_history) {
                create_latest_history_table(opt, &lg, table, &conn);
            }

            {
                etymon::odbc_tx tx(&conn);

                lg.write(log_level::trace, "", "", "staging: " + table.name, -1);
                bool ok = stage_table_1(opt, source_states, &lg, &table, &odbc,
                                        &conn, &dbt, load_dir, &drop_fields);
                if (!ok)
                    continue;

                ok = stage_table_2(opt, source_states, &lg, &table, &odbc,
                                   &conn, &dbt, load_dir,
                                   &drop_fields);
                if (!ok)
                    continue;

                if (opt.record_history && table.source_type != data_source_type::srs_marc_records && table.source_type != data_source_type::srs_records) {
                    lg.write(log_level::trace, "", "", "merging: " + table.name, -1);
                    merge_table(opt, &lg, table, &odbc, &conn, dbt);
                }

                remove_foreign_key_constraints(&conn, &lg);
                drop_table(opt, &lg, table.name, &conn);

                place_table(opt, &lg, table, &conn);
                //updateStatus(opt, table, &conn);

                //updateDBPermissions(opt, &lg, &conn);

                tx.commit();
            }

            index_loaded_table(&lg, table, &conn, &dbt, opt.index_large_varchar);

            if (opt.record_history) {
                drop_latest_history_table(opt, &lg, table, &conn);
            }

            //vacuumAnalyzeTable(opt, table, &conn);

            string sql =
                "SELECT COUNT(*) FROM\n"
                "    " + table.name + ";";
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
                "    history." + table.name + ";";
            lg.detail(sql);
            string history_row_count;
            {
                etymon::odbc_stmt stmt(&conn);
                conn.exec_direct(&stmt, sql);
                conn.fetch(&stmt);
                conn.get_data(&stmt, 1, &history_row_count);
            }
            sql =
                "UPDATE dbsystem.tables\n"
                "    SET updated = " + string(dbt.current_timestamp()) + ",\n"
                "        row_count = " + rowCount + ",\n"
                "        history_row_count = " + history_row_count + ",\n"
                "        documentation = '" + table.source_spec + " in "
                + table.module_name + "',\n"
                "        documentation_url = 'https://dev.folio.org/reference/api/#"
                + table.module_name + "'\n"
                "    WHERE table_name = '" + table.name + "';";
            lg.detail(sql);
            conn.exec(sql);

            lg.write(log_level::debug, "update", table.name, "updated table: " + table.name, update_timer.elapsed_time());

            //if (opt.logLevel == log_level::trace)
            //    loadTimer.print("load time");

        } catch (runtime_error& e) {
            string s = table.name + ": " + e.what();
            if ( !(s.empty()) && s.back() == '\n' )
                s.pop_back();
            etymon::odbc_env odbc;
            etymon::odbc_conn log_conn(&odbc, opt.db);
            ldp_log lg(&log_conn, opt.lg_level, opt.console, opt.quiet);
            lg.write(log_level::error, "server", "", s, -1);
        }
        
    } // for

    //{
    //    etymon::odbc_conn conn(&odbc, opt.db);
    //    {
    //        etymon::odbc_tx tx(&conn);
    //        dropOldTables(opt, &lg, &conn);
    //        tx.commit();
    //    }
    //}

    lg.write(log_level::debug, "server", "", "completed full update",
            full_update_timer.elapsed_time());

    // Add optional columns
    add_optional_columns(opt, &lg, &odbc);

    // Vacuum and analyze all updated tables
    {
        etymon::odbc_conn conn(&odbc, opt.db);
        lg.write(log_level::debug, "server", "", "starting vacuum", -1);
        timer vacuum_analyze_timer;
        string v;
        vacuum_sql(opt, &v);
        for (auto& table : schema.tables) {
            if (table.skip || opt.extract_only)
                continue;
            string sql = v + table.name + ";";
            lg.detail(sql);
            conn.exec(sql);
            sql = "ANALYZE " + table.name + ";";
            lg.detail(sql);
            conn.exec(sql);
            if (opt.record_history) {
                sql = v + "history." + table.name + ";";
                lg.detail(sql);
                conn.exec(sql);
                sql = "ANALYZE history." + table.name + ";";
                lg.detail(sql);
                conn.exec(sql);
            }
        }
        lg.write(log_level::debug, "server", "", "completed vacuum",
                 vacuum_analyze_timer.elapsed_time());
    }

    // TODO Move analysis and constraints out of update process.
    {
        etymon::odbc_conn conn(&odbc, opt.db);

        bool detect_foreign_keys = false;
        bool force_foreign_key_constraints = false;
        bool enable_foreign_key_warnings = false;

        select_config_general(&conn, &lg, &detect_foreign_keys,
               &force_foreign_key_constraints, &enable_foreign_key_warnings);

        // Always clear suggested_foreign_keys, even if foreign key detection
        // is disabled.
        string sql = "DELETE FROM dbsystem.suggested_foreign_keys;";
        lg.detail(sql);
        conn.exec(sql);

        if (detect_foreign_keys) {

            lg.write(log_level::debug, "server", "",
                    "starting foreign key detection", -1);

            timer ref_timer;

            etymon::odbc_tx tx(&conn);

            map<string, vector<reference>> refs;
            for (auto& table : schema.tables)
                search_table_foreign_keys(&odbc, opt.db, &conn, &lg, schema,
                        table, detect_foreign_keys, &refs);

            for (pair<string, vector<reference>> p : refs) {
                bool enable = (p.second.size() == 1);
                for (auto& r : p.second) {
                    sql =
                        "INSERT INTO dbsystem.suggested_foreign_keys\n"
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

            lg.write(log_level::debug, "server", "",
                    "completed foreign key detection",
                    ref_timer.elapsed_time());
        }

        if (enable_foreign_key_warnings || force_foreign_key_constraints) {

            lg.write(log_level::debug, "server", "",
                    "starting foreign key constraint processing", -1);

            timer ref_timer;

            process_foreign_keys(opt, enable_foreign_key_warnings, force_foreign_key_constraints, &conn, &lg);

            lg.write(log_level::debug, "server", "",
                    "completed foreign key constraint processing",
                    ref_timer.elapsed_time());
        }

    }

}

//void run_update(const ldp_options& opt)
//{
//    for (auto& source : opt.enable_sources) {
//        update_from_source(opt, source);
//    }
//}

void run_update_process(const ldp_options& opt)
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
        ldp_log lg(&log_conn, opt.lg_level, opt.console, opt.quiet);
        lg.write(log_level::error, "server", "", s, -1);
        exit(1);
    }
}
