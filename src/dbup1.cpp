#include <stdexcept>

#include "dbup1.h"
#include "initutil.h"
#include "schema.h"

void ulog_sql(const string& sql, database_upgrade_options* opt)
{
    fprintf(opt->ulog, "%s\n", sql.c_str());
    fflush(opt->ulog);
}

void ulog_commit(database_upgrade_options* opt)
{
    fprintf(opt->ulog, "-- Committed\n");
    fflush(opt->ulog);
}

void add_table_to_catalog_sql_ldpsystem(etymon::pgconn* conn,
                                        const string& table,
                                        string* sql)
{
    *sql =
        "INSERT INTO ldpsystem.tables (table_name) VALUES\n"
        "    ('" + table + "');";
}

void add_table_to_catalog_sql_dbsystem(etymon::pgconn* conn,
                                       const string& table,
                                       string* sql)
{
    *sql =
        "INSERT INTO dbsystem.tables (table_name) VALUES\n"
        "    ('" + table + "');";
}

void upgrade_add_new_table_ldpsystem(const string& table,
                                     database_upgrade_options* opt,
                                     const dbtype& dbt)
{
    string sql;

    create_main_table_sql(table, opt->conn, dbt, &sql);
    ulog_sql(sql, opt);
    { etymon::pgconn_result r(opt->conn, sql); }
    ulog_commit(opt);

    grant_select_on_table_sql(table, opt->ldp_user, opt->conn, &sql);
    ulog_sql(sql, opt);
    { etymon::pgconn_result r(opt->conn, sql); }
    ulog_commit(opt);

    grant_select_on_table_sql(table, opt->ldpconfig_user, opt->conn, &sql);
    ulog_sql(sql, opt);
    { etymon::pgconn_result r(opt->conn, sql); }
    ulog_commit(opt);

    create_history_table_sql(table, opt->conn, dbt, &sql);
    ulog_sql(sql, opt);
    { etymon::pgconn_result r(opt->conn, sql); }
    ulog_commit(opt);

    grant_select_on_table_sql("history." + table, opt->ldp_user, opt->conn,
                              &sql);
    ulog_sql(sql, opt);
    { etymon::pgconn_result r(opt->conn, sql); }
    ulog_commit(opt);

    grant_select_on_table_sql("history." + table, opt->ldpconfig_user,
                              opt->conn, &sql);
    ulog_sql(sql, opt);
    { etymon::pgconn_result r(opt->conn, sql); }
    ulog_commit(opt);

    add_table_to_catalog_sql_ldpsystem(opt->conn, table, &sql);
    ulog_sql(sql, opt);
    { etymon::pgconn_result r(opt->conn, sql); }
    ulog_commit(opt);
}

void upgrade_add_new_table_dbsystem(const string& table,
                                    database_upgrade_options* opt,
                                    const dbtype& dbt,
                                    bool autocommit)
{
    string sql;

    create_main_table_sql(table, opt->conn, dbt, &sql);
    ulog_sql(sql, opt);
    { etymon::pgconn_result r(opt->conn, sql); }
    if (autocommit)
        ulog_commit(opt);

    grant_select_on_table_sql(table, opt->ldp_user, opt->conn, &sql);
    ulog_sql(sql, opt);
    { etymon::pgconn_result r(opt->conn, sql); }
    if (autocommit)
        ulog_commit(opt);

    grant_select_on_table_sql(table, opt->ldpconfig_user, opt->conn, &sql);
    ulog_sql(sql, opt);
    { etymon::pgconn_result r(opt->conn, sql); }
    if (autocommit)
        ulog_commit(opt);

    create_history_table_sql(table, opt->conn, dbt, &sql);
    ulog_sql(sql, opt);
    { etymon::pgconn_result r(opt->conn, sql); }
    if (autocommit)
        ulog_commit(opt);

    grant_select_on_table_sql("history." + table, opt->ldp_user, opt->conn,
                              &sql);
    ulog_sql(sql, opt);
    { etymon::pgconn_result r(opt->conn, sql); }
    if (autocommit)
        ulog_commit(opt);

    grant_select_on_table_sql("history." + table, opt->ldpconfig_user,
                              opt->conn, &sql);
    ulog_sql(sql, opt);
    { etymon::pgconn_result r(opt->conn, sql); }
    if (autocommit)
        ulog_commit(opt);

    add_table_to_catalog_sql_dbsystem(opt->conn, table, &sql);
    ulog_sql(sql, opt);
    { etymon::pgconn_result r(opt->conn, sql); }
    if (autocommit)
        ulog_commit(opt);
}

