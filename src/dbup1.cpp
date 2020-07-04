#include "dbup1.h"
#include "initutil.h"

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

void upgrade_add_new_table(const string& table, database_upgrade_options* opt,
                           const dbtype& dbt)
{
    string sql;

    create_main_table_sql(table, opt->conn, dbt, &sql);
    ulog_sql(sql, opt);
    opt->conn->exec(sql);
    ulog_commit(opt);

    grant_select_on_table_sql(table, opt->ldp_user, opt->conn, &sql);
    ulog_sql(sql, opt);
    opt->conn->exec(sql);
    ulog_commit(opt);

    grant_select_on_table_sql(table, opt->ldpconfig_user, opt->conn, &sql);
    ulog_sql(sql, opt);
    opt->conn->exec(sql);
    ulog_commit(opt);

    create_history_table_sql(table, opt->conn, dbt, &sql);
    ulog_sql(sql, opt);
    opt->conn->exec(sql);
    ulog_commit(opt);

    grant_select_on_table_sql("history." + table, opt->ldp_user, opt->conn,
                              &sql);
    ulog_sql(sql, opt);
    opt->conn->exec(sql);
    ulog_commit(opt);

    grant_select_on_table_sql("history." + table, opt->ldpconfig_user,
                              opt->conn, &sql);
    ulog_sql(sql, opt);
    opt->conn->exec(sql);
    ulog_commit(opt);

    add_table_to_catalog_sql(opt->conn, table, &sql);
    ulog_sql(sql, opt);
    opt->conn->exec(sql);
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
    opt->conn->exec_direct(nullptr, sql);

    sql = "GRANT SELECT ON ldpsystem.tables TO " + opt->ldp_user + ";";
    opt->conn->exec_direct(nullptr, sql);

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
        add_table_to_catalog_sql(opt->conn, table[x], &sql);
        opt->conn->exec(sql);
        // Create stub table if it doesn't exist.
        sql =
            "CREATE TABLE IF NOT EXISTS " + string(table[x]) + " (\n"
            "    row_id BIGINT NOT NULL,\n"
            "    sk BIGINT NOT NULL,\n"
            "    id VARCHAR(65535) NOT NULL,\n"
            "    data " + dbt.json_type() + ",\n"
            "    tenant_id SMALLINT NOT NULL\n"
            ");";
        opt->conn->exec_direct(nullptr, sql);
        sql =
            "GRANT SELECT ON\n"
            "    " + string(table[x]) + "\n"
            "    TO " + opt->ldp_user + ";";
        opt->conn->exec_direct(nullptr, sql);
        // Recreate history table.
        sql = "DROP TABLE IF EXISTS\n"
            "    history." + string(table[x]) + ";";
        opt->conn->exec_direct(nullptr, sql);
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
        opt->conn->exec_direct(nullptr, sql);
        sql =
            "GRANT SELECT ON\n"
            "    history." + string(table[x]) + "\n"
            "    TO " + opt->ldp_user + ";";
        opt->conn->exec_direct(nullptr, sql);
        if (string(dbt.type_string()) == "PostgreSQL") {
            // Remove row_id columns.
            sql =
                "ALTER TABLE \n"
                "    " + string(table[x]) + "\n"
                "    DROP COLUMN row_id;";
            opt->conn->exec_direct(nullptr, sql);
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
    opt->conn->exec_direct(nullptr, sql);

    sql = "GRANT SELECT ON ldpsystem.idmap TO " + opt->ldp_user + ";";
    opt->conn->exec_direct(nullptr, sql);

    sql = "UPDATE ldpsystem.main SET ldp_schema_version = 1;";
    fprintf(opt->ulog, "%s\n", sql.c_str());
    fflush(opt->ulog);
    opt->conn->exec(sql);
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
        opt->conn->exec_direct(nullptr, sql);
        sql =
            "ALTER TABLE history." + string(table[x]) + "\n"
            "    ADD CONSTRAINT history_" + table[x] + "_pkey\n"
            "    PRIMARY KEY (sk, updated);";
        opt->conn->exec_direct(nullptr, sql);
    }

    string sql = "UPDATE ldpsystem.main SET ldp_schema_version = 2;";
    fprintf(opt->ulog, "%s\n", sql.c_str());
    fflush(opt->ulog);
    opt->conn->exec(sql);
    fprintf(opt->ulog, "-- Committed\n");
    fflush(opt->ulog);
}

