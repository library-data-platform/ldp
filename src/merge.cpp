#include "merge.h"
#include "names.h"
#include "util.h"

void create_latest_history_table(const ldp_options& opt, ldp_log* lg,
                                 const table_schema& table,
                                 etymon::pgconn* conn)
{
    if (table.source_type == data_source_type::srs_marc_records) {
        return;
    }
    if (table.source_type == data_source_type::srs_error_records) {
        return;
    }

    string history_table;
    history_table_name(table.name, &history_table);

    string latest_history_table;
    latest_history_table_name(table.name, &latest_history_table);

    string sql =
        "CREATE TEMP TABLE\n"
        "    " + latest_history_table + "\n"
        "    AS\n"
        "SELECT id, data\n"
        "    FROM " + history_table + " AS h1\n"
        "    WHERE NOT EXISTS\n"
        "      ( SELECT 1\n"
        "            FROM " + history_table + " AS h2\n"
        "            WHERE h1.id = h2.id AND\n"
        "                  h1.updated < h2.updated\n"
        "      );";
    lg->write(log_level::detail, "", "", sql, -1);
    { etymon::pgconn_result r(conn, sql); }

    sql = "VACUUM ANALYZE " + latest_history_table + ";";
    lg->detail(sql);
    { etymon::pgconn_result r(conn, sql); }
}

void drop_latest_history_table(const ldp_options& opt, ldp_log* lg,
                               const table_schema& table,
                               etymon::pgconn* conn)
{
    if (table.source_type == data_source_type::srs_marc_records) {
        return;
    }
    if (table.source_type == data_source_type::srs_error_records) {
        return;
    }

    string latest_history_table;
    latest_history_table_name(table.name, &latest_history_table);

    string sql = "DROP TABLE IF EXISTS " + latest_history_table + ";";
    lg->detail(sql);
    { etymon::pgconn_result r(conn, sql); }
}

void merge_table(const ldp_options& opt, ldp_log* lg, const table_schema& table, etymon::pgconn* conn, const dbtype& dbt)
{
    // Update history tables.

    string history_table;
    history_table_name(table.name, &history_table);

    string latest_history_table;
    latest_history_table_name(table.name, &latest_history_table);

    string loading_table;
    loading_table_name(table.name, &loading_table);

    string sql =
        "INSERT INTO " + history_table + "\n"
        "    (id, data, updated)\n"
        "SELECT s.id,\n"
        "       s.data,\n" +
        "       " + dbt.current_timestamp() + "\n"
        "    FROM " + loading_table + " AS s\n"
        "        LEFT JOIN " + latest_history_table + "\n"
        "            AS h\n"
        "            ON s.id = h.id\n"
        "    WHERE s.data IS NOT NULL AND\n"
        "          ( h.id IS NULL OR\n"
        "            (s.data)::varchar <> (h.data)::varchar );";
    lg->write(log_level::detail, "", "", sql, -1);
    { etymon::pgconn_result r(conn, sql); }
}

void drop_table(const ldp_options& opt, ldp_log* lg, const string& tableName,
        etymon::pgconn* conn)
{
    string sql = "DROP TABLE IF EXISTS " + tableName + ";";
    lg->detail(sql);
    { etymon::pgconn_result r(conn, sql); }
}

void place_table(const ldp_options& opt, ldp_log* lg, const table_schema& table,
        etymon::pgconn* conn)
{
    string loading_table;
    loading_table_name(table.name, &loading_table);
    string sql =
        "ALTER TABLE " + loading_table + "\n"
        "    RENAME TO " + table.name + ";";
    lg->write(log_level::detail, "", "", sql, -1);
    { etymon::pgconn_result r(conn, sql); }
}