void database_upgrade_1(database_upgrade_options* opt)
{
    dbtype dbt(opt->conn);

    // Create table catalog.

    string sql =
        "CREATE TABLE ldpsystem.tables (\n"
        "    table_name VARCHAR(63) NOT NULL\n"
        ");";
    { etymon::pgconn_result r(opt->conn, sql); }

    sql = "GRANT SELECT ON ldpsystem.tables TO " + opt->ldp_user + ";";
    { etymon::pgconn_result r(opt->conn, sql); }

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
        add_table_to_catalog_sql_ldpsystem(opt->conn, table[x], &sql);
        { etymon::pgconn_result r(opt->conn, sql); }
        // Create stub table if it doesn't exist.
        sql =
            "CREATE TABLE IF NOT EXISTS " + string(table[x]) + " (\n"
            "    row_id BIGINT NOT NULL,\n"
            "    sk BIGINT NOT NULL,\n"
            "    id VARCHAR(65535) NOT NULL,\n"
            "    data " + dbt.json_type() + ",\n"
            "    tenant_id SMALLINT NOT NULL\n"
            ");";
        { etymon::pgconn_result r(opt->conn, sql); }
        sql =
            "GRANT SELECT ON\n"
            "    " + string(table[x]) + "\n"
            "    TO " + opt->ldp_user + ";";
        { etymon::pgconn_result r(opt->conn, sql); }
        // Recreate history table.
        sql = "DROP TABLE IF EXISTS\n"
            "    history." + string(table[x]) + ";";
        { etymon::pgconn_result r(opt->conn, sql); }
        string rskeys;
        dbt.redshift_keys("sk", "sk, updated", &rskeys);
        string sql =
            "CREATE TABLE IF NOT EXISTS\n"
            "    history." + string(table[x]) + " (\n"
            "    sk BIGINT NOT NULL,\n"
            "    id VARCHAR(65535) NOT NULL,\n"
            "    data " + dbt.json_type() + " NOT NULL,\n"
            "    updated TIMESTAMP WITH TIME ZONE NOT NULL,\n"
            "    tenant_id SMALLINT NOT NULL,\n"
            "    CONSTRAINT\n"
            "        history_" + table[x] + "_pkey\n"
            "        PRIMARY KEY (sk),\n"
            "    CONSTRAINT\n"
            "        history_" + table[x] + "_id_updated_key\n"
            "        UNIQUE (id, updated)\n"
            ")" + rskeys + ";";
        { etymon::pgconn_result r(opt->conn, sql); }
        sql =
            "GRANT SELECT ON\n"
            "    history." + string(table[x]) + "\n"
            "    TO " + opt->ldp_user + ";";
        { etymon::pgconn_result r(opt->conn, sql); }
        if (string(dbt.type_string()) == "PostgreSQL") {
            // Remove row_id columns.
            sql =
                "ALTER TABLE \n"
                "    " + string(table[x]) + "\n"
                "    DROP COLUMN row_id;";
            { etymon::pgconn_result r(opt->conn, sql); }
        }
    }

    // Create idmap table.

    string rskeys;
    dbt.redshift_keys("sk", "sk", &rskeys);
    string autoInc;
    dbt.auto_increment_type(1, false, "", &autoInc);
    sql =
        "CREATE TABLE ldpsystem.idmap (\n"
        "    sk " + autoInc + ",\n"
        "    id VARCHAR(65535),\n"
        "        PRIMARY KEY (sk),\n"
        "        UNIQUE (id)\n"
        ")" + rskeys + ";";
    { etymon::pgconn_result r(opt->conn, sql); }

    sql = "GRANT SELECT ON ldpsystem.idmap TO " + opt->ldp_user + ";";
    { etymon::pgconn_result r(opt->conn, sql); }

    sql = "UPDATE ldpsystem.main SET ldp_schema_version = 1;";
    fprintf(opt->ulog, "%s\n", sql.c_str());
    fflush(opt->ulog);
    { etymon::pgconn_result r(opt->conn, sql); }
    fprintf(opt->ulog, "-- Committed\n");
    fflush(opt->ulog);
}

void database_upgrade_2(database_upgrade_options* opt)
{
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
        string sql =
            "ALTER TABLE history." + string(table[x]) + "\n"
            "    DROP CONSTRAINT history_" + table[x] + "_pkey;";
        { etymon::pgconn_result r(opt->conn, sql); }
        sql =
            "ALTER TABLE history." + string(table[x]) + "\n"
            "    ADD CONSTRAINT history_" + table[x] + "_pkey\n"
            "    PRIMARY KEY (sk, updated);";
        { etymon::pgconn_result r(opt->conn, sql); }
    }

    string sql = "UPDATE ldpsystem.main SET ldp_schema_version = 2;";
    fprintf(opt->ulog, "%s\n", sql.c_str());
    fflush(opt->ulog);
    { etymon::pgconn_result r(opt->conn, sql); }
    fprintf(opt->ulog, "-- Committed\n");
    fflush(opt->ulog);
}

void database_upgrade_3(database_upgrade_options* opt)
{
    string sql =
        "ALTER TABLE ldpconfig.general\n"
        "    ADD COLUMN log_referential_analysis\n"
        "        BOOLEAN NOT NULL DEFAULT FALSE;";
    { etymon::pgconn_result r(opt->conn, sql); }
    sql =
        "ALTER TABLE ldpconfig.general\n"
        "    ADD COLUMN force_referential_constraints\n"
        "        BOOLEAN NOT NULL DEFAULT FALSE;";
    { etymon::pgconn_result r(opt->conn, sql); }

    sql = "UPDATE ldpsystem.main SET ldp_schema_version = 3;";
    fprintf(opt->ulog, "%s\n", sql.c_str());
    fflush(opt->ulog);
    { etymon::pgconn_result r(opt->conn, sql); }
    fprintf(opt->ulog, "-- Committed\n");
    fflush(opt->ulog);
}

