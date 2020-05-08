#ifndef LDP_INIT_H
#define LDP_INIT_H

//#include <string>

#include "dbcontext.h"

void initUpgrade(etymon::OdbcEnv* odbc, const string& dsn, DBContext* db,
        const string& ldpUser);

#endif