void database_upgrade_3(database_upgrade_options* opt)
{
    string sql =
        "ALTER TABLE ldpconfig.general\n"
        "    ADD COLUMN log_referential_analysis\n"
        "        BOOLEAN NOT NULL DEFAULT FALSE;";
    opt->conn->exec_direct(nullptr, sql);
    sql =
        "ALTER TABLE ldpconfig.general\n"
        "    ADD COLUMN force_referential_constraints\n"
        "        BOOLEAN NOT NULL DEFAULT FALSE;";
    opt->conn->exec_direct(nullptr, sql);

    sql = "UPDATE ldpsystem.main SET ldp_schema_version = 3;";
    fprintf(opt->ulog, "%s\n", sql.c_str());
    fflush(opt->ulog);
    opt->conn->exec(sql);
    fprintf(opt->ulog, "-- Committed\n");
    fflush(opt->ulog);
}

void database_upgrade_4(database_upgrade_options* opt)
{
    dbtype dbt(opt->conn);

    string sql = "ALTER TABLE ldpsystem.log DROP COLUMN level;";
    opt->conn->exec_direct(nullptr, sql);

    sql =
        "ALTER TABLE ldpsystem.log\n"
        "    ADD COLUMN level\n"
        "        VARCHAR(7) NOT NULL DEFAULT '';";
    opt->conn->exec_direct(nullptr, sql);

    sql =
        "ALTER TABLE ldpsystem.idmap\n"
        "    ADD COLUMN table_name\n"
        "        VARCHAR(63) NOT NULL DEFAULT '';";
    opt->conn->exec_direct(nullptr, sql);

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
    opt->conn->exec_direct(nullptr, sql);

    sql = "GRANT SELECT ON ALL TABLES IN SCHEMA ldpsystem TO " + opt->ldp_user +
        ";";
    opt->conn->exec_direct(nullptr, sql);

    sql = "UPDATE ldpsystem.main SET ldp_schema_version = 4;";
    fprintf(opt->ulog, "%s\n", sql.c_str());
    fflush(opt->ulog);
    opt->conn->exec(sql);
    fprintf(opt->ulog, "-- Committed\n");
    fflush(opt->ulog);
}

void database_upgrade_5(database_upgrade_options* opt)
{
    dbtype dbt(opt->conn);

    string sql = "ALTER TABLE ldpsystem.idmap DROP CONSTRAINT idmap_pkey;";
    opt->conn->exec_direct(nullptr, sql);

    sql = "ALTER TABLE ldpsystem.idmap DROP CONSTRAINT idmap_id_key;";
    opt->conn->exec_direct(nullptr, sql);

    sql = "ALTER TABLE ldpsystem.idmap RENAME TO idmap_old;";
    opt->conn->exec_direct(nullptr, sql);

    string rskeys;
    dbt.redshift_keys("sk", "sk", &rskeys);
    sql =
        "CREATE TABLE ldpsystem.idmap (\n"
        "    sk BIGINT NOT NULL,\n"
        "    id VARCHAR(65535) NOT NULL,\n"
        "        PRIMARY KEY (sk),\n"
        "        UNIQUE (id)\n"
        ")" + rskeys + ";";
    opt->conn->exec_direct(nullptr, sql);

    sql =
        "INSERT INTO ldpsystem.idmap (sk, id)\n"
        "    SELECT sk, id FROM ldpsystem.idmap_old;";
    opt->conn->exec_direct(nullptr, sql);

    sql = "DROP TABLE ldpsystem.idmap_old;";
    opt->conn->exec_direct(nullptr, sql);

    sql = "UPDATE ldpsystem.main SET ldp_schema_version = 5;";
    fprintf(opt->ulog, "%s\n", sql.c_str());
    fflush(opt->ulog);
    opt->conn->exec(sql);
    fprintf(opt->ulog, "-- Committed\n");
    fflush(opt->ulog);
}

