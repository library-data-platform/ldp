#ifndef LDP_ANONYMIZE_H
#define LDP_ANONYMIZE_H

#include "schema.h"

bool is_personal_data_field(const table_schema& table, const string& field);

#endif


