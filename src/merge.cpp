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

    dropTablePair(opt, "cancellation_reasons", db);
    dropTablePair(opt, "fixed_due_date_schedules", db);
    dropTablePair(opt, "loan_policies", db);
    dropTablePair(opt, "loans", db);
    dropTablePair(opt, "loan_history", db);
    dropTablePair(opt, "patron_action_sessions", db);
    dropTablePair(opt, "patron_notice_policies", db);
    dropTablePair(opt, "request_policies", db);
    dropTablePair(opt, "request_preference", db);
    dropTablePair(opt, "requests", db);
    dropTablePair(opt, "scheduled_notices", db);
    dropTablePair(opt, "staff_slips", db);
    dropTablePair(opt, "budgets", db);
    dropTablePair(opt, "fiscal_years", db);
    dropTablePair(opt, "fund_distributions", db);
    dropTablePair(opt, "fund_types", db);
    dropTablePair(opt, "funds", db);
    dropTablePair(opt, "ledger_fiscal_years", db);
    dropTablePair(opt, "ledgers", db);
    dropTablePair(opt, "transactions", db);
    dropTablePair(opt, "alternative_title_types", db);
    dropTablePair(opt, "call_number_types", db);
    dropTablePair(opt, "classification_types", db);
    dropTablePair(opt, "contributor_name_types", db);
    dropTablePair(opt, "contributor_types", db);
    dropTablePair(opt, "electronic_access_relationships", db);
    dropTablePair(opt, "holdings_note_types", db);
    dropTablePair(opt, "holdings", db);
    dropTablePair(opt, "holdings_types", db);
    dropTablePair(opt, "identifier_types", db);
    dropTablePair(opt, "ill_policies", db);
    dropTablePair(opt, "instance_formats", db);
    dropTablePair(opt, "instance_note_types", db);
    dropTablePair(opt, "instance_relationship_types", db);
    dropTablePair(opt, "instance_statuses", db);
    dropTablePair(opt, "instance_relationships", db);
    dropTablePair(opt, "instances", db);
    dropTablePair(opt, "instance_types", db);
    dropTablePair(opt, "item_damaged_statuses", db);
    dropTablePair(opt, "item_note_types", db);
    dropTablePair(opt, "items", db);
    dropTablePair(opt, "campuses", db);
    dropTablePair(opt, "institutions", db);
    dropTablePair(opt, "libraries", db);
    dropTablePair(opt, "loan_types", db);
    dropTablePair(opt, "locations", db);
    dropTablePair(opt, "material_types", db);
    dropTablePair(opt, "modes_of_issuance", db);
    dropTablePair(opt, "nature_of_content_terms", db);
    dropTablePair(opt, "service_points", db);
    dropTablePair(opt, "service_points_users", db);
    dropTablePair(opt, "statistical_code_types", db);
    dropTablePair(opt, "statistical_codes", db);
    dropTablePair(opt, "invoices", db);
    dropTablePair(opt, "voucher_lines", db);
    dropTablePair(opt, "vouchers", db);
    dropTablePair(opt, "memberships", db);
    dropTablePair(opt, "units", db);
    dropTablePair(opt, "alerts", db);
    dropTablePair(opt, "order_invoice_relns", db);
    dropTablePair(opt, "order_templates", db);
    dropTablePair(opt, "pieces", db);
    dropTablePair(opt, "purchase_orders", db);
    dropTablePair(opt, "receiving_history", db);
    dropTablePair(opt, "reporting_codes", db);
    dropTablePair(opt, "addresses", db);
    dropTablePair(opt, "categories", db);
    dropTablePair(opt, "contacts", db);
    dropTablePair(opt, "emails", db);
    dropTablePair(opt, "interfaces", db);
    dropTablePair(opt, "organizations", db);
    dropTablePair(opt, "phone_numbers", db);
    dropTablePair(opt, "urls", db);
    dropTablePair(opt, "addresstypes", db);
    dropTablePair(opt, "groups", db);
    dropTablePair(opt, "proxiesfor", db);
    dropTablePair(opt, "users", db);
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
