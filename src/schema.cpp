#include <cstring>

#include "schema.h"

void ldp_schema::make_default_schema(ldp_schema* schema)
{
    schema->tables.clear();

    table_schema table;

    ///////////////////////////////////////////////////////////////////////////
    table.module_name = "mod-auth";
    
    table.source_type = data_source_type::permissions;
    table.source_spec = "perm::permissions";
    table.direct_source_table = "mod_permissions.permissions";
    table.name = "perm_permissions";
    schema->tables.push_back(table);

    table.source_type = data_source_type::permissions_users;
    table.source_spec = "perm::permissions_users";
    table.direct_source_table = "mod_permissions.permissions_users";
    table.name = "perm_users";
    schema->tables.push_back(table);

    table.source_type = data_source_type::rmb;
    ///////////////////////////////////////////////////////////////////////////

    table.source_type = data_source_type::rmb;
    table.anonymize = false;

    ///////////////////////////////////////////////////////////////////////////

    table.source_spec = "/instance-storage/instances";
    table.direct_source_table = "mod_inventory_storage.instance";
    table.name = "inventory_instances";
    schema->tables.push_back(table);

    table.source_spec = "/item-storage/items";
    table.direct_source_table = "mod_inventory_storage.item";
    table.name = "inventory_items";
    schema->tables.push_back(table);

    table.source_type = data_source_type::srs_marc_records;
    table.source_spec = "srs::marc_records_lb";
    table.direct_source_table = "mod_source_record_storage.marc_records_lb";
    table.name = "srs_marc";
    schema->tables.push_back(table);
    table.source_type = data_source_type::rmb;

    table.source_spec = "/holdings-storage/holdings";
    table.direct_source_table = "mod_inventory_storage.holdings_record";
    table.name = "inventory_holdings";
    schema->tables.push_back(table);

    table.source_type = data_source_type::srs_records;
    table.source_spec = "srs::records_lb";
    table.direct_source_table = "mod_source_record_storage.records_lb";
    table.name = "srs_records";
    schema->tables.push_back(table);
    table.source_type = data_source_type::rmb;

    table.source_spec = "/loan-storage/loan-history";
    table.name = "circulation_loan_history";
    schema->tables.push_back(table);

    table.source_spec = "/audit-data/circulation/logs";
    table.name = "audit_circulation_logs";
    table.anonymize = true;
    schema->tables.push_back(table);
    table.anonymize = false;

    table.source_spec = "/orders-storage/receiving-history";
    table.direct_source_table = "mod_orders_storage.receiving_history_view";
    table.name = "po_receiving_history";
    schema->tables.push_back(table);

    table.source_spec = "/accounts";
    table.name = "feesfines_accounts";
    schema->tables.push_back(table);

    table.source_spec = "/users";
    table.name = "user_users";
    table.anonymize = true;
    schema->tables.push_back(table);
    table.anonymize = false;

    table.source_spec = "/loan-storage/loans";
    table.name = "circulation_loans";
    schema->tables.push_back(table);

    table.source_spec = "/orders-storage/po-lines";
    table.name = "po_lines";
    schema->tables.push_back(table);

    table.source_spec = "/finance-storage/transactions";
    table.name = "finance_transactions";
    schema->tables.push_back(table);

    table.source_spec = "/feefineactions";
    table.name = "feesfines_feefineactions";
    schema->tables.push_back(table);

    table.source_spec = "/orders-storage/purchase-orders";
    table.name = "po_purchase_orders";
    schema->tables.push_back(table);

    table.source_spec = "/orders-storage/pieces";
    table.name = "po_pieces";
    schema->tables.push_back(table);

    table.source_type = data_source_type::srs_error_records;
    table.source_spec = "srs::error_records_lb";
    table.direct_source_table = "mod_source_record_storage.error_records_lb";
    table.name = "srs_error";
    schema->tables.push_back(table);
    table.source_type = data_source_type::rmb;


    ///////////////////////////////////////////////////////////////////////////
    table.module_name = "mod-audit";

    // audit_circulation_logs

    ///////////////////////////////////////////////////////////////////////////
    table.module_name = "mod-circulation-storage";

    table.source_spec = "/cancellation-reason-storage/cancellation-reasons";
    table.name = "circulation_cancellation_reasons";
    schema->tables.push_back(table);

    table.source_spec = "/check-in-storage/check-ins";
    table.name = "circulation_check_ins";
    schema->tables.push_back(table);

    // TODO This seems to use a different json format.
    // E.g. from folio-snapshot:
    // {
    //   "id" : "1721f01b-e69d-5c4c-5df2-523428a04c54",
    //     "rulesAsText" : "priority: t, s, c, b, a, m,
    //     g\nfallback-policy: l d9cd0bed-1b49-4b5e-a7bd-064b8d177231 r
    //     334e5a9e-94f9-4673-8d1d-ab552863886b n
    //     122b3d2b-4788-4f1e-9117-56daa91cb75c o
    //     fallback-overdue-fine-policy \nm
    //     1a54b431-2e4f-452d-9cae-9cee66c9a892: l
    //     43198de5-f56a-4a53-a0bd-5a324418967a r
    //     334e5a9e-94f9-4673-8d1d-ab552863886b n
    //     122b3d2b-4788-4f1e-9117-56daa91cb75c o
    //     15bb6216-8198-42da-bdd7-4b1e6dfb27ff "
    // }
    //table.source_spec = "/circulation-rules-storage";
    //table.name = "";
    // schema->tables.push_back(table);

    table.source_spec =
        "/fixed-due-date-schedule-storage/fixed-due-date-schedules";
    table.name = "circulation_fixed_due_date_schedules";
    schema->tables.push_back(table);

    // circulation_loan_history

    table.source_spec = "/loan-policy-storage/loan-policies";
    table.name = "circulation_loan_policies";
    schema->tables.push_back(table);

    // circulation_loans

    // okapi returns 422 Unprocessable Entity
    //table.source_spec = "/patron-action-session-storage/expired-session-patron-ids";
    //table.name = "";
    // schema->tables.push_back(table);

    table.source_spec = "/patron-action-session-storage/patron-action-sessions";
    table.name = "circulation_patron_action_sessions";
    schema->tables.push_back(table);

    table.source_spec = "/patron-notice-policy-storage/patron-notice-policies";
    table.name = "circulation_patron_notice_policies";
    schema->tables.push_back(table);

    table.source_spec = "/request-policy-storage/request-policies";
    table.name = "circulation_request_policies";
    schema->tables.push_back(table);

    table.source_spec = "/request-preference-storage/request-preference";
    table.name = "circulation_request_preference";
    schema->tables.push_back(table);

    table.source_spec = "/request-storage/requests";
    table.name = "circulation_requests";
    schema->tables.push_back(table);

    table.source_spec = "/scheduled-notice-storage/scheduled-notices";
    table.name = "circulation_scheduled_notices";
    schema->tables.push_back(table);

    table.source_spec = "/staff-slips-storage/staff-slips";
    table.name = "circulation_staff_slips";
    schema->tables.push_back(table);

    ///////////////////////////////////////////////////////////////////////////
    table.module_name = "mod-configuration";

    table.source_spec = "/configurations/entries";
    table.name = "configuration_entries";
    table.anonymize = true;
    schema->tables.push_back(table);
    table.anonymize = false;

    // No data found in reference environment
    // table.source_spec = "/configurations/audit";
    // table.name = "configuration_audit";
    // schema->tables.push_back(table);

    ///////////////////////////////////////////////////////////////////////////
    table.module_name = "mod-courses";

    table.source_spec = "/coursereserves/copyrightstatuses";
    table.name = "course_copyrightstatuses";
    schema->tables.push_back(table);

    table.source_spec = "/coursereserves/courselistings";
    table.name = "course_courselistings";
    schema->tables.push_back(table);

    table.source_spec = "/coursereserves/courses";
    table.name = "course_courses";
    schema->tables.push_back(table);

    table.source_spec = "/coursereserves/coursetypes";
    table.name = "course_coursetypes";
    schema->tables.push_back(table);

    table.source_spec = "/coursereserves/departments";
    table.name = "course_departments";
    schema->tables.push_back(table);

    table.source_spec = "/coursereserves/processingstatuses";
    table.name = "course_processingstatuses";
    schema->tables.push_back(table);

    table.source_spec = "/coursereserves/reserves";
    table.name = "course_reserves";
    schema->tables.push_back(table);

    table.source_spec = "/coursereserves/roles";
    table.name = "course_roles";
    schema->tables.push_back(table);

    table.source_spec = "/coursereserves/terms";
    table.name = "course_terms";
    schema->tables.push_back(table);

    ///////////////////////////////////////////////////////////////////////////
    table.module_name = "mod-email";

    table.source_spec = "/email";
    table.name = "email_email";
    schema->tables.push_back(table);

    ///////////////////////////////////////////////////////////////////////////
    table.module_name = "mod-feesfines";

    // feesfines_accounts

    table.source_spec = "/comments";
    table.name = "feesfines_comments";
    schema->tables.push_back(table);

    // feesfines_feefineactions

    table.source_spec = "/feefines";
    table.name = "feesfines_feefines";
    schema->tables.push_back(table);

    table.source_spec = "/lost-item-fees-policies";
    table.name = "feesfines_lost_item_fees_policies";
    schema->tables.push_back(table);

    table.source_spec = "/manualblocks";
    table.name = "feesfines_manualblocks";
    schema->tables.push_back(table);

    table.source_spec = "/overdue-fines-policies";
    table.name = "feesfines_overdue_fines_policies";
    schema->tables.push_back(table);

    table.source_spec = "/owners";
    table.name = "feesfines_owners";
    schema->tables.push_back(table);

    table.source_spec = "/payments";
    table.name = "feesfines_payments";
    schema->tables.push_back(table);

    table.source_spec = "/refunds";
    table.name = "feesfines_refunds";
    schema->tables.push_back(table);

    table.source_spec = "/transfer-criterias";
    table.name = "feesfines_transfer_criterias";
    schema->tables.push_back(table);

    table.source_spec = "/transfers";
    table.name = "feesfines_transfers";
    schema->tables.push_back(table);

    table.source_spec = "/waives";
    table.name = "feesfines_waives";
    schema->tables.push_back(table);

    ///////////////////////////////////////////////////////////////////////////
    table.module_name = "mod-finance-storage";

    table.source_spec = "/finance-storage/budgets";
    table.name = "finance_budgets";
    schema->tables.push_back(table);

    table.source_spec = "/finance-storage/expense-classes";
    table.name = "finance_expense_classes";
    schema->tables.push_back(table);

    table.source_spec = "/finance-storage/fiscal-years";
    table.name = "finance_fiscal_years";
    schema->tables.push_back(table);

    // Removed for lack of data
    //table.source_spec = "/finance-storage/fund-distributions";
    //table.name = "finance_fund_distributions";
    //schema->tables.push_back(table);

    table.source_spec = "/finance-storage/fund-types";
    table.name = "finance_fund_types";
    schema->tables.push_back(table);

    table.source_spec = "/finance-storage/funds";
    table.name = "finance_funds";
    schema->tables.push_back(table);

    table.source_spec = "/finance-storage/group-fund-fiscal-years";
    table.name = "finance_group_fund_fiscal_years";
    schema->tables.push_back(table);

    table.source_spec = "/finance-storage/groups";
    table.name = "finance_groups";
    schema->tables.push_back(table);

    // table.source_spec = "/finance-storage/ledger-fiscal-years";
    // table.name = "finance_ledger_fiscal_years";
    // schema->tables.push_back(table);

    table.source_spec = "/finance-storage/ledgers";
    table.name = "finance_ledgers";
    schema->tables.push_back(table);

    // finance_transactions

    ///////////////////////////////////////////////////////////////////////////
    table.module_name = "mod-inventory-storage";

    table.source_spec = "/alternative-title-types";
    table.name = "inventory_alternative_title_types";
    schema->tables.push_back(table);

    table.source_spec = "/call-number-types";
    table.name = "inventory_call_number_types";
    schema->tables.push_back(table);

    table.source_spec = "/location-units/campuses";
    table.name = "inventory_campuses";
    schema->tables.push_back(table);

    table.source_spec = "/classification-types";
    table.name = "inventory_classification_types";
    schema->tables.push_back(table);

    table.source_spec = "/contributor-name-types";
    table.name = "inventory_contributor_name_types";
    schema->tables.push_back(table);

    table.source_spec = "/contributor-types";
    table.name = "inventory_contributor_types";
    schema->tables.push_back(table);

    table.source_spec = "/electronic-access-relationships";
    table.name = "inventory_electronic_access_relationships";
    schema->tables.push_back(table);

    // inventory_holdings

    table.source_spec = "/holdings-note-types";
    table.name = "inventory_holdings_note_types";
    schema->tables.push_back(table);

    table.source_spec = "/holdings-types";
    table.name = "inventory_holdings_types";
    schema->tables.push_back(table);

    table.source_spec = "/identifier-types";
    table.name = "inventory_identifier_types";
    schema->tables.push_back(table);

    table.source_spec = "/ill-policies";
    table.name = "inventory_ill_policies";
    schema->tables.push_back(table);

    table.source_spec = "/instance-formats";
    table.name = "inventory_instance_formats";
    schema->tables.push_back(table);

    table.source_spec = "/instance-note-types";
    table.name = "inventory_instance_note_types";
    schema->tables.push_back(table);

    table.source_spec = "/instance-relationship-types";
    table.name = "inventory_instance_relationship_types";
    schema->tables.push_back(table);

    table.source_spec = "/instance-storage/instance-relationships";
    table.name = "inventory_instance_relationships";
    schema->tables.push_back(table);

    table.source_spec = "/instance-statuses";
    table.name = "inventory_instance_statuses";
    schema->tables.push_back(table);

    table.source_spec = "/instance-types";
    table.name = "inventory_instance_types";
    schema->tables.push_back(table);

    // inventory_instances

    table.source_spec = "/item-damaged-statuses";
    table.name = "inventory_item_damaged_statuses";
    schema->tables.push_back(table);

    table.source_spec = "/item-note-types";
    table.name = "inventory_item_note_types";
    schema->tables.push_back(table);

    // inventory_items

    table.source_spec = "/location-units/institutions";
    table.name = "inventory_institutions";
    schema->tables.push_back(table);

    table.source_spec = "/location-units/libraries";
    table.name = "inventory_libraries";
    schema->tables.push_back(table);

    table.source_spec = "/loan-types";
    table.name = "inventory_loan_types";
    schema->tables.push_back(table);

    table.source_spec = "/locations";
    table.name = "inventory_locations";
    schema->tables.push_back(table);

    table.source_spec = "/material-types";
    table.name = "inventory_material_types";
    schema->tables.push_back(table);

    table.source_spec = "/modes-of-issuance";
    table.name = "inventory_modes_of_issuance";
    schema->tables.push_back(table);

    table.source_spec = "/nature-of-content-terms";
    table.name = "inventory_nature_of_content_terms";
    schema->tables.push_back(table);

    table.source_spec = "/service-points";
    table.name = "inventory_service_points";
    schema->tables.push_back(table);

    table.source_spec = "/service-points-users";
    table.name = "inventory_service_points_users";
    schema->tables.push_back(table);

    table.source_spec = "/statistical-code-types";
    table.name = "inventory_statistical_code_types";
    schema->tables.push_back(table);

    table.source_spec = "/statistical-codes";
    table.name = "inventory_statistical_codes";
    schema->tables.push_back(table);

    ///////////////////////////////////////////////////////////////////////////
    table.module_name = "mod-invoice-storage";

    // okapi returns 400 Bad Request
    //table.source_spec = "/invoice-storage/invoice-line-number";
    //table.name = "invoice_line_number";
    //schema->tables.push_back(table);

    table.source_spec = "/invoice-storage/invoices";
    table.name = "invoice_invoices";
    schema->tables.push_back(table);

    table.source_spec = "/invoice-storage/invoice-lines";
    table.name = "invoice_lines";
    schema->tables.push_back(table);

    table.source_spec = "/voucher-storage/voucher-lines";
    table.name = "invoice_voucher_lines";
    schema->tables.push_back(table);

    table.source_spec = "/voucher-storage/vouchers";
    table.name = "invoice_vouchers";
    schema->tables.push_back(table);

    ///////////////////////////////////////////////////////////////////////////
    table.module_name = "mod-notes";

    table.source_spec = "/notes";
    table.name = "notes";
    table.anonymize = true;
    schema->tables.push_back(table);
    table.anonymize = false;

    ///////////////////////////////////////////////////////////////////////////
    table.module_name = "mod-orders-storage";

    table.source_spec = "/acquisitions-units-storage/memberships";
    table.name = "acquisitions_memberships";
    schema->tables.push_back(table);

    table.source_spec = "/acquisitions-units-storage/units";
    table.name = "acquisitions_units";
    schema->tables.push_back(table);

    table.source_spec = "/orders-storage/alerts";
    table.name = "po_alerts";
    schema->tables.push_back(table);

    // po_lines

    table.source_spec = "/orders-storage/order-invoice-relns";
    table.name = "po_order_invoice_relns";
    schema->tables.push_back(table);

    table.source_spec = "/orders-storage/order-templates";
    table.name = "po_order_templates";
    schema->tables.push_back(table);

    // po_pieces

    // okapi returns 400 Bad Request
    //table.source_spec = "/orders-storage/po-line-number";
    //table.name = "";
    //schema->tables.push_back(table);

    // po_purchase_orders

    // po_receiving_history

    table.source_spec = "/orders-storage/reporting-codes";
    table.name = "po_reporting_codes";
    schema->tables.push_back(table);

    ///////////////////////////////////////////////////////////////////////////
    table.module_name = "mod-organizations-storage";

    table.source_spec = "/organizations-storage/addresses";
    table.name = "organization_addresses";
    schema->tables.push_back(table);

    table.source_spec = "/organizations-storage/categories";
    table.name = "organization_categories";
    schema->tables.push_back(table);

    table.source_spec = "/organizations-storage/contacts";
    table.name = "organization_contacts";
    schema->tables.push_back(table);

    table.source_spec = "/organizations-storage/emails";
    table.name = "organization_emails";
    schema->tables.push_back(table);

    table.source_spec = "/organizations-storage/interfaces";
    table.name = "organization_interfaces";
    schema->tables.push_back(table);

    table.source_spec = "/organizations-storage/organizations";
    table.name = "organization_organizations";
    schema->tables.push_back(table);

    table.source_spec = "/organizations-storage/phone-numbers";
    table.name = "organization_phone_numbers";
    schema->tables.push_back(table);

    table.source_spec = "/organizations-storage/urls";
    table.name = "organization_urls";
    schema->tables.push_back(table);

    ///////////////////////////////////////////////////////////////////////////
    table.module_name = "mod-source-record-storage";

    // srs_marc

    // srs_records

    ///////////////////////////////////////////////////////////////////////////
    table.module_name = "mod-users";

    table.source_spec = "/addresstypes";
    table.name = "user_addresstypes";
    schema->tables.push_back(table);

    table.source_spec = "/departments";
    table.name = "user_departments";
    schema->tables.push_back(table);

    table.source_spec = "/groups";
    table.name = "user_groups";
    schema->tables.push_back(table);

    table.source_spec = "/proxiesfor";
    table.name = "user_proxiesfor";
    schema->tables.push_back(table);

    // user_users

    table.source_spec = "/holdings-sources";
    table.name = "inventory_holdings_sources";
    schema->tables.push_back(table);

    
}

