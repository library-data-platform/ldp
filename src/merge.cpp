#include "merge.h"
#include "names.h"
#include "util.h"

void mergeTable(const Options& opt, Log* log, const TableSchema& table,
        etymon::OdbcDbc* dbc, const DBType& dbt)
{
    // Update history tables.

    string historyTable;
    historyTableName(table.tableName, &historyTable);

    // Temporarily drop history tables before recreating them.
    dropTable(opt, log, historyTable, dbc);

    string rskeys;
    dbt.redshiftKeys("id", "id, updated", &rskeys);
    string autoInc;
    dbt.autoIncrementType(1, false, "", &autoInc);
    string sql =
        "CREATE TABLE IF NOT EXISTS\n"
        "    " + historyTable + " (\n"
        "    row_id " + autoInc + ",\n"
        "    id VARCHAR(65535) NOT NULL,\n"
        "    data " + dbt.jsonType() + " NOT NULL,\n"
        "    updated TIMESTAMPTZ NOT NULL,\n"
        //"    updated " + string(dbt.timestamp0()) + " NOT NULL,\n"
        "    tenant_id SMALLINT NOT NULL,\n"
        "    PRIMARY KEY (row_id),\n"
        "    CONSTRAINT\n"
        "        history_" + table.tableName + "_id_updated_key\n"
        "        UNIQUE (id, updated)\n"
        ")" + rskeys + ";";
    log->log(Level::detail, "", "", sql, -1);
    dbc->execDirect(nullptr, sql);

    string latestHistoryTable;
    latestHistoryTableName(table.tableName, &latestHistoryTable);

    sql =
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
    log->log(Level::detail, "", "", sql, -1);
    dbc->execDirect(nullptr, sql);

    string loadingTable;
    loadingTableName(table.tableName, &loadingTable);

    sql =
        "INSERT INTO " + historyTable + "\n"
        "    (id, data, updated, tenant_id)\n"
        "SELECT s.id,\n"
        "       s.data,\n" +
        "       " + dbt.currentTimestamp() + ",\n"
        "       s.tenant_id\n"
        "    FROM " + loadingTable + " AS s\n"
        "        LEFT JOIN " + latestHistoryTable + "\n"
        "            AS h\n"
        "            ON s.tenant_id = h.tenant_id AND\n"
        "               s.id = h.id\n"
        "    WHERE s.data IS NOT NULL AND\n"
        "          ( h.id IS NULL OR\n"
        "            (s.data)::VARCHAR <> (h.data)::VARCHAR );\n";
    log->log(Level::detail, "", "", sql, -1);
    dbc->execDirect(nullptr, sql);
}

