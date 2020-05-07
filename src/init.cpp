#include <stdexcept>

#include "dbtype.h"
#include "init.h"
#include "log.h"

/**
 * \brief Looks up the schema version number in the LDP database.
 *
 * \param[in] db Database context.
 * \param[out] version The retrieved version number.
 * \retval true The version number was retrieved.
 * \retval false The version number was not present in the database.
 */
bool selectSchemaVersion(DBContext* db, int64_t* version)
{
    string sql = "SELECT ldp_schema_version FROM ldpsystem.main;";
    db->log->log(Level::detail, "", "", sql, -1);
    etymon::OdbcStmt stmt(db->dbc);
    try {
        db->dbc->execDirect(&stmt, sql);
    } catch (runtime_error& e) {
        // This could happen if the table does not exist.
        return false;
    }
    if (db->dbc->fetch(&stmt) == false) {
        // This means there are no rows.  Do not try to recover
        // automatically from this problem.
        string e = "No rows could be read from table: ldpsystem.main";
        db->log->log(Level::error, "", "", e, -1);
        throw runtime_error(e);
    }
    string ldpSchemaVersion;
    db->dbc->getData(&stmt, 1, &ldpSchemaVersion);
    if (db->dbc->fetch(&stmt)) {
        // This means there is more than one row.  Do not try to
        // recover automatically from this problem.
        string e = "Too many rows in table: ldpsystem.main";
        db->log->log(Level::error, "", "", e, -1);
        throw runtime_error(e);
    }
    *version = stol(ldpSchemaVersion);
    return true;
}

/**
 * \brief Initializes a new database with the LDP schema.
 *
 * This function assumes that the database is empty, or at least
 * contains no tables etc. that would conflict with the new schema
 * that is to be created.  In case of conflicts, this function will
 * throw an exception rather than continue by making assumptions about
 * the state or version of the database schema.
 *
 * \param[in] db Database context.
 */
void initSchema(DBContext* db)
{
    string sql = "CREATE SCHEMA ldpsystem;";
    db->log->log(Level::detail, "", "", sql, -1);
    db->dbc->execDirect(nullptr, sql);

    sql =
        "CREATE TABLE ldpsystem.main (\n"
        "    ldp_schema_version BIGINT NOT NULL\n"
        ");";
    db->log->log(Level::detail, "", "", sql, -1);
    db->dbc->execDirect(nullptr, sql);
    sql = "DELETE FROM ldpsystem.main;";  // Temporary: pre-LDP-1.0
    db->log->log(Level::detail, "", "", sql, -1);
    db->dbc->execDirect(nullptr, sql);
    sql = "INSERT INTO ldpsystem.main (ldp_schema_version) VALUES (0);";
    db->log->log(Level::detail, "", "", sql, -1);
    db->dbc->execDirect(nullptr, sql);

    sql =
        "CREATE TABLE ldpsystem.log (\n"
        "    log_time TIMESTAMPTZ NOT NULL,\n"
        "    pid BIGINT NOT NULL,\n"
        "    level VARCHAR(6) NOT NULL,\n"
        "    type VARCHAR(63) NOT NULL,\n"
        "    table_name VARCHAR(63) NOT NULL,\n"
        "    message VARCHAR(65535) NOT NULL,\n"
        "    elapsed_time REAL\n"
        ");";
    db->dbc->execDirect(nullptr, sql);
    db->log->log(Level::detail, "", "", sql, -1);

    sql = "CREATE SCHEMA ldpconfig;";
    db->log->log(Level::detail, "", "", sql, -1);
    db->dbc->execDirect(nullptr, sql);

    sql =
        "CREATE TABLE ldpconfig.general (\n"
        "    full_update_enabled BOOLEAN NOT NULL,\n"
        "    next_full_update TIMESTAMPTZ NOT NULL\n"
        ");";
    db->dbc->execDirect(nullptr, sql);
    db->log->log(Level::detail, "", "", sql, -1);
    sql = "DELETE FROM ldpconfig.general;";  // Temporary: pre-LDP-1.0
    db->log->log(Level::detail, "", "", sql, -1);
    db->dbc->execDirect(nullptr, sql);
    sql =
        "INSERT INTO ldpconfig.general\n"
        "    (full_update_enabled, next_full_update)\n"
        "    VALUES\n"
        "    (TRUE, " + string(db->dbt->currentTimestamp()) + ");";
    db->log->log(Level::detail, "", "", sql, -1);
    db->dbc->execDirect(nullptr, sql);

    sql = "CREATE SCHEMA history;";
    db->log->log(Level::detail, "", "", sql, -1);
    db->dbc->execDirect(nullptr, sql);

    sql = "CREATE SCHEMA local;";
    db->log->log(Level::detail, "", "", sql, -1);
    db->dbc->execDirect(nullptr, sql);
}

