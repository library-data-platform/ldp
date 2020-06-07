#include <stdexcept>

#include "merge.h"
#include "names.h"
#include "util.h"

void mergeTable(const ldp_options& opt, ldp_log* lg, const table_schema& table,
        etymon::odbc_env* odbc, etymon::odbc_conn* conn, const dbtype& dbt)
{
    // Update history tables.

    string historyTable;
    historyTableName(table.name, &historyTable);

    string latestHistoryTable;
    latestHistoryTableName(table.name, &latestHistoryTable);

    string sql =
        "CREATE TEMPORARY TABLE\n"
        "    " + latestHistoryTable + "\n"
        "    AS\n"
        "SELECT id, data, tenant_id\n"
        "    FROM " + historyTable + " AS h1\n"
        "    WHERE NOT EXISTS\n"
        "      ( SELECT 1\n"
        "            FROM " + historyTable + " AS h2\n"
        "            WHERE h1.tenant_id = h2.tenant_id AND\n"
        "                  h1.id = h2.id AND\n"
        "                  h1.updated < h2.updated\n"
        "      );";
    lg->write(log_level::detail, "", "", sql, -1);
    conn->exec(sql);

    string loadingTable;
    loadingTableName(table.name, &loadingTable);

    sql =
        "INSERT INTO " + historyTable + "\n"
        "    (id, data, updated, tenant_id)\n"
        "SELECT s.id,\n"
        "       s.data,\n" +
        "       " + dbt.current_timestamp() + ",\n"
        "       s.tenant_id\n"
        "    FROM " + loadingTable + " AS s\n"
        "        LEFT JOIN " + latestHistoryTable + "\n"
        "            AS h\n"
        "            ON s.tenant_id = h.tenant_id AND\n"
        "               s.id = h.id\n"
        "    WHERE s.data IS NOT NULL AND\n"
        "          ( h.id IS NULL OR\n"
        "            (s.data)::VARCHAR <> (h.data)::VARCHAR );";
    lg->write(log_level::detail, "", "", sql, -1);
    conn->exec(sql);
}

void dropTable(const ldp_options& opt, ldp_log* lg, const string& tableName,
        etymon::odbc_conn* conn)
{
    string sql = "DROP TABLE IF EXISTS " + tableName + ";";
    lg->detail(sql);
    conn->exec(sql);
}

void placeTable(const ldp_options& opt, ldp_log* lg, const table_schema& table,
        etymon::odbc_conn* conn)
{
    string loadingTable;
    loadingTableName(table.name, &loadingTable);
    string sql =
        "ALTER TABLE " + loadingTable + "\n"
        "    RENAME TO " + table.name + ";";
    lg->write(log_level::detail, "", "", sql, -1);
    conn->exec(sql);
}

