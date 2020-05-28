#ifndef LDP_INIT_H
#define LDP_INIT_H

#include "log.h"

void init_upgrade(etymon::odbc_env* odbc, const string& dbname,
        const string& ldpUser, const string& ldpconfigUser,
        const string& datadir, FILE* err, const char* prog);

#endif