void column_schema::type_to_string(column_type type, string* str)
{
    switch (type) {
    case column_type::bigint:
        *str = "BIGINT";
        break;
    case column_type::boolean:
        *str = "BOOLEAN";
        break;
    case column_type::numeric:
        *str = "NUMERIC(12,2)";
        break;
    case column_type::timestamptz:
        *str = "TIMESTAMPTZ";
        break;
    case column_type::id:
        *str = "VARCHAR(36)";
        break;
    case column_type::varchar:
        *str = "VARCHAR";
        break;
    default:
        *str = "(unknown_column_type)";
    }
}

bool column_schema::select_type(ldp_log* lg, const string& table,
                                const string& source_path,
                                const string& field,
                                const type_counts& counts,
                                column_type* ctype)
{
    // Check for incompatible types.
    if (counts.string > 0 && counts.number > 0) {
        lg->write(log_level::error, "", "",
                "Inconsistent data types in source data:\n"
                "    Table: " + table + "\n"
                "    Source path: " + source_path + "\n"
                "    Field: " + field + "\n"
                "    Data types found: number, string\n"
                "    Action: Table update canceled",
                -1);
        return false;
    }
    // Select a type.
    if (counts.string > 0) {
        if (counts.string == counts.uuid) {
            *ctype = column_type::id;
            return true;
        } else {
            if (counts.string == counts.date_time) {
                *ctype = column_type::timestamptz;
                return true;
            } else {
                *ctype = column_type::varchar;
                return true;
            }
        }
    }
    if (counts.number > 0) {
        if (counts.floating > 0) {
            *ctype = column_type::numeric;
            return true;
        } else {
            *ctype = column_type::bigint;
            return true;
        }
    }
    if (counts.boolean > 0) {
        *ctype = column_type::boolean;
        return true;
    }
    *ctype = column_type::varchar;
    return true;
}

