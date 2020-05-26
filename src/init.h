#ifndef LDP_INIT_H
#define LDP_INIT_H

#include "log.h"

void init_upgrade(etymon::OdbcEnv* odbc, const string& dbname,
        const string& ldpUser, const string& ldpconfigUser,
        const string& datadir, Log* lg);

#endif

