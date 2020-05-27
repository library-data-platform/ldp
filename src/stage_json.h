#ifndef LDP_STAGE_H
#define LDP_STAGE_H

#include "../etymoncpp/include/postgres.h"
#include "idmap.h"
#include "options.h"

void stageTable(const Options& opt, Log* log, TableSchema* table,
        etymon::odbc_env* odbc, etymon::odbc_conn* conn, DBType* dbt,
        const string& loadDir, idmap* idm);

//void stageAll(const Options& o, Schema* schema, etymon::Postgres* db,
//        const string& loadDir);

#endif