void database_upgrade_4(database_upgrade_options* opt)
{
    dbtype dbt(opt->conn);

    string sql = "ALTER TABLE ldpsystem.log DROP COLUMN level;";
    { etymon::pgconn_result r(opt->conn, sql); }

    sql =
        "ALTER TABLE ldpsystem.log\n"
        "    ADD COLUMN level\n"
        "        VARCHAR(7) NOT NULL DEFAULT '';";
    { etymon::pgconn_result r(opt->conn, sql); }

    sql =
        "ALTER TABLE ldpsystem.idmap\n"
        "    ADD COLUMN table_name\n"
        "        VARCHAR(63) NOT NULL DEFAULT '';";
    { etymon::pgconn_result r(opt->conn, sql); }

    string rskeys;
    dbt.redshift_keys("referencing_table",
            "referencing_table, referencing_column", &rskeys);
    sql =
        "CREATE TABLE ldpsystem.referential_constraints (\n"
        "    referencing_table VARCHAR(63) NOT NULL,\n"
        "    referencing_column VARCHAR(63) NOT NULL,\n"
        "    referenced_table VARCHAR(63) NOT NULL,\n"
        "    referenced_column VARCHAR(63) NOT NULL,\n"
        "        PRIMARY KEY (referencing_table, referencing_column)\n"
        ")" + rskeys + ";";
    { etymon::pgconn_result r(opt->conn, sql); }

    sql = "GRANT SELECT ON ALL TABLES IN SCHEMA ldpsystem TO " + opt->ldp_user +
        ";";
    { etymon::pgconn_result r(opt->conn, sql); }

    sql = "UPDATE ldpsystem.main SET ldp_schema_version = 4;";
    fprintf(opt->ulog, "%s\n", sql.c_str());
    fflush(opt->ulog);
    { etymon::pgconn_result r(opt->conn, sql); }
    fprintf(opt->ulog, "-- Committed\n");
    fflush(opt->ulog);
}

void database_upgrade_5(database_upgrade_options* opt)
{
    dbtype dbt(opt->conn);

    string sql = "ALTER TABLE ldpsystem.idmap DROP CONSTRAINT idmap_pkey;";
    { etymon::pgconn_result r(opt->conn, sql); }

    sql = "ALTER TABLE ldpsystem.idmap DROP CONSTRAINT idmap_id_key;";
    { etymon::pgconn_result r(opt->conn, sql); }

    sql = "ALTER TABLE ldpsystem.idmap RENAME TO idmap_old;";
    { etymon::pgconn_result r(opt->conn, sql); }

    string rskeys;
    dbt.redshift_keys("sk", "sk", &rskeys);
    sql =
        "CREATE TABLE ldpsystem.idmap (\n"
        "    sk BIGINT NOT NULL,\n"
        "    id VARCHAR(65535) NOT NULL,\n"
        "        PRIMARY KEY (sk),\n"
        "        UNIQUE (id)\n"
        ")" + rskeys + ";";
    { etymon::pgconn_result r(opt->conn, sql); }

    sql =
        "INSERT INTO ldpsystem.idmap (sk, id)\n"
        "    SELECT sk, id FROM ldpsystem.idmap_old;";
    { etymon::pgconn_result r(opt->conn, sql); }

    sql = "DROP TABLE ldpsystem.idmap_old;";
    { etymon::pgconn_result r(opt->conn, sql); }

    sql = "UPDATE ldpsystem.main SET ldp_schema_version = 5;";
    fprintf(opt->ulog, "%s\n", sql.c_str());
    fflush(opt->ulog);
    { etymon::pgconn_result r(opt->conn, sql); }
    fprintf(opt->ulog, "-- Committed\n");
    fflush(opt->ulog);
}

void database_upgrade_6(database_upgrade_options* opt)
{
    string sql = "GRANT USAGE ON SCHEMA ldpsystem TO " + opt->ldpconfig_user +
        ";";
    { etymon::pgconn_result r(opt->conn, sql); }

    sql = "GRANT SELECT ON ldpsystem.idmap TO " + opt->ldp_user + ";";
    { etymon::pgconn_result r(opt->conn, sql); }
    sql = "GRANT SELECT ON ldpsystem.idmap TO " + opt->ldpconfig_user + ";";
    { etymon::pgconn_result r(opt->conn, sql); }

    sql = "GRANT SELECT ON ldpsystem.log TO " + opt->ldp_user + ";";
    { etymon::pgconn_result r(opt->conn, sql); }
    sql = "GRANT SELECT ON ldpsystem.log TO " + opt->ldpconfig_user + ";";
    { etymon::pgconn_result r(opt->conn, sql); }

    sql = "GRANT SELECT ON ldpsystem.main TO " + opt->ldp_user + ";";
    { etymon::pgconn_result r(opt->conn, sql); }
    sql = "GRANT SELECT ON ldpsystem.main TO " + opt->ldpconfig_user + ";";
    { etymon::pgconn_result r(opt->conn, sql); }

    sql = "GRANT SELECT ON ldpsystem.referential_constraints TO " +
        opt->ldp_user + ";";
    { etymon::pgconn_result r(opt->conn, sql); }
    sql = "GRANT SELECT ON ldpsystem.referential_constraints TO " +
        opt->ldpconfig_user + ";";
    { etymon::pgconn_result r(opt->conn, sql); }

    sql = "GRANT SELECT ON ldpsystem.tables TO " + opt->ldp_user + ";";
    { etymon::pgconn_result r(opt->conn, sql); }
    sql = "GRANT SELECT ON ldpsystem.tables TO " + opt->ldpconfig_user + ";";
    { etymon::pgconn_result r(opt->conn, sql); }

    sql = "GRANT USAGE ON SCHEMA ldpconfig TO " + opt->ldpconfig_user + ";";
    { etymon::pgconn_result r(opt->conn, sql); }

    sql = "GRANT SELECT ON ALL TABLES IN SCHEMA ldpconfig TO " +
        opt->ldpconfig_user + ";";
    { etymon::pgconn_result r(opt->conn, sql); }

    sql = "GRANT UPDATE ON ldpconfig.general TO " + opt->ldpconfig_user + ";";
    { etymon::pgconn_result r(opt->conn, sql); }

    sql = "GRANT USAGE ON SCHEMA history TO " + opt->ldpconfig_user + ";";
    { etymon::pgconn_result r(opt->conn, sql); }

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
        string sql =
            "GRANT SELECT ON\n"
            "    history." + string(table[x]) + "\n"
            "    TO " + opt->ldpconfig_user + ";";
        { etymon::pgconn_result r(opt->conn, sql); }
    }

    sql = "GRANT USAGE ON SCHEMA local TO " + opt->ldpconfig_user + ";";
    { etymon::pgconn_result r(opt->conn, sql); }

    sql = "UPDATE ldpsystem.main SET ldp_schema_version = 6;";
    fprintf(opt->ulog, "%s\n", sql.c_str());
    fflush(opt->ulog);
    { etymon::pgconn_result r(opt->conn, sql); }
    fprintf(opt->ulog, "-- Committed\n");
    fflush(opt->ulog);
}

