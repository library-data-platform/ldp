#include <cstring>

#include "schema.h"

/*
void deriveTableName(const string& table, string* tableName)
{
    tableName->clear();
    const char* p = strrchr(table.c_str(), '/');
    if (p == nullptr)
        return;
    while (true) {
        p++;
        switch (*p) {
        case '\0':
            return;
        case '-':
            (*tableName) += '_';
            break;
        default:
            (*tableName) += *p;
        }
    }
}
*/

void Schema::MakeDefaultSchema(Schema* schema)
{
    schema->tables.clear();

    TableSchema table;

    table.sourceType = SourceType::rmb;

    ///////////////////////////////////////////////////////////////////////////
    table.moduleName = "mod-circulation-storage";

    table.sourcePath = "/cancellation-reason-storage/cancellation-reasons";
    table.tableName = "circulation_cancellation_reasons";
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
    table.sourcePath = "/circulation-rules-storage";
    //table.tableName = "";
    // schema->tables.push_back(table);

    table.sourcePath =
        "/fixed-due-date-schedule-storage/fixed-due-date-schedules";
    table.tableName = "circulation_fixed_due_date_schedules";
    schema->tables.push_back(table);

    table.sourcePath = "/loan-policy-storage/loan-policies";
    table.tableName = "circulation_loan_policies";
    schema->tables.push_back(table);

    table.sourcePath = "/loan-storage/loans";
    table.tableName = "circulation_loans";
    schema->tables.push_back(table);

    table.sourcePath = "/loan-storage/loan-history";
    table.tableName = "circulation_loan_history";
    schema->tables.push_back(table);

    // okapi returns 422 Unprocessable Entity
    table.sourcePath =
        "/patron-action-session-storage/expired-session-patron-ids";
    //table.tableName = "";
    // schema->tables.push_back(table);

    table.sourcePath = "/patron-action-session-storage/patron-action-sessions";
    table.tableName = "circulation_patron_action_sessions";
    schema->tables.push_back(table);

    table.sourcePath = "/patron-notice-policy-storage/patron-notice-policies";
    table.tableName = "circulation_patron_notice_policies";
    schema->tables.push_back(table);

    table.sourcePath = "/request-policy-storage/request-policies";
    table.tableName = "circulation_request_policies";
    schema->tables.push_back(table);

    // Removed for lack of data
    //table.sourcePath = "/request-preference-storage/request-preference";
    //table.tableName = "circulation_request_preference";
    //schema->tables.push_back(table);

    table.sourcePath = "/request-storage/requests";
    table.tableName = "circulation_requests";
    schema->tables.push_back(table);

    table.sourcePath = "/scheduled-notice-storage/scheduled-notices";
    table.tableName = "circulation_scheduled_notices";
    schema->tables.push_back(table);

    table.sourcePath = "/staff-slips-storage/staff-slips";
    table.tableName = "circulation_staff_slips";
    schema->tables.push_back(table);

    ///////////////////////////////////////////////////////////////////////////
    table.moduleName = "mod-feesfines";

    table.sourcePath = "/lost-item-fees-policies";
    table.tableName = "feesfines_lost_item_fees_policies";
    schema->tables.push_back(table);

    table.sourcePath = "/overdue-fines-policies";
    table.tableName = "feesfines_overdue_fines_policies";
    schema->tables.push_back(table);

    ///////////////////////////////////////////////////////////////////////////
/*
    table.moduleName = "mod-courses";

    table.sourcePath = "/coursereserves/courselistings";
    table.tableName = "course_courselistings";
    schema->tables.push_back(table);

    table.sourcePath = "/coursereserves/roles";
    table.tableName = "course_roles";
    schema->tables.push_back(table);

    table.sourcePath = "/coursereserves/terms";
    table.tableName = "course_terms";
    schema->tables.push_back(table);

    table.sourcePath = "/coursereserves/coursetypes";
    table.tableName = "course_coursetypes";
    schema->tables.push_back(table);

    table.sourcePath = "/coursereserves/departments";
    table.tableName = "course_departments";
    schema->tables.push_back(table);

    table.sourcePath = "/coursereserves/processingstatuses";
    table.tableName = "course_processingstatuses";
    schema->tables.push_back(table);

    table.sourcePath = "/coursereserves/copyrightstatuses";
    table.tableName = "course_copyrightstatuses";
    schema->tables.push_back(table);

    table.sourcePath = "/coursereserves/courses";
    table.tableName = "course_courses";
    schema->tables.push_back(table);

    table.sourcePath = "/coursereserves/reserves";
    table.tableName = "course_reserves";
    schema->tables.push_back(table);
*/

    ///////////////////////////////////////////////////////////////////////////
    table.moduleName = "mod-finance-storage";

    table.sourcePath = "/finance-storage/budgets";
    table.tableName = "finance_budgets";
    schema->tables.push_back(table);

    table.sourcePath = "/finance-storage/fiscal-years";
    table.tableName = "finance_fiscal_years";
    schema->tables.push_back(table);

    // Removed for lack of data
    //table.sourcePath = "/finance-storage/fund-distributions";
    //table.tableName = "finance_fund_distributions";
    //schema->tables.push_back(table);

    table.sourcePath = "/finance-storage/fund-types";
    table.tableName = "finance_fund_types";
    schema->tables.push_back(table);

    table.sourcePath = "/finance-storage/funds";
    table.tableName = "finance_funds";
    schema->tables.push_back(table);

    table.sourcePath = "/finance-storage/group-fund-fiscal-years";
    table.tableName = "finance_group_fund_fiscal_years";
    schema->tables.push_back(table);

    table.sourcePath = "/finance-storage/groups";
    table.tableName = "finance_groups";
    schema->tables.push_back(table);

    // Removed for lack of data
    //table.sourcePath = "/finance-storage/ledger-fiscal-years";
    //table.tableName = "finance_ledger_fiscal_years";
    //schema->tables.push_back(table);

    table.sourcePath = "/finance-storage/ledgers";
    table.tableName = "finance_ledgers";
    schema->tables.push_back(table);

    table.sourcePath = "/finance-storage/transactions";
    table.tableName = "finance_transactions";
    schema->tables.push_back(table);

    ///////////////////////////////////////////////////////////////////////////
    table.moduleName = "mod-inventory-storage";

    table.sourcePath = "/alternative-title-types";
    table.tableName = "inventory_alternative_title_types";
    schema->tables.push_back(table);

    table.sourcePath = "/call-number-types";
    table.tableName = "inventory_call_number_types";
    schema->tables.push_back(table);

    table.sourcePath = "/classification-types";
    table.tableName = "inventory_classification_types";
    schema->tables.push_back(table);

    table.sourcePath = "/contributor-name-types";
    table.tableName = "inventory_contributor_name_types";
    schema->tables.push_back(table);

    table.sourcePath = "/contributor-types";
    table.tableName = "inventory_contributor_types";
    schema->tables.push_back(table);

    table.sourcePath = "/electronic-access-relationships";
    table.tableName = "inventory_electronic_access_relationships";
    schema->tables.push_back(table);

    table.sourcePath = "/holdings-note-types";
    table.tableName = "inventory_holdings_note_types";
    schema->tables.push_back(table);

    // large data
    table.sourcePath = "/holdings-storage/holdings";
    table.directSourceTable = "mod_inventory_storage.holdings_record";
    table.tableName = "inventory_holdings";
    schema->tables.push_back(table);

    table.sourcePath = "/holdings-types";
    table.tableName = "inventory_holdings_types";
    schema->tables.push_back(table);

    table.sourcePath = "/identifier-types";
    table.tableName = "inventory_identifier_types";
    schema->tables.push_back(table);

    table.sourcePath = "/ill-policies";
    table.tableName = "inventory_ill_policies";
    schema->tables.push_back(table);

    table.sourcePath = "/instance-formats";
    table.tableName = "inventory_instance_formats";
    schema->tables.push_back(table);

    table.sourcePath = "/instance-note-types";
    table.tableName = "inventory_instance_note_types";
    schema->tables.push_back(table);

    table.sourcePath = "/instance-relationship-types";
    table.tableName = "inventory_instance_relationship_types";
    schema->tables.push_back(table);

    table.sourcePath = "/instance-statuses";
    table.tableName = "inventory_instance_statuses";
    schema->tables.push_back(table);

    table.sourcePath = "/instance-storage/instance-relationships";
    table.tableName = "inventory_instance_relationships";
    schema->tables.push_back(table);

    // large data
    table.sourcePath = "/instance-storage/instances";
    table.directSourceTable = "mod_inventory_storage.instance";
    table.tableName = "inventory_instances";
    schema->tables.push_back(table);

    table.sourcePath = "/instance-types";
    table.tableName = "inventory_instance_types";
    schema->tables.push_back(table);

    table.sourcePath = "/item-damaged-statuses";
    table.tableName = "inventory_item_damaged_statuses";
    schema->tables.push_back(table);

    table.sourcePath = "/item-note-types";
    table.tableName = "inventory_item_note_types";
    schema->tables.push_back(table);

    // large data
    table.sourcePath = "/item-storage/items";
    table.directSourceTable = "mod_inventory_storage.item";
    table.tableName = "inventory_items";
    schema->tables.push_back(table);

    table.sourcePath = "/location-units/campuses";
    table.tableName = "inventory_campuses";
    schema->tables.push_back(table);

    table.sourcePath = "/location-units/institutions";
    table.tableName = "inventory_institutions";
    schema->tables.push_back(table);

    table.sourcePath = "/location-units/libraries";
    table.tableName = "inventory_libraries";
    schema->tables.push_back(table);

    table.sourcePath = "/loan-types";
    table.tableName = "inventory_loan_types";
    schema->tables.push_back(table);

    table.sourcePath = "/locations";
    table.tableName = "inventory_locations";
    schema->tables.push_back(table);

    table.sourcePath = "/material-types";
    table.tableName = "inventory_material_types";
    schema->tables.push_back(table);

    table.sourcePath = "/modes-of-issuance";
    table.tableName = "inventory_modes_of_issuance";
    schema->tables.push_back(table);

    table.sourcePath = "/nature-of-content-terms";
    table.tableName = "inventory_nature_of_content_terms";
    schema->tables.push_back(table);

    table.sourcePath = "/service-points";
    table.tableName = "inventory_service_points";
    schema->tables.push_back(table);

    table.sourcePath = "/service-points-users";
    table.tableName = "inventory_service_points_users";
    schema->tables.push_back(table);

    table.sourcePath = "/statistical-code-types";
    table.tableName = "inventory_statistical_code_types";
    schema->tables.push_back(table);

    table.sourcePath = "/statistical-codes";
    table.tableName = "inventory_statistical_codes";
    schema->tables.push_back(table);

    ///////////////////////////////////////////////////////////////////////////
    table.moduleName = "mod-invoice-storage";

    // okapi returns 400 Bad Request
    table.sourcePath = "/invoice-storage/invoice-line-number";
    //table.tableName = "";
    // schema->tables.push_back(table);

    table.sourcePath = "/invoice-storage/invoice-lines";
    table.tableName = "invoice_lines";
    schema->tables.push_back(table);

    table.sourcePath = "/invoice-storage/invoices";
    table.tableName = "invoice_invoices";
    schema->tables.push_back(table);

    table.sourcePath = "/voucher-storage/voucher-lines";
    table.tableName = "invoice_voucher_lines";
    schema->tables.push_back(table);

    table.sourcePath = "/voucher-storage/vouchers";
    table.tableName = "invoice_vouchers";
    schema->tables.push_back(table);

    ///////////////////////////////////////////////////////////////////////////
    table.moduleName = "mod-orders-storage";

    table.sourcePath = "/acquisitions-units-storage/memberships";
    table.tableName = "acquisitions_memberships";
    schema->tables.push_back(table);

    table.sourcePath = "/acquisitions-units-storage/units";
    table.tableName = "acquisitions_units";
    schema->tables.push_back(table);

    table.sourcePath = "/orders-storage/alerts";
    table.tableName = "po_alerts";
    schema->tables.push_back(table);

    table.sourcePath = "/orders-storage/order-invoice-relns";
    table.tableName = "po_order_invoice_relns";
    schema->tables.push_back(table);

    table.sourcePath = "/orders-storage/order-templates";
    table.tableName = "po_order_templates";
    schema->tables.push_back(table);

    table.sourcePath = "/orders-storage/pieces";
    table.tableName = "po_pieces";
    schema->tables.push_back(table);

    // okapi returns 400 Bad Request
    table.sourcePath = "/orders-storage/po-line-number";
    //table.tableName = "";
    // schema->tables.push_back(table);

    table.sourcePath = "/orders-storage/po-lines";
    table.tableName = "po_lines";
    schema->tables.push_back(table);

    table.sourcePath = "/orders-storage/purchase-orders";
    table.tableName = "po_purchase_orders";
    schema->tables.push_back(table);

    table.sourcePath = "/orders-storage/receiving-history";
    table.tableName = "po_receiving_history";
    schema->tables.push_back(table);

    table.sourcePath = "/orders-storage/reporting-codes";
    table.tableName = "po_reporting_codes";
    schema->tables.push_back(table);

    ///////////////////////////////////////////////////////////////////////////
    table.moduleName = "mod-organizations-storage";

    table.sourcePath = "/organizations-storage/addresses";
    table.tableName = "organization_addresses";
    schema->tables.push_back(table);

    table.sourcePath = "/organizations-storage/categories";
    table.tableName = "organization_categories";
    schema->tables.push_back(table);

    table.sourcePath = "/organizations-storage/contacts";
    table.tableName = "organization_contacts";
    schema->tables.push_back(table);

    table.sourcePath = "/organizations-storage/emails";
    table.tableName = "organization_emails";
    schema->tables.push_back(table);

    table.sourcePath = "/organizations-storage/interfaces";
    table.tableName = "organization_interfaces";
    schema->tables.push_back(table);

    table.sourcePath = "/organizations-storage/organizations";
    table.tableName = "organization_organizations";
    schema->tables.push_back(table);

    table.sourcePath = "/organizations-storage/phone-numbers";
    table.tableName = "organization_phone_numbers";
    schema->tables.push_back(table);

    table.sourcePath = "/organizations-storage/urls";
    table.tableName = "organization_urls";
    schema->tables.push_back(table);

    ///////////////////////////////////////////////////////////////////////////
    table.moduleName = "mod-source-record-storage";

    table.sourceType = SourceType::rmbMarc;
    table.sourcePath = "/source-storage/records";
    table.tableName = "testing_source_records";
    schema->tables.push_back(table);
    table.sourceType = SourceType::rmb;

    ///////////////////////////////////////////////////////////////////////////
    table.moduleName = "mod-users";

    // Removed for lack of data
    //table.sourcePath = "/addresstypes";
    //table.tableName = "user_addresstypes";
    //schema->tables.push_back(table);

    table.sourcePath = "/groups";
    table.tableName = "user_groups";
    schema->tables.push_back(table);

    // Removed for lack of data
    //table.sourcePath = "/proxiesfor";
    //table.tableName = "user_proxiesfor";
    //schema->tables.push_back(table);

    table.sourcePath = "/users";
    table.tableName = "user_users";
    schema->tables.push_back(table);
}

void ColumnSchema::columnTypeToString(ColumnType type, string* str)
{
    switch (type) {
    case ColumnType::bigint:
        *str = "BIGINT";
        break;
    case ColumnType::boolean:
        *str = "BOOLEAN";
        break;
    case ColumnType::numeric:
        *str = "NUMERIC(12,2)";
        break;
    case ColumnType::timestamptz:
        *str = "TIMESTAMPTZ";
        break;
    case ColumnType::varchar:
        *str = "VARCHAR(65535)";
        break;
    default:
        *str = "(unknown_column_type)";
    }
}

ColumnType ColumnSchema::selectColumnType(const Counts& counts)
{
    if (counts.string > 0) {
        if (counts.string == counts.dateTime)
            return ColumnType::timestamptz;
        else
            return ColumnType::varchar;
    }
    if (counts.number > 0) {
        if (counts.floating > 0)
            return ColumnType::numeric;
        else
            return ColumnType::bigint;
    }
    if (counts.boolean > 0)
        return ColumnType::boolean;
    return ColumnType::varchar;
}