void dropTable(const Options& opt, Log* log, const string& tableName,
        etymon::OdbcDbc* dbc)
{
    string sql = "DROP TABLE IF EXISTS " + tableName + ";";
    log->log(Level::detail, "", "", sql, -1);
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

//void updateStatus(const Options& opt, const TableSchema& table,
//        etymon::OdbcDbc* dbc)
//{
//    string sql =
//        "DELETE FROM ldp_catalog.table_updates WHERE table_name = '" +
//        table.tableName + "' AND tenant_id = 1;";
//    printSQL(Print::debug, opt, sql);
//    dbc->execDirect(nullptr, sql);

//    sql =
//        "INSERT INTO ldp_catalog.table_updates\n"
//        "    (table_name, updated, tenant_id)\n"
//        "    VALUES ('" +
//        table.tableName + "', " + dbt.currentTimestamp() + ", 1);";
//    printSQL(Print::debug, opt, sql);
//    dbc->execDirect(nullptr, sql);
//}

static void dropTablePair(const Options& opt, Log* log, const string& tableName,
        etymon::OdbcDbc* dbc)
{
    dropTable(opt, log, tableName, dbc);
    dropTable(opt, log, "history." + tableName, dbc);
}

void dropOldTables(const Options& opt, Log* log, etymon::OdbcDbc* dbc)
{
    dropTablePair(opt, log, "circulation_request_preference", dbc);
    dropTablePair(opt, log, "finance_ledger_fiscal_years", dbc);
    dropTablePair(opt, log, "user_addresstypes", dbc);
    dropTablePair(opt, log, "user_proxiesfor", dbc);
    dropTablePair(opt, log, "finance_fund_distributions", dbc);

    dropTablePair(opt, log, "order_lines", dbc);
    dropTablePair(opt, log, "orders", dbc);
    dropTablePair(opt, log, "invoice_number", dbc);
    dropTablePair(opt, log, "voucher_number", dbc);
    dropTablePair(opt, log, "po_number", dbc);
    dropTablePair(opt, log, "finance_group_budgets", dbc);
    dropTablePair(opt, log, "shelf_locations", dbc);

    dropTablePair(opt, log, "cancellation_reasons", dbc);
    dropTablePair(opt, log, "fixed_due_date_schedules", dbc);
    dropTablePair(opt, log, "loan_policies", dbc);
    dropTablePair(opt, log, "loans", dbc);
    dropTablePair(opt, log, "loan_history", dbc);
    dropTablePair(opt, log, "patron_action_sessions", dbc);
    dropTablePair(opt, log, "patron_notice_policies", dbc);
    dropTablePair(opt, log, "request_policies", dbc);
    dropTablePair(opt, log, "request_preference", dbc);
    dropTablePair(opt, log, "requests", dbc);
    dropTablePair(opt, log, "scheduled_notices", dbc);
    dropTablePair(opt, log, "staff_slips", dbc);
    dropTablePair(opt, log, "budgets", dbc);
    dropTablePair(opt, log, "fiscal_years", dbc);
    dropTablePair(opt, log, "fund_distributions", dbc);
    dropTablePair(opt, log, "fund_types", dbc);
    dropTablePair(opt, log, "funds", dbc);
    dropTablePair(opt, log, "ledger_fiscal_years", dbc);
    dropTablePair(opt, log, "ledgers", dbc);
    dropTablePair(opt, log, "transactions", dbc);
    dropTablePair(opt, log, "alternative_title_types", dbc);
    dropTablePair(opt, log, "call_number_types", dbc);
    dropTablePair(opt, log, "classification_types", dbc);
    dropTablePair(opt, log, "contributor_name_types", dbc);
    dropTablePair(opt, log, "contributor_types", dbc);
    dropTablePair(opt, log, "electronic_access_relationships", dbc);
    dropTablePair(opt, log, "holdings_note_types", dbc);
    dropTablePair(opt, log, "holdings", dbc);
    dropTablePair(opt, log, "holdings_types", dbc);
    dropTablePair(opt, log, "identifier_types", dbc);
    dropTablePair(opt, log, "ill_policies", dbc);
    dropTablePair(opt, log, "instance_formats", dbc);
    dropTablePair(opt, log, "instance_note_types", dbc);
    dropTablePair(opt, log, "instance_relationship_types", dbc);
    dropTablePair(opt, log, "instance_statuses", dbc);
    dropTablePair(opt, log, "instance_relationships", dbc);
    dropTablePair(opt, log, "instances", dbc);
    dropTablePair(opt, log, "instance_types", dbc);
    dropTablePair(opt, log, "item_damaged_statuses", dbc);
    dropTablePair(opt, log, "item_note_types", dbc);
    dropTablePair(opt, log, "items", dbc);
    dropTablePair(opt, log, "campuses", dbc);
    dropTablePair(opt, log, "institutions", dbc);
    dropTablePair(opt, log, "libraries", dbc);
    dropTablePair(opt, log, "loan_types", dbc);
    dropTablePair(opt, log, "locations", dbc);
    dropTablePair(opt, log, "material_types", dbc);
    dropTablePair(opt, log, "modes_of_issuance", dbc);
    dropTablePair(opt, log, "nature_of_content_terms", dbc);
    dropTablePair(opt, log, "service_points", dbc);
    dropTablePair(opt, log, "service_points_users", dbc);
    dropTablePair(opt, log, "statistical_code_types", dbc);
    dropTablePair(opt, log, "statistical_codes", dbc);
    dropTablePair(opt, log, "invoices", dbc);
    dropTablePair(opt, log, "voucher_lines", dbc);
    dropTablePair(opt, log, "vouchers", dbc);
    dropTablePair(opt, log, "memberships", dbc);
    dropTablePair(opt, log, "units", dbc);
    dropTablePair(opt, log, "alerts", dbc);
    dropTablePair(opt, log, "order_invoice_relns", dbc);
    dropTablePair(opt, log, "order_templates", dbc);
    dropTablePair(opt, log, "pieces", dbc);
    dropTablePair(opt, log, "purchase_orders", dbc);
    dropTablePair(opt, log, "receiving_history", dbc);
    dropTablePair(opt, log, "reporting_codes", dbc);
    dropTablePair(opt, log, "addresses", dbc);
    dropTablePair(opt, log, "categories", dbc);
    dropTablePair(opt, log, "contacts", dbc);
    dropTablePair(opt, log, "emails", dbc);
    dropTablePair(opt, log, "interfaces", dbc);
    dropTablePair(opt, log, "organizations", dbc);
    dropTablePair(opt, log, "phone_numbers", dbc);
    dropTablePair(opt, log, "urls", dbc);
    dropTablePair(opt, log, "addresstypes", dbc);
    dropTablePair(opt, log, "groups", dbc);
    dropTablePair(opt, log, "proxiesfor", dbc);
    dropTablePair(opt, log, "users", dbc);

    dropTablePair(opt, log, "testing_source_records", dbc);
    dropTablePair(opt, log, "srs_source_records", dbc);
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

