#ifndef LDP_STAGE_H
#define LDP_STAGE_H

#include "../etymoncpp/include/postgres.h"
#include "options.h"

bool stageTable(const ldp_options& opt, ldp_log* lg, TableSchema* table,
        etymon::odbc_env* odbc, etymon::odbc_conn* conn, dbtype* dbt,
                const string& loadDir, bool anonymize_fields);

#endif