typedef void (*SchemaUpgrade)(etymon::OdbcDbc* dbc, Log* log);

void catalogAddTable(etymon::OdbcDbc* dbc, Log* log, const string& table)
{
    string sql =
        "INSERT INTO ldpsystem.tables (table_name) VALUES (" + table + ");";
    log->logSQL(sql);
    dbc->execDirect(nullptr, sql);
}

void schemaUpgrade1(etymon::OdbcDbc* dbc, Log* log)
{
    DBType dbt(dbc);

    // Create table catalog.

    string sql =
        "CREATE TABLE ldpsystem.tables (\n"
        "    table_name VARCHAR(63) NOT NULL\n"
        ");";
    log->logSQL(sql);
    dbc->execDirect(nullptr, sql);

    const char *table[] = {
        "circulation_cancellation_reasons",
        "circulation_fixed_due_date_schedules",
        "circulation_loan_policies",
        "circulation_loans",
        "circulation_loan_history",
        "circulation_patron_action_sessions",
        "circulation_patron_notice_policies",
        "circulation_request_policies",
        "circulation_requests",
        "circulation_scheduled_notices",
        "circulation_staff_slips",
        "feesfines_accounts",
        "feesfines_comments",
        "feesfines_feefines",
        "feesfines_feefineactions",
        "feesfines_lost_item_fees_policies",
        "feesfines_manualblocks",
        "feesfines_overdue_fines_policies",
        "feesfines_owners",
        "feesfines_payments",
        "feesfines_refunds",
        "feesfines_transfer_criterias",
        "feesfines_transfers",
        "feesfines_waives",
        "course_courselistings",
        "course_roles",
        "course_terms",
        "course_coursetypes",
        "course_departments",
        "course_processingstatuses",
        "course_copyrightstatuses",
        "course_courses",
        "course_reserves",
        "finance_budgets",
        "finance_fiscal_years",
        "finance_fund_types",
        "finance_funds",
        "finance_group_fund_fiscal_years",
        "finance_groups",
        "finance_ledgers",
        "finance_transactions",
        "inventory_alternative_title_types",
        "inventory_call_number_types",
        "inventory_classification_types",
        "inventory_contributor_name_types",
        "inventory_contributor_types",
        "inventory_electronic_access_relationships",
        "inventory_holdings_note_types",
        "inventory_holdings",
        "inventory_holdings_types",
        "inventory_identifier_types",
        "inventory_ill_policies",
        "inventory_instance_formats",
        "inventory_instance_note_types",
        "inventory_instance_relationship_types",
        "inventory_instance_statuses",
        "inventory_instance_relationships",
        "inventory_instances",
        "inventory_instance_types",
        "inventory_item_damaged_statuses",
        "inventory_item_note_types",
        "inventory_items",
        "inventory_campuses",
        "inventory_institutions",
        "inventory_libraries",
        "inventory_loan_types",
        "inventory_locations",
        "inventory_material_types",
        "inventory_modes_of_issuance",
        "inventory_nature_of_content_terms",
        "inventory_service_points",
        "inventory_service_points_users",
        "inventory_statistical_code_types",
        "inventory_statistical_codes",
        "invoice_lines",
        "invoice_invoices",
        "invoice_voucher_lines",
        "invoice_vouchers",
        "acquisitions_memberships",
        "acquisitions_units",
        "po_alerts",
        "po_order_invoice_relns",
        "po_order_templates",
        "po_pieces",
        "po_lines",
        "po_purchase_orders",
        "po_receiving_history",
        "po_reporting_codes",
        "organization_addresses",
        "organization_categories",
        "organization_contacts",
        "organization_emails",
        "organization_interfaces",
        "organization_organizations",
        "organization_phone_numbers",
        "organization_urls",
        "user_groups",
        "user_users",
        nullptr
    };
    for (int x = 0; table[x] != nullptr; x++) {
        // Add table to the catalog.
        catalogAddTable(dbc, log, table[x]);
        // Create stub tables if they don't exist.
        sql =
            "CREATE TABLE IF NOT EXISTS " + string(table[x]) + " (\n"
            "    sk BIGINT NOT NULL,\n"
            "    id VARCHAR(65535) NOT NULL,\n"
            "    data " + dbt.jsonType() + ",\n"
            "    tenant_id SMALLINT NOT NULL\n"
            ");";
        log->logSQL(sql);
        dbc->execDirect(nullptr, sql);
        sql =
            "CREATE TABLE IF NOT EXISTS history." + string(table[x]) + " (\n"
            "    sk BIGINT NOT NULL,\n"
            "    id VARCHAR(65535) NOT NULL,\n"
            "    data " + dbt.jsonType() + ",\n"
            "    updated TIMESTAMPTZ NOT NULL,\n"
            "    tenant_id SMALLINT NOT NULL\n"
            ");";
        log->logSQL(sql);
        dbc->execDirect(nullptr, sql);
    }

    // Create idmap table.

    string rskeys;
    dbt.redshiftKeys("sk", "sk", &rskeys);
    string autoInc;
    dbt.autoIncrementType(1, false, "", &autoInc);
    sql =
        "CREATE TABLE ldpsystem.idmap (\n"
        "    sk " + autoInc + ",\n"
        "    id VARCHAR(65535),\n"
        "        PRIMARY KEY (sk),\n"
        "        UNIQUE (id)\n"
        ")" + rskeys + ";";
    dbc->execDirect(nullptr, sql);
    log->log(Level::detail, "", "", sql, -1);

}

