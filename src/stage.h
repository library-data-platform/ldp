#ifndef LDP_STAGE_H
#define LDP_STAGE_H

#include "anonymize.h"
#include "options.h"
#include "util.h"

void encode_json(const char* str, string* newstr);

bool stage_table_1(const ldp_options& opt,
    const vector<source_state>& source_states,
    ldp_log* lg, table_schema* table,
    etymon::pgconn* conn, dbtype* dbt, const string& loadDir,
    field_set* drop_fields,
    char* read_buffer,
    vector<string>* users,
    bool lz4);

bool stage_table_2(const ldp_options& opt,
    const vector<source_state>& source_states,
    ldp_log* lg, table_schema* table,
    etymon::pgconn* conn, dbtype* dbt, const string& loadDir,
    field_set* drop_fields,
    char* read_buffer);

void add_pkey_and_indexes(ldp_log* lg, const table_schema& table, etymon::pgconn* conn, dbtype* dbt, bool create_indexes);

#endif

