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
    void sync();
    void makeSK(const string& table, const char* id, string* sk);
    //void syncCommit();
    static void schemaUpgradeRemoveNewColumn(const string& datadir);
private:
    int64_t ldpSelectMaxSK();
    int64_t cacheSelectMaxSK();
    void syncPull(int64_t startSK);
    void syncPush(int64_t startSK);
    void openCache(const string& filename);
    void createCache(const string& cacheFile);
    etymon::OdbcDbc* dbc;
    DBType* dbt;
    //etymon::OdbcTx* tx;
    Log* log;
    etymon::Sqlite3* cache;
    int64_t nextvalSK;
};

#endif

