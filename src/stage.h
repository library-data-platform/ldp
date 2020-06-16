#ifndef LDP_STAGE_H
#define LDP_STAGE_H

#include "options.h"

bool stage_table(const ldp_options& opt, const data_source& source,
                 ldp_log* lg, table_schema* table, etymon::odbc_env* odbc,
                 etymon::odbc_conn* conn, dbtype* dbt, const string& loadDir,
                 bool anonymize_fields);

#endif

