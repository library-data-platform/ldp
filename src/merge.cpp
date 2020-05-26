#include <stdexcept>

#include "merge.h"
#include "names.h"
#include "util.h"

void mergeTable(const Options& opt, Log* log, const TableSchema& table,
        etymon::OdbcEnv* odbc, etymon::OdbcDbc* dbc, const DBType& dbt)
{
    // Update history tables.

    string historyTable;
    historyTableName(table.tableName, &historyTable);

    string latestHistoryTable;
    latestHistoryTableName(table.tableName, &latestHistoryTable);

    string sql =
        "CREATE TEMPORARY TABLE\n"
        "    " + latestHistoryTable + "\n"
        "    AS\n"
        "SELECT sk, id, data, tenant_id\n"
        "    FROM " + historyTable + " AS h1\n"
        "    WHERE NOT EXISTS\n"
        "      ( SELECT 1\n"
        "            FROM " + historyTable + " AS h2\n"
        "            WHERE h1.tenant_id = h2.tenant_id AND\n"
        "                  h1.sk = h2.sk AND\n"
        "                  h1.updated < h2.updated\n"
        "      );";
    log->log(Level::detail, "", "", sql, -1);
    dbc->execDirect(nullptr, sql);

    string loadingTable;
    loadingTableName(table.tableName, &loadingTable);

    sql =
        "INSERT INTO " + historyTable + "\n"
        "    (sk, id, data, updated, tenant_id)\n"
        "SELECT s.sk,\n"
        "       s.id,\n"
        "       s.data,\n" +
        "       " + dbt.currentTimestamp() + ",\n"
        "       s.tenant_id\n"
        "    FROM " + loadingTable + " AS s\n"
        "        LEFT JOIN " + latestHistoryTable + "\n"
        "            AS h\n"
        "            ON s.tenant_id = h.tenant_id AND\n"
        "               s.sk = h.sk\n"
        "    WHERE s.data IS NOT NULL AND\n"
        "          ( h.sk IS NULL OR\n"
        "            (s.data)::VARCHAR <> (h.data)::VARCHAR );";
    log->log(Level::detail, "", "", sql, -1);
    dbc->execDirect(nullptr, sql);
}

void dropTable(const Options& opt, Log* log, const string& tableName,
        etymon::OdbcDbc* dbc)
{
    string sql = "DROP TABLE IF EXISTS " + tableName + ";";
    log->logDetail(sql);
    dbc->execDirect(nullptr, sql);
}

void placeTable(const Options& opt, Log* log, const TableSchema& table,
        etymon::OdbcDbc* dbc)
{
    string loadingTable;
    loadingTableName(table.tableName, &loadingTable);
    string sql =
        "ALTER TABLE " + loadingTable + "\n"
        "    RENAME TO " + table.tableName + ";";
    log->log(Level::detail, "", "", sql, -1);
    dbc->execDirect(nullptr, sql);
}