void database_upgrade_6(database_upgrade_options* opt)
{
    string sql = "GRANT USAGE ON SCHEMA ldpsystem TO " + opt->ldpconfig_user +
        ";";
    opt->conn->exec_direct(nullptr, sql);

    sql = "GRANT SELECT ON ldpsystem.idmap TO " + opt->ldp_user + ";";
    opt->conn->exec_direct(nullptr, sql);
    sql = "GRANT SELECT ON ldpsystem.idmap TO " + opt->ldpconfig_user + ";";
    opt->conn->exec_direct(nullptr, sql);

    sql = "GRANT SELECT ON ldpsystem.log TO " + opt->ldp_user + ";";
    opt->conn->exec_direct(nullptr, sql);
    sql = "GRANT SELECT ON ldpsystem.log TO " + opt->ldpconfig_user + ";";
    opt->conn->exec_direct(nullptr, sql);

    sql = "GRANT SELECT ON ldpsystem.main TO " + opt->ldp_user + ";";
    opt->conn->exec_direct(nullptr, sql);
    sql = "GRANT SELECT ON ldpsystem.main TO " + opt->ldpconfig_user + ";";
    opt->conn->exec_direct(nullptr, sql);

    sql = "GRANT SELECT ON ldpsystem.referential_constraints TO " +
        opt->ldp_user + ";";
    opt->conn->exec_direct(nullptr, sql);
    sql = "GRANT SELECT ON ldpsystem.referential_constraints TO " +
        opt->ldpconfig_user + ";";
    opt->conn->exec_direct(nullptr, sql);

    sql = "GRANT SELECT ON ldpsystem.tables TO " + opt->ldp_user + ";";
    opt->conn->exec_direct(nullptr, sql);
    sql = "GRANT SELECT ON ldpsystem.tables TO " + opt->ldpconfig_user + ";";
    opt->conn->exec_direct(nullptr, sql);

    sql = "GRANT USAGE ON SCHEMA ldpconfig TO " + opt->ldpconfig_user + ";";
    opt->conn->exec_direct(nullptr, sql);

    sql = "GRANT SELECT ON ALL TABLES IN SCHEMA ldpconfig TO " +
        opt->ldpconfig_user + ";";
    opt->conn->exec_direct(nullptr, sql);

    sql = "GRANT UPDATE ON ldpconfig.general TO " + opt->ldpconfig_user + ";";
    opt->conn->exec_direct(nullptr, sql);

    sql = "GRANT USAGE ON SCHEMA history TO " + opt->ldpconfig_user + ";";
    opt->conn->exec_direct(nullptr, sql);

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
        opt->conn->exec_direct(nullptr, sql);
    }

    sql = "GRANT USAGE ON SCHEMA local TO " + opt->ldpconfig_user + ";";
    opt->conn->exec_direct(nullptr, sql);

    sql = "UPDATE ldpsystem.main SET ldp_schema_version = 6;";
    fprintf(opt->ulog, "%s\n", sql.c_str());
    fflush(opt->ulog);
    opt->conn->exec(sql);
    fprintf(opt->ulog, "-- Committed\n");
    fflush(opt->ulog);
}

void database_upgrade_7(database_upgrade_options* opt)
{
    string sql =
        "ALTER TABLE ldpsystem.tables\n"
        "    ADD COLUMN updated TIMESTAMP WITH TIME ZONE;";
    opt->conn->exec_direct(nullptr, sql);

    sql =
        "ALTER TABLE ldpsystem.tables\n"
        "    ADD COLUMN row_count BIGINT;";
    opt->conn->exec_direct(nullptr, sql);

    sql =
        "ALTER TABLE ldpsystem.tables\n"
        "    ADD COLUMN history_row_count BIGINT;";
    opt->conn->exec_direct(nullptr, sql);

    sql =
        "ALTER TABLE ldpsystem.tables\n"
        "    ADD COLUMN documentation VARCHAR(65535);";
    opt->conn->exec_direct(nullptr, sql);

    sql =
        "ALTER TABLE ldpsystem.tables\n"
        "    ADD COLUMN documentation_url VARCHAR(65535);";
    opt->conn->exec_direct(nullptr, sql);

    sql =
        "ALTER TABLE ldpconfig.general\n"
        "    ADD COLUMN disable_anonymization BOOLEAN NOT NULL DEFAULT FALSE;";
    opt->conn->exec_direct(nullptr, sql);

    sql = "UPDATE ldpsystem.main SET ldp_schema_version = 7;";
    fprintf(opt->ulog, "%s\n", sql.c_str());
    fflush(opt->ulog);
    opt->conn->exec(sql);
    fprintf(opt->ulog, "-- Committed\n");
    fflush(opt->ulog);
}

