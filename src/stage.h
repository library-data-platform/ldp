#ifndef LDP_STAGE_H
#define LDP_STAGE_H

#include "../etymoncpp/include/postgres.h"
#include "options.h"

void stageTable(const options& opt, Log* log, TableSchema* table,
        etymon::odbc_env* odbc, etymon::odbc_conn* conn, DBType* dbt,
        const string& loadDir);

//void stageAll(const options& o, Schema* schema, etymon::Postgres* db,
//        const string& loadDir);

#endif

