#include <set>

#include "anonymize.h"

static set<pair<string,string>> personal_data_fields = {

    {"acquisitions_memberships", "/userId"},

    {"circulation_loans", "/userId"},
    {"circulation_loans", "/proxyUserId"},

    {"circulation_loan_history", "/loan"},

    {"circulation_patron_action_sessions", "/patronId"},

    {"circulation_requests", "/proxyUserId"},
    {"circulation_requests", "/requesterId"},
    {"circulation_requests", "/requester/firstName"},
    {"circulation_requests", "/requester/lastName"},
    {"circulation_requests", "/requester/middleName"},
    {"circulation_requests", "/requester/barcode"},
    {"circulation_requests", "/requester/patronGroup"},
    {"circulation_requests", "/proxy/firstName"},
    {"circulation_requests", "/proxy/lastName"},
    {"circulation_requests", "/proxy/middleName"},
    {"circulation_requests", "/proxy/barcode"},
    {"circulation_requests", "/proxy/patronGroup"},

    {"circulation_scheduled_notices", "/recipientUserId"},

    {"feesfines_accounts", "/userId"},

    {"feesfines_feefineactions", "/userId"},

    {"feesfines_owners", "/owner"},

    {"inventory_items", "/circulationNotes/source/id"},
    {"inventory_items", "/circulationNotes/source/personal"},
    {"inventory_items", "/circulationNotes/source/personal/lastName"},
    {"inventory_items", "/circulationNotes/source/personal/firstName"},
    {"inventory_items", "/lastCheckIn/staffMemberId"},

    {"inventory_service_points_users", "/userId"},

    {"invoice_invoices", "/approvedBy"},

    {"organization_contacts", "/prefix"},
    {"organization_contacts", "/firstName"},
    {"organization_contacts", "/lastName"},
    {"organization_contacts", "/language"},
    {"organization_contacts", "/notes"},
    {"organization_contacts", "/phoneNumbers"},
    {"organization_contacts", "/emails"},

    {"organization_emails", "/value"},
    {"organization_emails", "/language"},

    {"organization_organizations", "/edi/ediJob/sendToEmails"},
    {"organization_organizations", "/contacts"},
    {"organization_organizations", "/emails"},
    {"organization_organizations", "/phoneNumbers"},

    {"organization_phone_numbers", "/phoneNumber"},
    {"organization_phone_numbers", "/language"},

    {"po_lines", "/requester"},
    {"po_lines", "/selector"},

    {"po_purchase_orders", "/approvedById"},
    {"po_purchase_orders", "/assignedTo"},
    {"po_purchase_orders", "/vendor"},
    {"po_purchase_orders", "/billTo"},
    {"po_purchase_orders", "/shipTo"},

    {"po_order_templates", "/vendor"},
    {"po_order_templates", "/billTo"},
    {"po_order_templates", "/shipTo"}

};

bool is_personal_data_field(const table_schema& table, const string& field)
{
    pair<string,string> p = pair<string,string>(table.name, field);
    return (personal_data_fields.find(p) != personal_data_fields.end());
}