void database_upgrade_7(database_upgrade_options* opt)
{
    string sql =
        "ALTER TABLE ldpsystem.tables\n"
        "    ADD COLUMN updated TIMESTAMP WITH TIME ZONE;";
    { etymon::pgconn_result r(opt->conn, sql); }

    sql =
        "ALTER TABLE ldpsystem.tables\n"
        "    ADD COLUMN row_count BIGINT;";
    { etymon::pgconn_result r(opt->conn, sql); }

    sql =
        "ALTER TABLE ldpsystem.tables\n"
        "    ADD COLUMN history_row_count BIGINT;";
    { etymon::pgconn_result r(opt->conn, sql); }

    sql =
        "ALTER TABLE ldpsystem.tables\n"
        "    ADD COLUMN documentation VARCHAR(65535);";
    { etymon::pgconn_result r(opt->conn, sql); }

    sql =
        "ALTER TABLE ldpsystem.tables\n"
        "    ADD COLUMN documentation_url VARCHAR(65535);";
    { etymon::pgconn_result r(opt->conn, sql); }

    sql =
        "ALTER TABLE ldpconfig.general\n"
        "    ADD COLUMN disable_anonymization BOOLEAN NOT NULL DEFAULT FALSE;";
    { etymon::pgconn_result r(opt->conn, sql); }

    sql = "UPDATE ldpsystem.main SET ldp_schema_version = 7;";
    fprintf(opt->ulog, "%s\n", sql.c_str());
    fflush(opt->ulog);
    { etymon::pgconn_result r(opt->conn, sql); }
    fprintf(opt->ulog, "-- Committed\n");
    fflush(opt->ulog);
}

void database_upgrade_8(database_upgrade_options* opt)
{
    string sql = "UPDATE ldpsystem.main SET ldp_schema_version = 8;";
    fprintf(opt->ulog, "%s\n", sql.c_str());
    fflush(opt->ulog);
    { etymon::pgconn_result r(opt->conn, sql); }
    fprintf(opt->ulog, "-- Committed\n");
    fflush(opt->ulog);
}

void database_upgrade_9(database_upgrade_options* opt)
{
    dbtype dbt(opt->conn);

    string sql = "DROP TABLE ldpsystem.referential_constraints;";
    { etymon::pgconn_result r(opt->conn, sql); }

    string rskeys;
    dbt.redshift_keys("referencing_table",
            "referencing_table, referencing_column", &rskeys);
    sql =
        "CREATE TABLE ldpsystem.foreign_key_constraints (\n"
        "    referencing_table VARCHAR(63) NOT NULL,\n"
        "    referencing_column VARCHAR(63) NOT NULL,\n"
        "    referenced_table VARCHAR(63) NOT NULL,\n"
        "    referenced_column VARCHAR(63) NOT NULL,\n"
        "    constraint_name VARCHAR(63) NOT NULL,\n"
        "        PRIMARY KEY (referencing_table, referencing_column)\n"
        ")" + rskeys + ";";
    { etymon::pgconn_result r(opt->conn, sql); }

    sql =
        "ALTER TABLE ldpconfig.general\n"
        "    RENAME COLUMN full_update_enabled TO enable_full_updates;";
    { etymon::pgconn_result r(opt->conn, sql); }

    sql =
        "ALTER TABLE ldpconfig.general\n"
        "    RENAME COLUMN log_referential_analysis TO detect_foreign_keys;";
    { etymon::pgconn_result r(opt->conn, sql); }

    sql =
        "ALTER TABLE ldpconfig.general\n"
        "    DROP COLUMN force_referential_constraints;";
    { etymon::pgconn_result r(opt->conn, sql); }

    sql =
        "ALTER TABLE ldpconfig.general\n"
        "    ADD COLUMN force_foreign_key_constraints\n"
        "    BOOLEAN NOT NULL DEFAULT FALSE;";
    { etymon::pgconn_result r(opt->conn, sql); }

    sql =
        "ALTER TABLE ldpconfig.general\n"
        "    ADD COLUMN enable_foreign_key_warnings\n"
        "    BOOLEAN NOT NULL DEFAULT FALSE;";
    { etymon::pgconn_result r(opt->conn, sql); }

    sql =
        "CREATE TABLE ldpconfig.foreign_keys (\n"
        "    enable_constraint BOOLEAN NOT NULL,\n"
        "    referencing_table VARCHAR(63) NOT NULL,\n"
        "    referencing_column VARCHAR(63) NOT NULL,\n"
        "    referenced_table VARCHAR(63) NOT NULL,\n"
        "    referenced_column VARCHAR(63) NOT NULL\n"
        ");";
    { etymon::pgconn_result r(opt->conn, sql); }

    sql = "UPDATE ldpsystem.main SET ldp_schema_version = 9;";
    fprintf(opt->ulog, "%s\n", sql.c_str());
    fflush(opt->ulog);
    { etymon::pgconn_result r(opt->conn, sql); }
    fprintf(opt->ulog, "-- Committed\n");
    fflush(opt->ulog);
}

