#ifndef LDP_STAGE_H
#define LDP_STAGE_H

#include "options.h"
#include "../etymoncpp/include/postgres.h"

void stageAll(const Options& o, Schema* schema, etymon::Postgres* db,
        const string& loadDir);

#endif

