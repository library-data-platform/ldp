#ifndef LDP_ANONYMIZE_H
#define LDP_ANONYMIZE_H

#include <set>

#include "schema.h"

class field_set {
public:
    set<pair<string,string>> fields;
    bool find(const string& table, const string& field);
};

void load_anonymize_field_list(field_set* drop_fields);

#endif