void database_upgrade_10(database_upgrade_options* opt)
{
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
        string sql =
            "ALTER TABLE history." + string(table[x]) + "\n"
            "    DROP CONSTRAINT history_" + table[x] + "_id_updated_key;";
        fprintf(opt->ulog, "%s\n", sql.c_str());
        fflush(opt->ulog);
        { etymon::pgconn_result r(opt->conn, sql); }
        fprintf(opt->ulog, "-- Committed\n");
        fflush(opt->ulog);
    }

    for (int x = 0; table[x] != nullptr; x++) {
        string sql =
            "ALTER TABLE history." + string(table[x]) + "\n"
            "    ALTER COLUMN id TYPE VARCHAR(36);";
        fprintf(opt->ulog, "%s\n", sql.c_str());
        fflush(opt->ulog);
        { etymon::pgconn_result r(opt->conn, sql); }
        fprintf(opt->ulog, "-- Committed\n");
        fflush(opt->ulog);
    }

    for (int x = 0; table[x] != nullptr; x++) {
        string sql =
            "ALTER TABLE history." + string(table[x]) + "\n"
            "    ADD CONSTRAINT history_" + table[x] + "_id_updated_key\n"
            "    UNIQUE (id, updated);";
        fprintf(opt->ulog, "%s\n", sql.c_str());
        fflush(opt->ulog);
        { etymon::pgconn_result r(opt->conn, sql); }
        fprintf(opt->ulog, "-- Committed\n");
        fflush(opt->ulog);
    }

    string sql = "UPDATE ldpsystem.main SET ldp_schema_version = 10;";
    fprintf(opt->ulog, "%s\n", sql.c_str());
    fflush(opt->ulog);
    { etymon::pgconn_result r(opt->conn, sql); }
    fprintf(opt->ulog, "-- Committed\n");
    fflush(opt->ulog);
}

void database_upgrade_11(database_upgrade_options* opt)
{
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

    dbtype dbt(opt->conn);

    string sql = "DROP TABLE ldpsystem.idmap;";
    fprintf(opt->ulog, "%s\n", sql.c_str());
    fflush(opt->ulog);
    { etymon::pgconn_result r(opt->conn, sql); }
    fprintf(opt->ulog, "-- Committed\n");
    fflush(opt->ulog);

    for (int x = 0; table[x] != nullptr; x++) {

        if (string(dbt.type_string()) == "Redshift") {

            sql =
                "ALTER TABLE history." + string(table[x]) + "\n"
                "    ALTER DISTKEY id;";
            fprintf(opt->ulog, "%s\n", sql.c_str());
            fflush(opt->ulog);
            { etymon::pgconn_result r(opt->conn, sql); }
            fprintf(opt->ulog, "-- Committed\n");
            fflush(opt->ulog);

            sql =
                "ALTER TABLE history." + string(table[x]) + "\n"
                "    ALTER COMPOUND SORTKEY (id, updated);";
            fprintf(opt->ulog, "%s\n", sql.c_str());
            fflush(opt->ulog);
            { etymon::pgconn_result r(opt->conn, sql); }
            fprintf(opt->ulog, "-- Committed\n");
            fflush(opt->ulog);
        }

        sql =
            "ALTER TABLE history." + string(table[x]) + "\n"
            "    DROP COLUMN sk CASCADE;";
        fprintf(opt->ulog, "%s\n", sql.c_str());
        fflush(opt->ulog);
        { etymon::pgconn_result r(opt->conn, sql); }
        fprintf(opt->ulog, "-- Committed\n");
        fflush(opt->ulog);

        sql =
            "ALTER TABLE history." + string(table[x]) + "\n"
            "    DROP CONSTRAINT history_" + table[x] + "_id_updated_key;";
        fprintf(opt->ulog, "%s\n", sql.c_str());
        fflush(opt->ulog);
        { etymon::pgconn_result r(opt->conn, sql); }
        fprintf(opt->ulog, "-- Committed\n");
        fflush(opt->ulog);

        sql =
            "ALTER TABLE history." + string(table[x]) + "\n"
            "    ADD CONSTRAINT history_" + table[x] + "_pkey\n"
            "    PRIMARY KEY (id, updated);";
        fprintf(opt->ulog, "%s\n", sql.c_str());
        fflush(opt->ulog);
        { etymon::pgconn_result r(opt->conn, sql); }
        fprintf(opt->ulog, "-- Committed\n");
        fflush(opt->ulog);

    }

    sql = "UPDATE ldpsystem.main SET ldp_schema_version = 11;";
    fprintf(opt->ulog, "%s\n", sql.c_str());
    fflush(opt->ulog);
    { etymon::pgconn_result r(opt->conn, sql); }
    fprintf(opt->ulog, "-- Committed\n");
    fflush(opt->ulog);
}

