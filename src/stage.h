#ifndef LDP_STAGE_H
#define LDP_STAGE_H

#include "options.h"
#include "util.h"

//bool stage_table(const ldp_options& opt,
//                 ldp_log* lg, table_schema* table, etymon::odbc_env* odbc,
//                 etymon::odbc_conn* conn, dbtype* dbt, const string& loadDir,
//                 bool anonymize_fields);

bool stage_table_1(const ldp_options& opt,
                   const vector<source_state>& source_states,
                 ldp_log* lg, table_schema* table, etymon::odbc_env* odbc,
                 etymon::odbc_conn* conn, dbtype* dbt, const string& loadDir,
                 bool anonymize_fields);

bool stage_table_2(const ldp_options& opt,
                   const vector<source_state>& source_states,
                 ldp_log* lg, table_schema* table, etymon::odbc_env* odbc,
                 etymon::odbc_conn* conn, dbtype* dbt, const string& loadDir,
                 bool anonymize_fields);

void index_loaded_table(ldp_log* lg, const table_schema& table,
                        etymon::odbc_conn* conn, dbtype* dbt);

#endif

