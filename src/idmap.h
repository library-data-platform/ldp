#ifndef LDP_IDMAP_H
#define LDP_IDMAP_H

#include <string>

#include "../etymoncpp/include/odbc.h"
#include "../etymoncpp/include/sqlite3.h"
#include "dbtype.h"
#include "extract.h"
#include "log.h"

using namespace std;

class IDMap {
public:
    IDMap(etymon::OdbcEnv* odbc, const string& databaseDSN, Log* log,
            const string& tempPath, const string& datadir);
    ~IDMap();
    void makeSK(const string& table, const char* id, string* sk);
    void syncCommit();
private:
    void createCache(const string& cacheFile);
    etymon::OdbcDbc* dbc;
    DBType* dbt;
    etymon::OdbcTx* tx;
    Log* log;
    string cacheLock;
    etymon::Sqlite3* cacheDB;
    int64_t nextvalSK;
};

#endif

