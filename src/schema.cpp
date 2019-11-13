#include <cstring>

#include "schema.h"

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

void Schema::MakeDefaultSchema(Schema* schema)
{
    schema->tables.clear();

    TableSchema table;

    table.sourceType = 1;

    ///////////////////////////////////////////////////////////////////////////
    table.moduleName = "mod-agreements";

    table.sourcePath = "/erm/contacts";
    table.tableName = "erm_contacts";
    //schema->tables.push_back(table);

    table.sourcePath = "/erm/entitlements";
    table.tableName = "erm_entitlements";
    //schema->tables.push_back(table);

    table.sourcePath = "/erm/jobs";
    table.tableName = "erm_jobs";
    //schema->tables.push_back(table);

    table.sourcePath = "/erm/kbs";
    table.tableName = "erm_kbs";
    //schema->tables.push_back(table);

    table.sourcePath = "/erm/org";
    table.tableName = "erm_org";
    //schema->tables.push_back(table);

    table.sourcePath = "/erm/packages";
    table.tableName = "erm_packages";
    //schema->tables.push_back(table);

    table.sourcePath = "/erm/refdata";
    table.tableName = "erm_refdata";
    //schema->tables.push_back(table);

    table.sourcePath = "/erm/resource";
    table.tableName = "erm_resource";
    //schema->tables.push_back(table);

    table.sourcePath = "/erm/sas";
    table.tableName = "erm_sas";
    //schema->tables.push_back(table);

    table.sourcePath = "/erm/titles";
    table.tableName = "erm_titles";
    //schema->tables.push_back(table);

    table.sourcePath = "/export";
    table.tableName = "erm_export";
    //schema->tables.push_back(table);

    table.sourcePath = "/erm/files";
    table.tableName = "erm_files";
    //schema->tables.push_back(table);

    ///////////////////////////////////////////////////////////////////////////
    table.moduleName = "mod-circulation-storage";

    table.sourcePath = "/cancellation-reason-storage/cancellation-reasons";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    // TODO This seems to use a different json format, e.g. from folio-snapshot:
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
    deriveTableName(table.sourcePath, &(table.tableName));
    //schema->tables.push_back(table);

    table.sourcePath =
        "/fixed-due-date-schedule-storage/fixed-due-date-schedules";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    table.sourcePath = "/loan-policy-storage/loan-policies";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    table.sourcePath = "/loan-storage/loans";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    table.sourcePath =
        "/patron-action-session-storage/expired-session-patron-ids";
    deriveTableName(table.sourcePath, &(table.tableName));
    //schema->tables.push_back(table);

    table.sourcePath = "/patron-action-session-storage/patron-action-sessions";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    table.sourcePath = "/patron-notice-policy-storage/patron-notice-policies";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    table.sourcePath = "/request-policy-storage/request-policies";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    table.sourcePath = "/request-preference-storage/request-preference";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    table.sourcePath = "/request-storage/requests";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    table.sourcePath = "/scheduled-notice-storage/scheduled-notices";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    table.sourcePath = "/staff-slips-storage/staff-slips";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    ///////////////////////////////////////////////////////////////////////////
    table.moduleName = "mod-finance-storage";

    table.sourcePath = "/finance-storage/budgets";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    table.sourcePath = "/finance-storage/fiscal-years";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    table.sourcePath = "/finance-storage/fund-distributions";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    table.sourcePath = "/finance-storage/fund-types";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    table.sourcePath = "/finance-storage/funds";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    table.sourcePath = "/finance-storage/group-budgets";
    table.tableName = "finance_group_budgets";
    schema->tables.push_back(table);

    table.sourcePath = "/finance-storage/group-fund-fiscal-years";
    table.tableName = "finance_group_fund_fiscal_years";
    schema->tables.push_back(table);

    table.sourcePath = "/finance-storage/groups";
    table.tableName = "finance_groups";
    schema->tables.push_back(table);

    table.sourcePath = "/finance-storage/ledger-fiscal-years";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    table.sourcePath = "/finance-storage/ledgers";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    table.sourcePath = "/finance-storage/transactions";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    ///////////////////////////////////////////////////////////////////////////
    table.moduleName = "mod-inventory-storage";

    table.sourcePath = "/alternative-title-types";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    table.sourcePath = "/call-number-types";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    table.sourcePath = "/classification-types";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    table.sourcePath = "/contributor-name-types";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    table.sourcePath = "/contributor-types";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    table.sourcePath = "/electronic-access-relationships";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    table.sourcePath = "/holdings-note-types";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    table.sourcePath = "/holdings-storage/holdings";
    deriveTableName(table.sourcePath, &(table.tableName));
    // large data
    schema->tables.push_back(table);

    table.sourcePath = "/holdings-types";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    table.sourcePath = "/identifier-types";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    table.sourcePath = "/ill-policies";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    table.sourcePath = "/instance-formats";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    table.sourcePath = "/instance-note-types";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    table.sourcePath = "/instance-relationship-types";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    table.sourcePath = "/instance-statuses";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    table.sourcePath = "/instance-storage/instance-relationships";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    table.sourcePath = "/instance-storage/instances";
    deriveTableName(table.sourcePath, &(table.tableName));
    // large data
    schema->tables.push_back(table);

    table.sourcePath = "/instance-types";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    table.sourcePath = "/item-damaged-statuses";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    table.sourcePath = "/item-note-types";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    table.sourcePath = "/item-storage/items";
    deriveTableName(table.sourcePath, &(table.tableName));
    // large data
    schema->tables.push_back(table);

    table.sourcePath = "/location-units/campuses";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    table.sourcePath = "/location-units/institutions";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    table.sourcePath = "/location-units/libraries";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    table.sourcePath = "/loan-types";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    table.sourcePath = "/locations";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    table.sourcePath = "/material-types";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    table.sourcePath = "/modes-of-issuance";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    table.sourcePath = "/nature-of-content-terms";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    table.sourcePath = "/service-points";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    table.sourcePath = "/service-points-users";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    table.sourcePath = "/shelf-locations";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    table.sourcePath = "/statistical-code-types";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    table.sourcePath = "/statistical-codes";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    ///////////////////////////////////////////////////////////////////////////
    table.moduleName = "mod-invoice-storage";

    table.sourcePath = "/invoice-storage/invoice-line-number";
    deriveTableName(table.sourcePath, &(table.tableName));
    //schema->tables.push_back(table);

    table.sourcePath = "/invoice-storage/invoice-lines";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    table.sourcePath = "/invoice-storage/invoice-number";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    table.sourcePath = "/invoice-storage/invoices";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    table.sourcePath = "/voucher-storage/voucher-lines";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    table.sourcePath = "/voucher-storage/voucher-number";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    table.sourcePath = "/voucher-storage/vouchers";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    ///////////////////////////////////////////////////////////////////////////
    table.moduleName = "mod-orders-storage";

    table.sourcePath = "/acquisitions-units-storage/memberships";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    table.sourcePath = "/acquisitions-units-storage/units";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    table.sourcePath = "/orders-storage/alerts";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    table.sourcePath = "/orders-storage/order-invoice-relns";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    table.sourcePath = "/orders-storage/order-lines";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    table.sourcePath = "/orders-storage/order-templates";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    table.sourcePath = "/orders-storage/orders";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    table.sourcePath = "/orders-storage/pieces";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    table.sourcePath = "/orders-storage/po-line-number";
    deriveTableName(table.sourcePath, &(table.tableName));
    //schema->tables.push_back(table);

    table.sourcePath = "/orders-storage/po-lines";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    table.sourcePath = "/orders-storage/po-number";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    table.sourcePath = "/orders-storage/purchase-orders";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    table.sourcePath = "/orders-storage/receiving-history";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    table.sourcePath = "/orders-storage/reporting-codes";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    ///////////////////////////////////////////////////////////////////////////
    table.moduleName = "mod-organizations-storage";

    table.sourcePath = "/organizations-storage/addresses";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    table.sourcePath = "/organizations-storage/categories";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    table.sourcePath = "/organizations-storage/contacts";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    table.sourcePath = "/organizations-storage/emails";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    table.sourcePath = "/organizations-storage/interfaces";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    table.sourcePath = "/organizations-storage/organizations";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    table.sourcePath = "/organizations-storage/phone-numbers";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    table.sourcePath = "/organizations-storage/urls";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    ///////////////////////////////////////////////////////////////////////////
    table.moduleName = "mod-users";

    // Disabled for possible personal data.
    table.sourcePath = "/addresstypes";
    deriveTableName(table.sourcePath, &(table.tableName));
    //schema->tables.push_back(table);

    table.sourcePath = "/groups";
    deriveTableName(table.sourcePath, &(table.tableName));
    schema->tables.push_back(table);

    // Disabled for possible personal data.
    table.sourcePath = "/proxiesfor";
    deriveTableName(table.sourcePath, &(table.tableName));
    //schema->tables.push_back(table);

    table.sourcePath = "/users";
    deriveTableName(table.sourcePath, &(table.tableName));
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