void database_upgrade_12(database_upgrade_options* opt)
{
    // Table: ldpconfig.general

    // Column: disable_anonymization
    // Drop default
    //string sql =
    //    "ALTER TABLE ldpconfig.general\n"
    //    "    ALTER COLUMN disable_anonymization DROP DEFAULT;";
    //fprintf(opt->ulog, "%s\n", sql.c_str());
    //fflush(opt->ulog);
    //opt->conn->exec(sql);
    //fprintf(opt->ulog, "-- Committed\n");
    //fflush(opt->ulog);

    // Column: update_all_tables
    // Add column
    string sql =
        "ALTER TABLE ldpconfig.general\n"
        "    ADD COLUMN update_all_tables\n"
        "    BOOLEAN;";
    fprintf(opt->ulog, "%s\n", sql.c_str());
    fflush(opt->ulog);
    { etymon::pgconn_result r(opt->conn, sql); }
    fprintf(opt->ulog, "-- Committed\n");
    fflush(opt->ulog);
    // Update value
    sql =
        "UPDATE ldpconfig.general\n"
        "    SET update_all_tables = TRUE;";
    fprintf(opt->ulog, "%s\n", sql.c_str());
    fflush(opt->ulog);
    { etymon::pgconn_result r(opt->conn, sql); }
    fprintf(opt->ulog, "-- Committed\n");
    fflush(opt->ulog);
    // Set not null
    //sql =
    //    "ALTER TABLE ldpconfig.general\n"
    //    "    ALTER COLUMN update_all_tables SET NOT NULL;";
    //fprintf(opt->ulog, "%s\n", sql.c_str());
    //fflush(opt->ulog);
    //opt->conn->exec(sql);
    //fprintf(opt->ulog, "-- Committed\n");
    //fflush(opt->ulog);

    // Table: ldpconfig.update_tables
    // Create table
    sql =
        "CREATE TABLE ldpconfig.update_tables (\n"
        "    enable_update BOOLEAN NOT NULL,\n"
        "    table_name VARCHAR(63) NOT NULL,\n"
        "    tenant_id SMALLINT NOT NULL\n"
        ");";
    fprintf(opt->ulog, "%s\n", sql.c_str());
    fflush(opt->ulog);
    { etymon::pgconn_result r(opt->conn, sql); }
    fprintf(opt->ulog, "-- Committed\n");
    fflush(opt->ulog);

    sql = "UPDATE ldpsystem.main SET ldp_schema_version = 12;";
    fprintf(opt->ulog, "%s\n", sql.c_str());
    fflush(opt->ulog);
    { etymon::pgconn_result r(opt->conn, sql); }
    fprintf(opt->ulog, "-- Committed\n");
    fflush(opt->ulog);
}

void database_upgrade_13(database_upgrade_options* opt)
{
    dbtype dbt(opt->conn);

    upgrade_add_new_table_ldpsystem("course_copyrightstatuses", opt, dbt);
    upgrade_add_new_table_ldpsystem("course_courselistings", opt, dbt);
    upgrade_add_new_table_ldpsystem("course_courses", opt, dbt);
    upgrade_add_new_table_ldpsystem("course_coursetypes", opt, dbt);
    upgrade_add_new_table_ldpsystem("course_departments", opt, dbt);
    upgrade_add_new_table_ldpsystem("course_processingstatuses", opt, dbt);
    upgrade_add_new_table_ldpsystem("course_reserves", opt, dbt);
    upgrade_add_new_table_ldpsystem("course_roles", opt, dbt);
    upgrade_add_new_table_ldpsystem("course_terms", opt, dbt);

    string sql = "UPDATE ldpsystem.main SET ldp_schema_version = 13;";
    ulog_sql(sql, opt);
    { etymon::pgconn_result r(opt->conn, sql); }
    ulog_commit(opt);
}

void database_upgrade_14(database_upgrade_options* opt)
{
    dbtype dbt(opt->conn);

    upgrade_add_new_table_ldpsystem("email_email", opt, dbt);

    string sql = "UPDATE ldpsystem.main SET ldp_schema_version = 14;";
    ulog_sql(sql, opt);
    { etymon::pgconn_result r(opt->conn, sql); }
    ulog_commit(opt);
}

void database_upgrade_15(database_upgrade_options* opt)
{
    dbtype dbt(opt->conn);

    if (dbt.type() == dbsys::postgresql) {
        string sql =
            "ALTER TABLE ldpconfig.general\n"
            "    ALTER COLUMN disable_anonymization SET DEFAULT FALSE;";
        fprintf(opt->ulog, "%s\n", sql.c_str());
        fflush(opt->ulog);
        { etymon::pgconn_result r(opt->conn, sql); }
        fprintf(opt->ulog, "-- Committed\n");
        fflush(opt->ulog);
    }

    if (dbt.type() == dbsys::postgresql) {
        string sql =
            "ALTER TABLE ldpconfig.general\n"
            "    ALTER COLUMN update_all_tables DROP NOT NULL;";
        fprintf(opt->ulog, "%s\n", sql.c_str());
        fflush(opt->ulog);
        { etymon::pgconn_result r(opt->conn, sql); }
        fprintf(opt->ulog, "-- Committed\n");
        fflush(opt->ulog);
    }

    string sql = "UPDATE ldpsystem.main SET ldp_schema_version = 15;";
    ulog_sql(sql, opt);
    { etymon::pgconn_result r(opt->conn, sql); }
    ulog_commit(opt);
}

