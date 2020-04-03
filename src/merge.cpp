#include "merge.h"
#include "names.h"
#include "util.h"

static void mergeTable(const Options& opt, const TableSchema& table,
        etymon::Postgres* db)
{
    // Update history tables.

    string historyTable;
    historyTableName(table.tableName, &historyTable);

    string rskeys;
    opt.dbtype.redshiftKeys("id", "id, updated", &rskeys);
    string sql =
        "CREATE TABLE IF NOT EXISTS " + historyTable + " (\n"
        "    id VARCHAR(65535) NOT NULL,\n"
        "    data " + opt.dbtype.jsonType() + " NOT NULL,\n"
        "    updated TIMESTAMPTZ NOT NULL,\n"
        "    tenant_id SMALLINT NOT NULL,\n"
        "    CONSTRAINT ldp_history_" + table.tableName + "_pkey\n"
        "        PRIMARY KEY (id, updated)\n"
        ")" + rskeys + ";";
    printSQL(Print::debug, opt, sql);
    { etymon::PostgresResult result(db, sql); }

    // Temporary: recreate primary key.
    sql =
        "ALTER TABLE " + historyTable + "\n"
        "    DROP CONSTRAINT ldp_history_" + table.tableName + "_pkey;\n";
    printSQL(Print::debug, opt, sql);
    { etymon::PostgresResult result(db, sql); }
    sql =
        "ALTER TABLE " + historyTable + "\n"
        "    ADD CONSTRAINT ldp_history_" + table.tableName + "_pkey\n"
        "        PRIMARY KEY (id, updated);";
    printSQL(Print::debug, opt, sql);
    { etymon::PostgresResult result(db, sql); }

    string latestHistoryTable;
    latestHistoryTableName(table.tableName, &latestHistoryTable);

    sql =
        "CREATE TEMPORARY TABLE " + latestHistoryTable + " AS\n"
        "SELECT id, data, tenant_id\n"
        "    FROM " + historyTable + " AS h1\n"
        "    WHERE NOT EXISTS\n"
        "      ( SELECT 1\n"
        "            FROM " + historyTable + " AS h2\n"
        "            WHERE h1.tenant_id = h2.tenant_id AND\n"
        "                  h1.id = h2.id AND\n"
        "                  h1.updated < h2.updated\n"
        "      );";
    printSQL(Print::debug, opt, sql);
    { etymon::PostgresResult result(db, sql); }

    string loadingTable;
    loadingTableName(table.tableName, &loadingTable);

    sql =
        "INSERT INTO " + historyTable + "\n"
        "SELECT s.id, s.data, 'now', s.tenant_id\n"
        "    FROM " + loadingTable + " AS s\n"
        "        LEFT JOIN " + latestHistoryTable + " AS h\n"
        "            ON s.tenant_id = h.tenant_id AND\n"
        "               s.id = h.id\n"
        "    WHERE s.data IS NOT NULL AND\n"
        "          ( h.id IS NULL OR\n"
        "            (s.data)::VARCHAR <> (h.data)::VARCHAR );\n";
    printSQL(Print::debug, opt, sql);
    { etymon::PostgresResult result(db, sql); }
}

static void dropTable(const Options& opt, const string& tableName,
        etymon::Postgres* db)
{
    string sql = "DROP TABLE IF EXISTS " + tableName + ";";
    printSQL(Print::debug, opt, sql);
    { etymon::PostgresResult result(db, sql); }
}

static void placeTable(const Options& opt, const TableSchema& table,
        etymon::Postgres* db)
{
    string loadingTable;
    loadingTableName(table.tableName, &loadingTable);
    string sql =
        "ALTER TABLE " + loadingTable + " RENAME TO " + table.tableName + ";";
    printSQL(Print::debug, opt, sql);
    { etymon::PostgresResult result(db, sql); }
}

static void updateStatus(const Options& opt, const TableSchema& table,
        etymon::Postgres* db)
{
    string sql =
        "DELETE FROM ldp_catalog.table_updates WHERE table_name = '" +
        table.tableName + "' AND tenant_id = 1;";
    printSQL(Print::debug, opt, sql);
    { etymon::PostgresResult result(db, sql); }

    sql =
        "INSERT INTO ldp_catalog.table_updates\n"
        "    (table_name, updated, tenant_id)\n"
        "    VALUES ('" + table.tableName + "', 'now', 1);";
    printSQL(Print::debug, opt, sql);
    { etymon::PostgresResult result(db, sql); }
}

static void dropTablePair(const Options& opt, const string& tableName,
        etymon::Postgres* db)
{
    dropTable(opt, tableName, db);
    dropTable(opt, "history." + tableName, db);
}

static void dropOldTables(const Options& opt, etymon::Postgres* db)
{
    dropTablePair(opt, "order_lines", db);
    dropTablePair(opt, "orders", db);
    dropTablePair(opt, "invoice_number", db);
    dropTablePair(opt, "voucher_number", db);
    dropTablePair(opt, "po_number", db);
    dropTablePair(opt, "shelf_locations", db);
}

void mergeAll(const Options& opt, Schema* schema, etymon::Postgres* db)
{
    print(Print::verbose, opt, "merging");
    for (auto& table : schema->tables) {
        if (table.skip)
            continue;
        print(Print::verbose, opt, "merging table: " + table.tableName);
        mergeTable(opt, table, db);
    }
    // Table-level exclusive locks begin here.
    print(Print::verbose, opt, "replacing tables");
    for (auto& table : schema->tables) {
        if (table.skip)
            continue;
        dropTable(opt, table.tableName, db);
        placeTable(opt, table, db);
        updateStatus(opt, table, db);
    }
    // printSchema(stdout, *schema);

    dropOldTables(opt, db);
}
