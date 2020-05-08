#ifndef LDP_IDMAP_H
#define LDP_IDMAP_H

#include <string>

#include "../etymoncpp/include/odbc.h"
#include "dbtype.h"
#include "log.h"

using namespace std;

class IDMap {
public:
    IDMap(etymon::OdbcEnv* odbc, const string& dataSourceName, Log* log);
    void makeSK(const char* id, string* sk);
    ~IDMap();
private:
    etymon::OdbcDbc* dbc;
    DBType* dbt;
    Log* log;
};

#endif