void database_upgrade_16(database_upgrade_options* opt)
{
    dbtype dbt(opt->conn);

    { etymon::pgconn_result r(opt->conn, "BEGIN;"); }

    string sql =
        "ALTER TABLE ldpconfig.general\n"
        "    DROP COLUMN update_all_tables;";
    ulog_sql(sql, opt);
    { etymon::pgconn_result r(opt->conn, sql); }

    sql =
        "ALTER TABLE ldpconfig.general\n"
        "    ADD COLUMN update_all_tables BOOLEAN NOT NULL DEFAULT FALSE;";
    ulog_sql(sql, opt);
    { etymon::pgconn_result r(opt->conn, sql); }

    sql = "UPDATE ldpsystem.main SET ldp_schema_version = 16;";
    ulog_sql(sql, opt);
    { etymon::pgconn_result r(opt->conn, sql); }

    { etymon::pgconn_result r(opt->conn, "COMMIT;"); }
    ulog_commit(opt);
}

void database_upgrade_17(database_upgrade_options* opt)
{
    dbtype dbt(opt->conn);

    { etymon::pgconn_result r(opt->conn, "BEGIN;"); }

    string rskeys;
    dbt.redshift_keys("referencing_table",
            "referencing_table, referencing_column", &rskeys);
    string sql =
        "CREATE TABLE ldpsystem.suggested_foreign_keys (\n"
        "    enable_constraint BOOLEAN NOT NULL,\n"
        "    referencing_table VARCHAR(63) NOT NULL,\n"
        "    referencing_column VARCHAR(63) NOT NULL,\n"
        "    referenced_table VARCHAR(63) NOT NULL,\n"
        "    referenced_column VARCHAR(63) NOT NULL\n"
        ")" + rskeys + ";";
    ulog_sql(sql, opt);
    { etymon::pgconn_result r(opt->conn, sql); }

    sql = "UPDATE ldpsystem.main SET ldp_schema_version = 17;";
    ulog_sql(sql, opt);
    { etymon::pgconn_result r(opt->conn, sql); }

    { etymon::pgconn_result r(opt->conn, "COMMIT;"); }
    ulog_commit(opt);
}

void database_upgrade_18(database_upgrade_options* opt)
{
    dbtype dbt(opt->conn);

    { etymon::pgconn_result r(opt->conn, "BEGIN;"); }

    string sql = "ALTER SCHEMA ldpsystem RENAME TO dbsystem;";
    ulog_sql(sql, opt);
    { etymon::pgconn_result r(opt->conn, sql); }

    sql = "DROP TABLE dbsystem.server_lock;";
    ulog_sql(sql, opt);
    { etymon::pgconn_result r(opt->conn, sql); }

    sql =
        "ALTER TABLE dbsystem.main\n"
        "    RENAME COLUMN ldp_schema_version TO database_version;";
    ulog_sql(sql, opt);
    { etymon::pgconn_result r(opt->conn, sql); }

    sql = "ALTER SCHEMA ldpconfig RENAME TO dbconfig;";
    ulog_sql(sql, opt);
    { etymon::pgconn_result r(opt->conn, sql); }

    sql = "UPDATE dbsystem.main SET database_version = 18;";
    ulog_sql(sql, opt);
    { etymon::pgconn_result r(opt->conn, sql); }

    { etymon::pgconn_result r(opt->conn, "COMMIT;"); }
    ulog_commit(opt);
}

void database_upgrade_19(database_upgrade_options* opt)
{
    dbtype dbt(opt->conn);

    { etymon::pgconn_result r(opt->conn, "BEGIN;"); }

    string sql =
        "ALTER TABLE dbconfig.general\n"
        "    DROP COLUMN disable_anonymization;";
    ulog_sql(sql, opt);
    { etymon::pgconn_result r(opt->conn, sql); }

    sql =
        "ALTER TABLE dbsystem.main\n"
        "    ADD COLUMN anonymize BOOLEAN NOT NULL DEFAULT TRUE;";
    ulog_sql(sql, opt);
    { etymon::pgconn_result r(opt->conn, sql); }

    sql = "UPDATE dbsystem.main SET database_version = 19;";
    ulog_sql(sql, opt);
    { etymon::pgconn_result r(opt->conn, sql); }

    { etymon::pgconn_result r(opt->conn, "COMMIT;"); }
    ulog_commit(opt);
}

void database_upgrade_20(database_upgrade_options* opt)
{
    dbtype dbt(opt->conn);

    { etymon::pgconn_result r(opt->conn, "BEGIN;"); }

    upgrade_add_new_table_dbsystem("circulation_request_preference", opt,
                                   dbt, false);
    upgrade_add_new_table_dbsystem("finance_ledger_fiscal_years", opt, dbt, false);
    upgrade_add_new_table_dbsystem("user_addresstypes", opt, dbt, false);
    upgrade_add_new_table_dbsystem("user_departments", opt, dbt, false);
    upgrade_add_new_table_dbsystem("user_proxiesfor", opt, dbt, false);

    string sql = "UPDATE dbsystem.main SET database_version = 20;";
    ulog_sql(sql, opt);
    { etymon::pgconn_result r(opt->conn, sql); }

    { etymon::pgconn_result r(opt->conn, "COMMIT;"); }
    ulog_commit(opt);
}

void database_upgrade_21(database_upgrade_options* opt)
{
    dbtype dbt(opt->conn);

    { etymon::pgconn_result r(opt->conn, "BEGIN;"); }

    upgrade_add_new_table_dbsystem("configuration_entries", opt, dbt, false);

    string sql = "UPDATE dbsystem.main SET database_version = 21;";
    ulog_sql(sql, opt);
    { etymon::pgconn_result r(opt->conn, sql); }

    { etymon::pgconn_result r(opt->conn, "COMMIT;"); }
    ulog_commit(opt);
}