void database_upgrade_8(database_upgrade_options* opt)
{
    string sql = "UPDATE ldpsystem.main SET ldp_schema_version = 8;";
    fprintf(opt->ulog, "%s\n", sql.c_str());
    fflush(opt->ulog);
    opt->conn->exec(sql);
    fprintf(opt->ulog, "-- Committed\n");
    fflush(opt->ulog);
}

void database_upgrade_9(database_upgrade_options* opt)
{
    dbtype dbt(opt->conn);

    string sql = "DROP TABLE ldpsystem.referential_constraints;";
    opt->conn->exec(sql);

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
    opt->conn->exec(sql);

    sql =
        "ALTER TABLE ldpconfig.general\n"
        "    RENAME COLUMN full_update_enabled TO enable_full_updates;";
    opt->conn->exec(sql);

    sql =
        "ALTER TABLE ldpconfig.general\n"
        "    RENAME COLUMN log_referential_analysis TO detect_foreign_keys;";
    opt->conn->exec(sql);

    sql =
        "ALTER TABLE ldpconfig.general\n"
        "    DROP COLUMN force_referential_constraints;";
    opt->conn->exec(sql);

    sql =
        "ALTER TABLE ldpconfig.general\n"
        "    ADD COLUMN force_foreign_key_constraints\n"
        "    BOOLEAN NOT NULL DEFAULT FALSE;";
    opt->conn->exec(sql);

    sql =
        "ALTER TABLE ldpconfig.general\n"
        "    ADD COLUMN enable_foreign_key_warnings\n"
        "    BOOLEAN NOT NULL DEFAULT FALSE;";
    opt->conn->exec(sql);

    sql =
        "CREATE TABLE ldpconfig.foreign_keys (\n"
        "    enable_constraint BOOLEAN NOT NULL,\n"
        "    referencing_table VARCHAR(63) NOT NULL,\n"
        "    referencing_column VARCHAR(63) NOT NULL,\n"
        "    referenced_table VARCHAR(63) NOT NULL,\n"
        "    referenced_column VARCHAR(63) NOT NULL\n"
        ");";
    opt->conn->exec(sql);

    sql = "UPDATE ldpsystem.main SET ldp_schema_version = 9;";
    fprintf(opt->ulog, "%s\n", sql.c_str());
    fflush(opt->ulog);
    opt->conn->exec(sql);
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
        opt->conn->exec(sql);
        fprintf(opt->ulog, "-- Committed\n");
        fflush(opt->ulog);
    }

    for (int x = 0; table[x] != nullptr; x++) {
        string sql =
            "ALTER TABLE history." + string(table[x]) + "\n"
            "    ALTER COLUMN id TYPE VARCHAR(36);";
        fprintf(opt->ulog, "%s\n", sql.c_str());
        fflush(opt->ulog);
        opt->conn->exec(sql);
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
        opt->conn->exec(sql);
        fprintf(opt->ulog, "-- Committed\n");
        fflush(opt->ulog);
    }

    string sql = "UPDATE ldpsystem.main SET ldp_schema_version = 10;";
    fprintf(opt->ulog, "%s\n", sql.c_str());
    fflush(opt->ulog);
    opt->conn->exec(sql);
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
    opt->conn->exec(sql);
    fprintf(opt->ulog, "-- Committed\n");
    fflush(opt->ulog);

    for (int x = 0; table[x] != nullptr; x++) {

        if (string(dbt.type_string()) == "Redshift") {

            sql =
                "ALTER TABLE history." + string(table[x]) + "\n"
                "    ALTER DISTKEY id;";
            fprintf(opt->ulog, "%s\n", sql.c_str());
            fflush(opt->ulog);
            opt->conn->exec(sql);
            fprintf(opt->ulog, "-- Committed\n");
            fflush(opt->ulog);

            sql =
                "ALTER TABLE history." + string(table[x]) + "\n"
                "    ALTER COMPOUND SORTKEY (id, updated);";
            fprintf(opt->ulog, "%s\n", sql.c_str());
            fflush(opt->ulog);
            opt->conn->exec(sql);
            fprintf(opt->ulog, "-- Committed\n");
            fflush(opt->ulog);
        }

        sql =
            "ALTER TABLE history." + string(table[x]) + "\n"
            "    DROP COLUMN sk CASCADE;";
        fprintf(opt->ulog, "%s\n", sql.c_str());
        fflush(opt->ulog);
        opt->conn->exec(sql);
        fprintf(opt->ulog, "-- Committed\n");
        fflush(opt->ulog);

        sql =
            "ALTER TABLE history." + string(table[x]) + "\n"
            "    DROP CONSTRAINT history_" + table[x] + "_id_updated_key;";
        fprintf(opt->ulog, "%s\n", sql.c_str());
        fflush(opt->ulog);
        opt->conn->exec(sql);
        fprintf(opt->ulog, "-- Committed\n");
        fflush(opt->ulog);

        sql =
            "ALTER TABLE history." + string(table[x]) + "\n"
            "    ADD CONSTRAINT history_" + table[x] + "_pkey\n"
            "    PRIMARY KEY (id, updated);";
        fprintf(opt->ulog, "%s\n", sql.c_str());
        fflush(opt->ulog);
        opt->conn->exec(sql);
        fprintf(opt->ulog, "-- Committed\n");
        fflush(opt->ulog);

    }

    sql = "UPDATE ldpsystem.main SET ldp_schema_version = 11;";
    fprintf(opt->ulog, "%s\n", sql.c_str());
    fflush(opt->ulog);
    opt->conn->exec(sql);
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
    opt->conn->exec(sql);
    fprintf(opt->ulog, "-- Committed\n");
    fflush(opt->ulog);
    // Update value
    sql =
        "UPDATE ldpconfig.general\n"
        "    SET update_all_tables = TRUE;";
    fprintf(opt->ulog, "%s\n", sql.c_str());
    fflush(opt->ulog);
    opt->conn->exec(sql);
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
    opt->conn->exec(sql);
    fprintf(opt->ulog, "-- Committed\n");
    fflush(opt->ulog);

    sql = "UPDATE ldpsystem.main SET ldp_schema_version = 12;";
    fprintf(opt->ulog, "%s\n", sql.c_str());
    fflush(opt->ulog);
    opt->conn->exec(sql);
    fprintf(opt->ulog, "-- Committed\n");
    fflush(opt->ulog);
}