SchemaUpgrade schemaUpgrade[] = {
    nullptr,  // Version 0 has no migration.
    schemaUpgrade1
};

void upgradeSchema(etymon::OdbcEnv* odbc, const string& dsn, int64_t version,
        Log* log)
{
    int64_t latestVersion = 1;

    if (version < 0 || version > latestVersion)
        throw runtime_error(
                "Unknown LDP schema version: " + to_string(version));

    etymon::OdbcDbc dbc(odbc, dsn);
    etymon::OdbcTx tx(&dbc);

    for (int v = version + 1; v <= latestVersion; v++)
        schemaUpgrade[v](&dbc, log);

    string sql = "UPDATE ldpsystem.main SET ldp_schema_version = " +
        to_string(latestVersion) + ";";
    log->log(Level::detail, "", "", sql, -1);
    dbc.execDirect(nullptr, sql);

    tx.commit();
}

/**
 * \brief Initializes or upgrades an LDP database if needed.
 *
 * This function checks if the database has been previously
 * initialized with an LDP schema.  If not, it creates the schema.  If
 * the schema exists but its version number is not current, this
 * function attempts to upgrade the schema to the current version.
 *
 * If an unexpected state in the database is detected, this function
 * will throw an exception rather than continue by making assumptions
 * about the state or version of the database schema.  However, it
 * does not perform any thorough validation of the database, and it
 * assumes that the schema has not been altered or corrupted.
 *
 * \param[in] odbc ODBC environment.
 * \param[in] db Database context.
 */
void initUpgrade(etymon::OdbcEnv* odbc, const string& dsn, DBContext* db)
{
    db->log->log(Level::trace, "", "", "Initializing database", -1);

    int64_t version;
    bool versionFound = selectSchemaVersion(db, &version);
    db->log->log(Level::detail, "", "", "ldp_schema_version: " +
            (versionFound ? to_string(version) : "(not found)"),
            -1);

    if (versionFound) {
        // Schema is present: check if it needs to be upgraded.
        upgradeSchema(odbc, dsn, version, db->log);
    } else {
        // Schema is not present: create it.
        db->log->log(Level::detail, "", "", "Creating schema", -1);
        {
            etymon::OdbcTx tx(db->dbc);
            initSchema(db);
            tx.commit();
            fprintf(stderr, "ldp: Logging enabled in table: ldpsystem.log\n");
        }
    }
}

