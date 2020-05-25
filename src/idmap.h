#ifndef LDP_IDMAP_H
#define LDP_IDMAP_H

#include <string>

#include "../etymoncpp/include/odbc.h"
#include "../etymoncpp/include/sqlite3.h"
#include "dbtype.h"
#include "extract.h"
#include "log.h"

class IDMap {
public:
    IDMap(etymon::OdbcEnv* odbc, const string& databaseDSN, Log* log,
            const string& tempPath, const string& datadir);
    ~IDMap();
    void makeSK(const string& table, const char* id, string* sk);
    void syncCommit();
    void vacuum();
    static void addIndexes(etymon::OdbcDbc* conn, Log* log);
    static void removeIndexes(etymon::OdbcDbc* conn, Log* log);
    static void schemaUpgradeRemoveNewColumn(const string& datadir);
private:
    void syncDown();
    void syncUp();
    int64_t ldpSelectMaxSK();
    int64_t cacheSelectMaxSK();
    void down(int64_t startSK);
    void up(int64_t startSK);
    void openCache(const string& filename);
    void createCache(const string& cacheFile);
    etymon::OdbcDbc* dbc;
    DBType* dbt;
    Log* log;
    etymon::sqlite_db* cache;
    int64_t nextvalSK;
#ifdef PERF
    double makeSKTime;
#endif
};

#endif

