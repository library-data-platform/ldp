#include <cstring>

#include "schema.h"

void ldp_schema::make_default_schema(ldp_schema* schema)
{
    schema->tables.clear();

    table_schema table;

    table.source_type = data_source_type::rmb;
    table.anonymize = false;

    ///////////////////////////////////////////////////////////////////////////

    table.direct_source_table = "mod_inventory_storage.instance";
    table.module_name = "mod-inventory-storage";
    table.name = "inventory_instances";
    table.source_spec = "/instance-storage/instances";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_inventory_storage.item";
    table.module_name = "mod-inventory-storage";
    table.name = "inventory_items";
    table.source_spec = "/item-storage/items";
    schema->tables.push_back(table);

    table.source_type = data_source_type::srs_marc_records;
    table.direct_source_table = "mod_source_record_storage.marc_records_lb";
    table.module_name = "mod-source-record-storage";
    table.name = "srs_marc";
    table.source_spec = "srs::marc_records_lb";
    schema->tables.push_back(table);
    table.source_type = data_source_type::rmb;

    table.direct_source_table = "mod_inventory_storage.holdings_record";
    table.module_name = "mod-inventory-storage";
    table.name = "inventory_holdings";
    table.source_spec = "/holdings-storage/holdings";
    schema->tables.push_back(table);

    table.source_type = data_source_type::srs_records;
    table.direct_source_table = "mod_source_record_storage.records_lb";
    table.module_name = "mod-source-record-storage";
    table.name = "srs_records";
    table.source_spec = "srs::records_lb";
    schema->tables.push_back(table);
    table.source_type = data_source_type::rmb;

    table.direct_source_table = "mod_circulation_storage.audit_loan";
    table.module_name = "mod-circulation-storage";
    table.name = "circulation_loan_history";
    table.source_spec = "/loan-storage/loan-history";
    schema->tables.push_back(table);

    table.anonymize = true;
    table.direct_source_table = "mod_audit.circulation_logs";
    table.module_name = "mod-audit";
    table.name = "audit_circulation_logs";
    table.source_spec = "/audit-data/circulation/logs";
    schema->tables.push_back(table);
    table.anonymize = false;

    table.direct_source_table = "mod_orders_storage.receiving_history_view";
    table.module_name = "mod-orders-storage";
    table.name = "po_receiving_history";
    table.source_spec = "/orders-storage/receiving-history";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_feesfines.accounts";
    table.module_name = "mod-feesfines";
    table.name = "feesfines_accounts";
    table.source_spec = "/accounts";
    schema->tables.push_back(table);

    table.anonymize = true;
    table.direct_source_table = "mod_users.users";
    table.module_name = "mod-users";
    table.name = "user_users";
    table.source_spec = "/users";
    schema->tables.push_back(table);
    table.anonymize = false;

    table.direct_source_table = "mod_circulation_storage.loan";
    table.module_name = "mod-circulation-storage";
    table.name = "circulation_loans";
    table.source_spec = "/loan-storage/loans";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_orders_storage.po_line";
    table.module_name = "mod-orders-storage";
    table.name = "po_lines";
    table.source_spec = "/orders-storage/po-lines";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_finance_storage.transaction";
    table.name = "finance_transactions";
    table.source_spec = "/finance-storage/transactions";
    schema->tables.push_back(table);

    table.source_type = data_source_type::srs_error_records;
    table.direct_source_table = "mod_source_record_storage.error_records_lb";
    table.name = "srs_error";
    table.source_spec = "srs::error_records_lb";
    schema->tables.push_back(table);
    table.source_type = data_source_type::rmb;

    ///////////////////////////////////////////////////////////////////////////
    table.module_name = "mod-permissions";
    
    table.source_type = data_source_type::direct_only;
    table.source_spec = "perm::permissions";
    table.direct_source_table = "mod_permissions.permissions";
    table.name = "perm_permissions";
    schema->tables.push_back(table);

    table.source_type = data_source_type::direct_only;
    table.source_spec = "perm::permissions_users";
    table.direct_source_table = "mod_permissions.permissions_users";
    table.name = "perm_users";
    schema->tables.push_back(table);

    table.source_type = data_source_type::rmb;
    ///////////////////////////////////////////////////////////////////////////

    table.source_type = data_source_type::direct_only;
    table.direct_source_table = "mod_patron_blocks.user_summary";
    table.module_name = "mod-patron-blocks";
    table.name = "patron_blocks_user_summary";
    table.source_spec = "patron_blocks::user_summary";
    schema->tables.push_back(table);
    table.source_type = data_source_type::rmb;

    table.direct_source_table = "mod_feesfines.feefineactions";
    table.module_name = "mod-feesfines";
    table.name = "feesfines_feefineactions";
    table.source_spec = "/feefineactions";
    schema->tables.push_back(table);

    ///////////////////////////////////////////////////////////////////////////
    table.module_name = "mod-orders-storage";

    table.direct_source_table = "mod_orders_storage.purchase_order";
    table.name = "po_purchase_orders";
    table.source_spec = "/orders-storage/purchase-orders";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_orders_storage.pieces";
    table.name = "po_pieces";
    table.source_spec = "/orders-storage/pieces";
    schema->tables.push_back(table);

    ///////////////////////////////////////////////////////////////////////////
    table.module_name = "mod-audit";

    // audit_circulation_logs

    ///////////////////////////////////////////////////////////////////////////
    table.module_name = "mod-circulation-storage";

    table.direct_source_table = "mod_circulation_storage.cancellation_reason";
    table.name = "circulation_cancellation_reasons";
    table.source_spec = "/cancellation-reason-storage/cancellation-reasons";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_circulation_storage.check_in";
    table.name = "circulation_check_ins";
    table.source_spec = "/check-in-storage/check-ins";
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

    table.direct_source_table = "mod_circulation_storage.fixed_due_date_schedule";
    table.name = "circulation_fixed_due_date_schedules";
    table.source_spec = "/fixed-due-date-schedule-storage/fixed-due-date-schedules";
    schema->tables.push_back(table);

    // circulation_loan_history

    table.direct_source_table = "mod_circulation_storage.loan_policy";
    table.name = "circulation_loan_policies";
    table.source_spec = "/loan-policy-storage/loan-policies";
    schema->tables.push_back(table);

    // circulation_loans

    // okapi returns 422 Unprocessable Entity
    //table.source_spec = "/patron-action-session-storage/expired-session-patron-ids";
    //table.name = "";
    // schema->tables.push_back(table);

    table.direct_source_table = "mod_circulation_storage.patron_action_session";
    table.name = "circulation_patron_action_sessions";
    table.source_spec = "/patron-action-session-storage/patron-action-sessions";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_circulation_storage.patron_notice_policy";
    table.name = "circulation_patron_notice_policies";
    table.source_spec = "/patron-notice-policy-storage/patron-notice-policies";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_circulation_storage.request_policy";
    table.name = "circulation_request_policies";
    table.source_spec = "/request-policy-storage/request-policies";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_circulation_storage.user_request_preference";
    table.name = "circulation_request_preference";
    table.source_spec = "/request-preference-storage/request-preference";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_circulation_storage.request";
    table.name = "circulation_requests";
    table.source_spec = "/request-storage/requests";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_circulation_storage.scheduled_notice";
    table.name = "circulation_scheduled_notices";
    table.source_spec = "/scheduled-notice-storage/scheduled-notices";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_circulation_storage.staff_slips";
    table.name = "circulation_staff_slips";
    table.source_spec = "/staff-slips-storage/staff-slips";
    schema->tables.push_back(table);

    ///////////////////////////////////////////////////////////////////////////
    table.module_name = "mod-configuration";

    table.anonymize = true;
    table.direct_source_table = "mod_configuration.config_data";
    table.name = "configuration_entries";
    table.source_spec = "/configurations/entries";
    schema->tables.push_back(table);
    table.anonymize = false;

    // No data found in reference environment
    // table.source_spec = "/configurations/audit";
    // table.name = "configuration_audit";
    // schema->tables.push_back(table);

    ///////////////////////////////////////////////////////////////////////////
    table.module_name = "mod-courses";

    table.direct_source_table = "mod_courses.coursereserves_copyrightstates";
    table.name = "course_copyrightstatuses";
    table.source_spec = "/coursereserves/copyrightstatuses";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_courses.coursereserves_courselistings";
    table.name = "course_courselistings";
    table.source_spec = "/coursereserves/courselistings";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_courses.coursereserves_courses";
    table.name = "course_courses";
    table.source_spec = "/coursereserves/courses";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_courses.coursereserves_coursetypes";
    table.name = "course_coursetypes";
    table.source_spec = "/coursereserves/coursetypes";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_courses.coursereserves_departments";
    table.name = "course_departments";
    table.source_spec = "/coursereserves/departments";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_courses.coursereserves_processingstates";
    table.name = "course_processingstatuses";
    table.source_spec = "/coursereserves/processingstatuses";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_courses.coursereserves_reserves";
    table.name = "course_reserves";
    table.source_spec = "/coursereserves/reserves";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_courses.coursereserves_roles";
    table.name = "course_roles";
    table.source_spec = "/coursereserves/roles";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_courses.coursereserves_terms";
    table.name = "course_terms";
    table.source_spec = "/coursereserves/terms";
    schema->tables.push_back(table);

    ///////////////////////////////////////////////////////////////////////////
    table.module_name = "mod-email";

    table.direct_source_table = "mod_email.email_statistics";
    table.name = "email_email";
    table.source_spec = "/email";
    schema->tables.push_back(table);

    ///////////////////////////////////////////////////////////////////////////
    table.module_name = "mod-feesfines";

    // feesfines_accounts

    table.direct_source_table = "mod_feesfines.comments";
    table.name = "feesfines_comments";
    table.source_spec = "/comments";
    schema->tables.push_back(table);

    // feesfines_feefineactions

    table.direct_source_table = "mod_feesfines.feefines";
    table.name = "feesfines_feefines";
    table.source_spec = "/feefines";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_feesfines.lost_item_fee_policy";
    table.name = "feesfines_lost_item_fees_policies";
    table.source_spec = "/lost-item-fees-policies";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_feesfines.manualblocks";
    table.name = "feesfines_manualblocks";
    table.source_spec = "/manualblocks";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_feesfines.overdue_fine_policy";
    table.name = "feesfines_overdue_fines_policies";
    table.source_spec = "/overdue-fines-policies";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_feesfines.owners";
    table.name = "feesfines_owners";
    table.source_spec = "/owners";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_feesfines.payments";
    table.name = "feesfines_payments";
    table.source_spec = "/payments";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_feesfines.refunds";
    table.name = "feesfines_refunds";
    table.source_spec = "/refunds";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_feesfines.transfer_criteria";
    table.name = "feesfines_transfer_criterias";
    table.source_spec = "/transfer-criterias";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_feesfines.transfers";
    table.name = "feesfines_transfers";
    table.source_spec = "/transfers";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_feesfines.waives";
    table.name = "feesfines_waives";
    table.source_spec = "/waives";
    schema->tables.push_back(table);

    ///////////////////////////////////////////////////////////////////////////
    table.module_name = "mod-finance-storage";

    table.direct_source_table = "mod_finance_storage.budget";
    table.name = "finance_budgets";
    table.source_spec = "/finance-storage/budgets";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_finance_storage.expense_class";
    table.name = "finance_expense_classes";
    table.source_spec = "/finance-storage/expense-classes";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_finance_storage.fiscal_year";
    table.name = "finance_fiscal_years";
    table.source_spec = "/finance-storage/fiscal-years";
    schema->tables.push_back(table);

    // Removed for lack of data
    //table.source_spec = "/finance-storage/fund-distributions";
    //table.name = "finance_fund_distributions";
    //schema->tables.push_back(table);

    table.direct_source_table = "mod_finance_storage.fund_type";
    table.name = "finance_fund_types";
    table.source_spec = "/finance-storage/fund-types";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_finance_storage.fund";
    table.name = "finance_funds";
    table.source_spec = "/finance-storage/funds";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_finance_storage.group_fund_fiscal_year";
    table.name = "finance_group_fund_fiscal_years";
    table.source_spec = "/finance-storage/group-fund-fiscal-years";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_finance_storage.groups";
    table.name = "finance_groups";
    table.source_spec = "/finance-storage/groups";
    schema->tables.push_back(table);

    // table.source_spec = "/finance-storage/ledger-fiscal-years";
    // table.name = "finance_ledger_fiscal_years";
    // schema->tables.push_back(table);

    table.direct_source_table = "mod_finance_storage.ledger";
    table.name = "finance_ledgers";
    table.source_spec = "/finance-storage/ledgers";
    schema->tables.push_back(table);

    // finance_transactions

    ///////////////////////////////////////////////////////////////////////////
    table.module_name = "mod-inventory-storage";

    table.direct_source_table = "mod_inventory_storage.alternative_title_type";
    table.name = "inventory_alternative_title_types";
    table.source_spec = "/alternative-title-types";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_inventory_storage.call_number_type";
    table.name = "inventory_call_number_types";
    table.source_spec = "/call-number-types";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_inventory_storage.loccampus";
    table.name = "inventory_campuses";
    table.source_spec = "/location-units/campuses";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_inventory_storage.classification_type";
    table.name = "inventory_classification_types";
    table.source_spec = "/classification-types";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_inventory_storage.contributor_name_type";
    table.name = "inventory_contributor_name_types";
    table.source_spec = "/contributor-name-types";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_inventory_storage.contributor_type";
    table.name = "inventory_contributor_types";
    table.source_spec = "/contributor-types";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_inventory_storage.electronic_access_relationship";
    table.name = "inventory_electronic_access_relationships";
    table.source_spec = "/electronic-access-relationships";
    schema->tables.push_back(table);

    // inventory_holdings

    table.direct_source_table = "mod_inventory_storage.holdings_note_type";
    table.name = "inventory_holdings_note_types";
    table.source_spec = "/holdings-note-types";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_inventory_storage.holdings_type";
    table.name = "inventory_holdings_types";
    table.source_spec = "/holdings-types";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_inventory_storage.identifier_type";
    table.name = "inventory_identifier_types";
    table.source_spec = "/identifier-types";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_inventory_storage.ill_policy";
    table.name = "inventory_ill_policies";
    table.source_spec = "/ill-policies";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_inventory_storage.instance_format";
    table.name = "inventory_instance_formats";
    table.source_spec = "/instance-formats";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_inventory_storage.instance_note_type";
    table.name = "inventory_instance_note_types";
    table.source_spec = "/instance-note-types";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_inventory_storage.instance_relationship_type";
    table.name = "inventory_instance_relationship_types";
    table.source_spec = "/instance-relationship-types";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_inventory_storage.instance_relationship";
    table.name = "inventory_instance_relationships";
    table.source_spec = "/instance-storage/instance-relationships";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_inventory_storage.instance_status";
    table.name = "inventory_instance_statuses";
    table.source_spec = "/instance-statuses";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_inventory_storage.instance_type";
    table.name = "inventory_instance_types";
    table.source_spec = "/instance-types";
    schema->tables.push_back(table);

    // inventory_instances

    table.direct_source_table = "mod_inventory_storage.item_damaged_status";
    table.name = "inventory_item_damaged_statuses";
    table.source_spec = "/item-damaged-statuses";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_inventory_storage.item_note_type";
    table.name = "inventory_item_note_types";
    table.source_spec = "/item-note-types";
    schema->tables.push_back(table);

    // inventory_items

    table.direct_source_table = "mod_inventory_storage.locinstitution";
    table.name = "inventory_institutions";
    table.source_spec = "/location-units/institutions";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_inventory_storage.loclibrary";
    table.name = "inventory_libraries";
    table.source_spec = "/location-units/libraries";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_inventory_storage.loan_type";
    table.name = "inventory_loan_types";
    table.source_spec = "/loan-types";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_inventory_storage.location";
    table.name = "inventory_locations";
    table.source_spec = "/locations";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_inventory_storage.material_type";
    table.name = "inventory_material_types";
    table.source_spec = "/material-types";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_inventory_storage.mode_of_issuance";
    table.name = "inventory_modes_of_issuance";
    table.source_spec = "/modes-of-issuance";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_inventory_storage.nature_of_content_term";
    table.name = "inventory_nature_of_content_terms";
    table.source_spec = "/nature-of-content-terms";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_inventory_storage.service_point";
    table.name = "inventory_service_points";
    table.source_spec = "/service-points";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_inventory_storage.service_point_user";
    table.name = "inventory_service_points_users";
    table.source_spec = "/service-points-users";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_inventory_storage.statistical_code_type";
    table.name = "inventory_statistical_code_types";
    table.source_spec = "/statistical-code-types";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_inventory_storage.statistical_code";
    table.name = "inventory_statistical_codes";
    table.source_spec = "/statistical-codes";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_inventory_storage.holdings_records_source";
    table.name = "inventory_holdings_sources";
    table.source_spec = "/holdings-sources";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_inventory_storage.bound_with_part";
    table.name = "inventory_bound_with_part";
    table.source_spec = "/inventory-storage/bound-with-parts";
    schema->tables.push_back(table);

    ///////////////////////////////////////////////////////////////////////////
    table.module_name = "mod-invoice-storage";

    // okapi returns 400 Bad Request
    //table.source_spec = "/invoice-storage/invoice-line-number";
    //table.name = "invoice_line_number";
    //schema->tables.push_back(table);

    table.direct_source_table = "mod_invoice_storage.invoices";
    table.name = "invoice_invoices";
    table.source_spec = "/invoice-storage/invoices";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_invoice_storage.invoice_lines";
    table.name = "invoice_lines";
    table.source_spec = "/invoice-storage/invoice-lines";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_invoice_storage.voucher_lines";
    table.name = "invoice_voucher_lines";
    table.source_spec = "/voucher-storage/voucher-lines";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_invoice_storage.vouchers";
    table.name = "invoice_vouchers";
    table.source_spec = "/voucher-storage/vouchers";
    schema->tables.push_back(table);

    ///////////////////////////////////////////////////////////////////////////
    table.module_name = "mod-notes";

    table.source_type = data_source_type::notes;
    table.anonymize = true;
    table.direct_source_table = "mod_notes.note";
    table.name = "notes";
    table.source_spec = "/notes";
    schema->tables.push_back(table);
    table.anonymize = false;
    table.source_type = data_source_type::rmb;

    ///////////////////////////////////////////////////////////////////////////
    table.module_name = "mod-orders-storage";

    table.direct_source_table = "mod_orders_storage.acquisitions_unit_membership";
    table.name = "acquisitions_memberships";
    table.source_spec = "/acquisitions-units-storage/memberships";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_orders_storage.acquisitions_unit";
    table.name = "acquisitions_units";
    table.source_spec = "/acquisitions-units-storage/units";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_orders_storage.alert";
    table.name = "po_alerts";
    table.source_spec = "/orders-storage/alerts";
    schema->tables.push_back(table);

    // po_lines

    table.direct_source_table = "mod_orders_storage.order_invoice_relationship";
    table.name = "po_order_invoice_relns";
    table.source_spec = "/orders-storage/order-invoice-relns";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_orders_storage.order_templates";
    table.name = "po_order_templates";
    table.source_spec = "/orders-storage/order-templates";
    schema->tables.push_back(table);

    // po_pieces

    // okapi returns 400 Bad Request
    //table.source_spec = "/orders-storage/po-line-number";
    //table.name = "";
    //schema->tables.push_back(table);

    // po_purchase_orders

    // po_receiving_history

    table.direct_source_table = "mod_orders_storage.reporting_code";
    table.name = "po_reporting_codes";
    table.source_spec = "/orders-storage/reporting-codes";
    schema->tables.push_back(table);

    ///////////////////////////////////////////////////////////////////////////
    table.module_name = "mod-organizations-storage";

    table.direct_source_table = "mod_organizations_storage.addresses";
    table.name = "organization_addresses";
    table.source_spec = "/organizations-storage/addresses";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_organizations_storage.categories";
    table.name = "organization_categories";
    table.source_spec = "/organizations-storage/categories";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_organizations_storage.contacts";
    table.name = "organization_contacts";
    table.source_spec = "/organizations-storage/contacts";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_organizations_storage.emails";
    table.name = "organization_emails";
    table.source_spec = "/organizations-storage/emails";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_organizations_storage.interfaces";
    table.name = "organization_interfaces";
    table.source_spec = "/organizations-storage/interfaces";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_organizations_storage.organizations";
    table.name = "organization_organizations";
    table.source_spec = "/organizations-storage/organizations";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_organizations_storage.phone_numbers";
    table.name = "organization_phone_numbers";
    table.source_spec = "/organizations-storage/phone-numbers";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_organizations_storage.urls";
    table.name = "organization_urls";
    table.source_spec = "/organizations-storage/urls";
    schema->tables.push_back(table);

    ///////////////////////////////////////////////////////////////////////////
    table.module_name = "mod-source-record-storage";

    // srs_marc

    // srs_records

    ///////////////////////////////////////////////////////////////////////////
    table.module_name = "mod-users";

    table.direct_source_table = "mod_users.addresstype";
    table.name = "user_addresstypes";
    table.source_spec = "/addresstypes";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_users.departments";
    table.name = "user_departments";
    table.source_spec = "/departments";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_users.groups";
    table.name = "user_groups";
    table.source_spec = "/groups";
    schema->tables.push_back(table);

    table.direct_source_table = "mod_users.proxyfor";
    table.name = "user_proxiesfor";
    table.source_spec = "/proxiesfor";
    schema->tables.push_back(table);

    ///////////////////////////////////////////////////////////////////////////
    table.module_name = "mod-template-engine";

    table.direct_source_table = "mod_template_engine.template";
    table.name = "template_engine_template";
    table.source_spec = "/templates";
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
    if (counts.string > 0 && counts.boolean > 0) {
        // A warning is sufficient here, because Boolean values can be
        // converted later in json_value_to_string().  The same could be done
        // for numbers, but all number types would need to be handled and
        // tested.
        lg->write(log_level::warning, "", "", "inconsistent data types: table="+table+" field="+field+" types=boolean,string", -1);
    }
    if (counts.string > 0 && counts.number > 0) {
        lg->write(log_level::error, "", "", "inconsistent data types: table="+table+" field="+field+" types=number,string", -1);
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
