#ifndef LDP_STAGE_H
#define LDP_STAGE_H

#include "options.h"
#include "../etymoncpp/include/postgres.h"

void stageTable(const Options& opt, TableSchema* table, etymon::Postgres* db,
        const string& loadDir);

//void stageAll(const Options& o, Schema* schema, etymon::Postgres* db,
//        const string& loadDir);

#endif