void database_upgrade_22(database_upgrade_options* opt)
{
    dbtype dbt(opt->conn);

    { etymon::pgconn_result r(opt->conn, "BEGIN;"); }

    upgrade_add_new_table_dbsystem("finance_expense_classes", opt, dbt, false);
    upgrade_add_new_table_dbsystem("notes", opt, dbt, false);

    string sql = "UPDATE dbsystem.main SET database_version = 22;";
    ulog_sql(sql, opt);
    { etymon::pgconn_result r(opt->conn, sql); }

    { etymon::pgconn_result r(opt->conn, "COMMIT;"); }
    ulog_commit(opt);
}

void database_upgrade_23(database_upgrade_options* opt)
{
    dbtype dbt(opt->conn);

    { etymon::pgconn_result r(opt->conn, "BEGIN;"); }

    upgrade_add_new_table_dbsystem("srs_marc", opt, dbt, false);

    string sql = "UPDATE dbsystem.main SET database_version = 23;";
    ulog_sql(sql, opt);
    { etymon::pgconn_result r(opt->conn, sql); }

    { etymon::pgconn_result r(opt->conn, "COMMIT;"); }
    ulog_commit(opt);
}

void database_upgrade_24(database_upgrade_options* opt)
{
    dbtype dbt(opt->conn);

    { etymon::pgconn_result r(opt->conn, "BEGIN;"); }

    upgrade_add_new_table_dbsystem("circulation_check_ins", opt, dbt, false);

    string sql = "UPDATE dbsystem.main SET database_version = 24;";
    ulog_sql(sql, opt);
    { etymon::pgconn_result r(opt->conn, sql); }

    { etymon::pgconn_result r(opt->conn, "COMMIT;"); }
    ulog_commit(opt);
}

void database_upgrade_25(database_upgrade_options* opt)
{
    dbtype dbt(opt->conn);

    { etymon::pgconn_result r(opt->conn, "BEGIN;"); }

    upgrade_add_new_table_dbsystem("audit_circulation_logs", opt, dbt, false);

    string sql = "UPDATE dbsystem.main SET database_version = 25;";
    ulog_sql(sql, opt);
    { etymon::pgconn_result r(opt->conn, sql); }

    { etymon::pgconn_result r(opt->conn, "COMMIT;"); }
    ulog_commit(opt);
}

void database_upgrade_26(database_upgrade_options* opt)
{
    dbtype dbt(opt->conn);

    { etymon::pgconn_result r(opt->conn, "BEGIN;"); }

    upgrade_add_new_table_dbsystem("srs_records", opt, dbt, false);

    string sql = "UPDATE dbsystem.main SET database_version = 26;";
    ulog_sql(sql, opt);
    { etymon::pgconn_result r(opt->conn, sql); }

    { etymon::pgconn_result r(opt->conn, "COMMIT;"); }
    ulog_commit(opt);
}

void database_upgrade_27(database_upgrade_options* opt)
{
    dbtype dbt(opt->conn);

    ldp_schema schema;
    ldp_schema::make_default_schema(&schema);

    for (auto& table : schema.tables) {
        string sql = "ALTER TABLE " + table.name + " DROP COLUMN tenant_id;";
        ulog_sql(sql, opt);
        try {
            etymon::pgconn_result r(opt->conn, sql);
        } catch (runtime_error& e) {}
        sql = "ALTER TABLE history." + table.name + " DROP COLUMN tenant_id;";
        ulog_sql(sql, opt);
        try {
            etymon::pgconn_result r(opt->conn, sql);
        } catch (runtime_error& e) {}
    }

    string sql = "UPDATE dbsystem.main SET database_version = 27;";
    ulog_sql(sql, opt);
    { etymon::pgconn_result r(opt->conn, sql); }
}

void database_upgrade_28(database_upgrade_options* opt)
{
    dbtype dbt(opt->conn);

    { etymon::pgconn_result r(opt->conn, "BEGIN;"); }

    upgrade_add_new_table_dbsystem("inventory_holdings_sources", opt, dbt, false);

    string sql = "UPDATE dbsystem.main SET database_version = 28;";
    ulog_sql(sql, opt);
    { etymon::pgconn_result r(opt->conn, sql); }

    { etymon::pgconn_result r(opt->conn, "COMMIT;"); }
    ulog_commit(opt);
}

void database_upgrade_29(database_upgrade_options* opt)
{
    dbtype dbt(opt->conn);

    { etymon::pgconn_result r(opt->conn, "BEGIN;"); }

    upgrade_add_new_table_dbsystem("perm_permissions", opt, dbt, false);
    upgrade_add_new_table_dbsystem("perm_users", opt, dbt, false);
    upgrade_add_new_table_dbsystem("srs_error", opt, dbt, false);

    string sql = "UPDATE dbsystem.main SET database_version = 29;";
    ulog_sql(sql, opt);
    { etymon::pgconn_result r(opt->conn, sql); }

    { etymon::pgconn_result r(opt->conn, "COMMIT;"); }
    ulog_commit(opt);
}

void database_upgrade_30(database_upgrade_options* opt)
{
    dbtype dbt(opt->conn);

    { etymon::pgconn_result r(opt->conn, "BEGIN;"); }

    upgrade_add_new_table_dbsystem("patron_blocks_user_summary", opt, dbt, false);

    string sql = "UPDATE dbsystem.main SET database_version = 30;";
    ulog_sql(sql, opt);
    { etymon::pgconn_result r(opt->conn, sql); }

    { etymon::pgconn_result r(opt->conn, "COMMIT;"); }
    ulog_commit(opt);
}

