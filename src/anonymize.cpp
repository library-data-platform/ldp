#include <set>

#include "anonymize.h"

static set<pair<string,string>> personal_data_fields = {
    {"circulation_loans", "/userId"}
};

bool is_personal_data_field(const table_schema& table, const string& field)
{
    pair<string,string> p = pair<string,string>(table.name, field);
    return (personal_data_fields.find(p) != personal_data_fields.end());
}

