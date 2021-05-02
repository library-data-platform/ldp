#ifndef LDP_STAGE_H
#define LDP_STAGE_H

#include "anonymize.h"
#include "options.h"
#include "util.h"

bool stage_table_1(const ldp_options& opt,
                   const vector<source_state>& source_states,
                   ldp_log* lg, table_schema* table, etymon::odbc_env* odbc,
                   etymon::odbc_conn* conn, dbtype* dbt, const string& loadDir,
                   field_set* drop_fields);

bool stage_table_2(const ldp_options& opt,
                   const vector<source_state>& source_states,
                   ldp_log* lg, table_schema* table, etymon::odbc_env* odbc,
                   etymon::odbc_conn* conn, dbtype* dbt, const string& loadDir,
                   field_set* drop_fields);

void index_loaded_table(ldp_log* lg, const table_schema& table, etymon::odbc_conn* conn, dbtype* dbt, bool index_large_varchar);

#endif

