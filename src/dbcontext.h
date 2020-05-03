#ifndef LDP_DBCONTEXT_H
#define LDP_DBCONTEXT_H

#include "../etymoncpp/include/odbc.h"
#include "dbtype.h"
#include "log.h"

using namespace std;

class DBContext {
public:
    etymon::OdbcDbc* dbc;
    DBType* dbt;
    Log* log;
    DBContext(etymon::OdbcDbc* dbc, DBType* dbt, Log* log);
};

#endif
