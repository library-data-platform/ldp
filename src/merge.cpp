#include "merge.h"
#include "names.h"
#include "util.h"

void mergeTable(const Options& opt, const TableSchema& table,
        etymon::OdbcDbc* dbc, const DBType& dbt)
{
    // Update history tables.

    string historyTable;
    historyTableName(table.tableName, &historyTable);

    string rskeys;
    dbt.redshiftKeys("id", "id, updated", &rskeys);
    string sql =
        "CREATE TABLE IF NOT EXISTS " + historyTable + " (\n"
        "    id VARCHAR(65535) NOT NULL,\n"
        "    data " + dbt.jsonType() + " NOT NULL,\n"
        "    updated TIMESTAMPTZ NOT NULL,\n"
        "    tenant_id SMALLINT NOT NULL,\n"
        "    CONSTRAINT ldp_history_" + table.tableName + "_pkey\n"
        "        PRIMARY KEY (id, updated)\n"
        ")" + rskeys + ";";
    printSQL(Print::debug, opt, sql);
    dbc->execDirect(sql);

    // Temporary: recreate primary key.
    sql =
        "ALTER TABLE " + historyTable + "\n"
        "    DROP CONSTRAINT ldp_history_" + table.tableName + "_pkey;\n";
    printSQL(Print::debug, opt, sql);
    dbc->execDirect(sql);
    sql =
        "ALTER TABLE " + historyTable + "\n"
        "    ADD CONSTRAINT ldp_history_" + table.tableName + "_pkey\n"
        "        PRIMARY KEY (id, updated);";
    printSQL(Print::debug, opt, sql);
    dbc->execDirect(sql);

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
    dbc->execDirect(sql);

    string loadingTable;
    loadingTableName(table.tableName, &loadingTable);

    sql =
        "INSERT INTO " + historyTable + "\n"
        "SELECT s.id, s.data, " + dbt.currentTimestamp() + ", s.tenant_id\n"
        "    FROM " + loadingTable + " AS s\n"
        "        LEFT JOIN " + latestHistoryTable + " AS h\n"
        "            ON s.tenant_id = h.tenant_id AND\n"
        "               s.id = h.id\n"
        "    WHERE s.data IS NOT NULL AND\n"
        "          ( h.id IS NULL OR\n"
        "            (s.data)::VARCHAR <> (h.data)::VARCHAR );\n";
    printSQL(Print::debug, opt, sql);
    dbc->execDirect(sql);
}

void dropTable(const Options& opt, const string& tableName,
        etymon::OdbcDbc* dbc)
{
    string sql = "DROP TABLE IF EXISTS " + tableName + ";";
    printSQL(Print::debug, opt, sql);
    dbc->execDirect(sql);
}

void placeTable(const Options& opt, const TableSchema& table,
        etymon::OdbcDbc* dbc)
{
    string loadingTable;
    loadingTableName(table.tableName, &loadingTable);
    string sql =
        "ALTER TABLE " + loadingTable + " RENAME TO " + table.tableName + ";";
    printSQL(Print::debug, opt, sql);
    dbc->execDirect(sql);
}

//void updateStatus(const Options& opt, const TableSchema& table,
//        etymon::OdbcDbc* dbc)
//{
//    string sql =
//        "DELETE FROM ldp_catalog.table_updates WHERE table_name = '" +
//        table.tableName + "' AND tenant_id = 1;";
//    printSQL(Print::debug, opt, sql);
//    dbc->execDirect(sql);

//    sql =
//        "INSERT INTO ldp_catalog.table_updates\n"
//        "    (table_name, updated, tenant_id)\n"
//        "    VALUES ('" +
//        table.tableName + "', " + dbt.currentTimestamp() + ", 1);";
//    printSQL(Print::debug, opt, sql);
//    dbc->execDirect(sql);
//}

static void dropTablePair(const Options& opt, const string& tableName,
        etymon::OdbcDbc* dbc)
{
    dropTable(opt, tableName, dbc);
    dropTable(opt, "history." + tableName, dbc);
}