void database_upgrade_13(database_upgrade_options* opt)
{
    dbtype dbt(opt->conn);

    upgrade_add_new_table("course_copyrightstatuses", opt, dbt);
    upgrade_add_new_table("course_courselistings", opt, dbt);
    upgrade_add_new_table("course_courses", opt, dbt);
    upgrade_add_new_table("course_coursetypes", opt, dbt);
    upgrade_add_new_table("course_departments", opt, dbt);
    upgrade_add_new_table("course_processingstatuses", opt, dbt);
    upgrade_add_new_table("course_reserves", opt, dbt);
    upgrade_add_new_table("course_roles", opt, dbt);
    upgrade_add_new_table("course_terms", opt, dbt);

    string sql = "UPDATE ldpsystem.main SET ldp_schema_version = 13;";
    ulog_sql(sql, opt);
    opt->conn->exec(sql);
    ulog_commit(opt);
}

void database_upgrade_14(database_upgrade_options* opt)
{
    dbtype dbt(opt->conn);

    upgrade_add_new_table("email_email", opt, dbt);

    string sql = "UPDATE ldpsystem.main SET ldp_schema_version = 14;";
    ulog_sql(sql, opt);
    opt->conn->exec(sql);
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
        opt->conn->exec(sql);
        fprintf(opt->ulog, "-- Committed\n");
        fflush(opt->ulog);
    }

    if (dbt.type() == dbsys::postgresql) {
        string sql =
            "ALTER TABLE ldpconfig.general\n"
            "    ALTER COLUMN update_all_tables DROP NOT NULL;";
        fprintf(opt->ulog, "%s\n", sql.c_str());
        fflush(opt->ulog);
        opt->conn->exec(sql);
        fprintf(opt->ulog, "-- Committed\n");
        fflush(opt->ulog);
    }

    string sql = "UPDATE ldpsystem.main SET ldp_schema_version = 15;";
    ulog_sql(sql, opt);
    opt->conn->exec(sql);
    ulog_commit(opt);
}

    //vector<string> tables = {
    //    "circulation_cancellation_reasons",
    //    "circulation_fixed_due_date_schedules",
    //    "circulation_loan_policies",
    //    "circulation_loans",
    //    "circulation_loan_history",
    //    "circulation_patron_action_sessions",
    //    "circulation_patron_notice_policies",
    //    "circulation_request_policies",
    //    "circulation_requests",
    //    "circulation_scheduled_notices",
    //    "circulation_staff_slips",
    //    "feesfines_accounts",
    //    "feesfines_comments",
    //    "feesfines_feefines",
    //    "feesfines_feefineactions",
    //    "feesfines_lost_item_fees_policies",
    //    "feesfines_manualblocks",
    //    "feesfines_overdue_fines_policies",
    //    "feesfines_owners",
    //    "feesfines_payments",
    //    "feesfines_refunds",
    //    "feesfines_transfer_criterias",
    //    "feesfines_transfers",
    //    "feesfines_waives",
    //    "finance_budgets",
    //    "finance_fiscal_years",
    //    "finance_fund_types",
    //    "finance_funds",
    //    "finance_group_fund_fiscal_years",
    //    "finance_groups",
    //    "finance_ledgers",
    //    "finance_transactions",
    //    "inventory_alternative_title_types",
    //    "inventory_call_number_types",
    //    "inventory_classification_types",
    //    "inventory_contributor_name_types",
    //    "inventory_contributor_types",
    //    "inventory_electronic_access_relationships",
    //    "inventory_holdings_note_types",
    //    "inventory_holdings",
    //    "inventory_holdings_types",
    //    "inventory_identifier_types",
    //    "inventory_ill_policies",
    //    "inventory_instance_formats",
    //    "inventory_instance_note_types",
    //    "inventory_instance_relationship_types",
    //    "inventory_instance_statuses",
    //    "inventory_instance_relationships",
    //    "inventory_instances",
    //    "inventory_instance_types",
    //    "inventory_item_damaged_statuses",
    //    "inventory_item_note_types",
    //    "inventory_items",
    //    "inventory_campuses",
    //    "inventory_institutions",
    //    "inventory_libraries",
    //    "inventory_loan_types",
    //    "inventory_locations",
    //    "inventory_material_types",
    //    "inventory_modes_of_issuance",
    //    "inventory_nature_of_content_terms",
    //    "inventory_service_points",
    //    "inventory_service_points_users",
    //    "inventory_statistical_code_types",
    //    "inventory_statistical_codes",
    //    "invoice_lines",
    //    "invoice_invoices",
    //    "invoice_voucher_lines",
    //    "invoice_vouchers",
    //    "acquisitions_memberships",
    //    "acquisitions_units",
    //    "po_alerts",
    //    "po_order_invoice_relns",
    //    "po_order_templates",
    //    "po_pieces",
    //    "po_lines",
    //    "po_purchase_orders",
    //    "po_receiving_history",
    //    "po_reporting_codes",
    //    "organization_addresses",
    //    "organization_categories",
    //    "organization_contacts",
    //    "organization_emails",
    //    "organization_interfaces",
    //    "organization_organizations",
    //    "organization_phone_numbers",
    //    "organization_urls",
    //    "user_groups",
    //    "user_users"
    //};

    //{
    //    etymon::odbc_tx tx(opt->conn);
    //    for (auto& table : tables) {
    //        sql =
    //            "INSERT INTO ldpconfig.update_tables\n"
    //            "    (enable_update, table_name, tenant_id)\n"
    //            "    VALUES\n"
    //            "    (TRUE, '" + table + "', 1);";
    //        fprintf(opt->ulog, "%s\n", sql.c_str());
    //        fflush(opt->ulog);
    //        opt->conn->exec(sql);
    //    }
    //    tx.commit();
    //    fprintf(opt->ulog, "-- Committed\n");
    //    fflush(opt->ulog);
    //}

