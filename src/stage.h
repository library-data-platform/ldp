#ifndef LDP_STAGE_H
#define LDP_STAGE_H

#include "../etymoncpp/include/postgres.h"
#include "options.h"

bool stageTable(const options& opt, Log* log, TableSchema* table,
        etymon::odbc_env* odbc, etymon::odbc_conn* conn, DBType* dbt,
        const string& loadDir);

#endif