void dropOldTables(const Options& opt, etymon::OdbcDbc* dbc)
{
    dropTable(opt, "ldp_catalog.table_updates", dbc);

    dropTablePair(opt, "circulation_request_preference", dbc);
    dropTablePair(opt, "finance_ledger_fiscal_years", dbc);
    dropTablePair(opt, "user_addresstypes", dbc);
    dropTablePair(opt, "user_proxiesfor", dbc);
    dropTablePair(opt, "finance_fund_distributions", dbc);

    dropTablePair(opt, "order_lines", dbc);
    dropTablePair(opt, "orders", dbc);
    dropTablePair(opt, "invoice_number", dbc);
    dropTablePair(opt, "voucher_number", dbc);
    dropTablePair(opt, "po_number", dbc);
    dropTablePair(opt, "finance_group_budgets", dbc);
    dropTablePair(opt, "shelf_locations", dbc);

    dropTablePair(opt, "cancellation_reasons", dbc);
    dropTablePair(opt, "fixed_due_date_schedules", dbc);
    dropTablePair(opt, "loan_policies", dbc);
    dropTablePair(opt, "loans", dbc);
    dropTablePair(opt, "loan_history", dbc);
    dropTablePair(opt, "patron_action_sessions", dbc);
    dropTablePair(opt, "patron_notice_policies", dbc);
    dropTablePair(opt, "request_policies", dbc);
    dropTablePair(opt, "request_preference", dbc);
    dropTablePair(opt, "requests", dbc);
    dropTablePair(opt, "scheduled_notices", dbc);
    dropTablePair(opt, "staff_slips", dbc);
    dropTablePair(opt, "budgets", dbc);
    dropTablePair(opt, "fiscal_years", dbc);
    dropTablePair(opt, "fund_distributions", dbc);
    dropTablePair(opt, "fund_types", dbc);
    dropTablePair(opt, "funds", dbc);
    dropTablePair(opt, "ledger_fiscal_years", dbc);
    dropTablePair(opt, "ledgers", dbc);
    dropTablePair(opt, "transactions", dbc);
    dropTablePair(opt, "alternative_title_types", dbc);
    dropTablePair(opt, "call_number_types", dbc);
    dropTablePair(opt, "classification_types", dbc);
    dropTablePair(opt, "contributor_name_types", dbc);
    dropTablePair(opt, "contributor_types", dbc);
    dropTablePair(opt, "electronic_access_relationships", dbc);
    dropTablePair(opt, "holdings_note_types", dbc);
    dropTablePair(opt, "holdings", dbc);
    dropTablePair(opt, "holdings_types", dbc);
    dropTablePair(opt, "identifier_types", dbc);
    dropTablePair(opt, "ill_policies", dbc);
    dropTablePair(opt, "instance_formats", dbc);
    dropTablePair(opt, "instance_note_types", dbc);
    dropTablePair(opt, "instance_relationship_types", dbc);
    dropTablePair(opt, "instance_statuses", dbc);
    dropTablePair(opt, "instance_relationships", dbc);
    dropTablePair(opt, "instances", dbc);
    dropTablePair(opt, "instance_types", dbc);
    dropTablePair(opt, "item_damaged_statuses", dbc);
    dropTablePair(opt, "item_note_types", dbc);
    dropTablePair(opt, "items", dbc);
    dropTablePair(opt, "campuses", dbc);
    dropTablePair(opt, "institutions", dbc);
    dropTablePair(opt, "libraries", dbc);
    dropTablePair(opt, "loan_types", dbc);
    dropTablePair(opt, "locations", dbc);
    dropTablePair(opt, "material_types", dbc);
    dropTablePair(opt, "modes_of_issuance", dbc);
    dropTablePair(opt, "nature_of_content_terms", dbc);
    dropTablePair(opt, "service_points", dbc);
    dropTablePair(opt, "service_points_users", dbc);
    dropTablePair(opt, "statistical_code_types", dbc);
    dropTablePair(opt, "statistical_codes", dbc);
    dropTablePair(opt, "invoices", dbc);
    dropTablePair(opt, "voucher_lines", dbc);
    dropTablePair(opt, "vouchers", dbc);
    dropTablePair(opt, "memberships", dbc);
    dropTablePair(opt, "units", dbc);
    dropTablePair(opt, "alerts", dbc);
    dropTablePair(opt, "order_invoice_relns", dbc);
    dropTablePair(opt, "order_templates", dbc);
    dropTablePair(opt, "pieces", dbc);
    dropTablePair(opt, "purchase_orders", dbc);
    dropTablePair(opt, "receiving_history", dbc);
    dropTablePair(opt, "reporting_codes", dbc);
    dropTablePair(opt, "addresses", dbc);
    dropTablePair(opt, "categories", dbc);
    dropTablePair(opt, "contacts", dbc);
    dropTablePair(opt, "emails", dbc);
    dropTablePair(opt, "interfaces", dbc);
    dropTablePair(opt, "organizations", dbc);
    dropTablePair(opt, "phone_numbers", dbc);
    dropTablePair(opt, "urls", dbc);
    dropTablePair(opt, "addresstypes", dbc);
    dropTablePair(opt, "groups", dbc);
    dropTablePair(opt, "proxiesfor", dbc);
    dropTablePair(opt, "users", dbc);
}

//void mergeAll(const Options& opt, Schema* schema, etymon::Postgres* db)
//{
//    print(Print::verbose, opt, "merging");
//    for (auto& table : schema->tables) {
//        if (table.skip)
//            continue;
//        print(Print::verbose, opt, "merging table: " + table.tableName);
//        mergeTable(opt, table, db);
//    }
//    // Table-level exclusive locks begin here.
//    print(Print::verbose, opt, "replacing tables");
//    for (auto& table : schema->tables) {
//        if (table.skip)
//            continue;
//        dropTable(opt, table.tableName, db);
//        placeTable(opt, table, db);
//        updateStatus(opt, table, db);
//    }
//    // printSchema(stdout, *schema);
//    dropOldTables(opt, db);
//}

