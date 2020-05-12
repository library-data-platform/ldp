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
            const string& tempPath, ExtractionFiles* tempFiles);
    ~IDMap();
    void makeSK(const string& table, const char* id, string* sk);
    void syncCommit();
private:
    etymon::OdbcDbc* dbc;
    DBType* dbt;
    etymon::OdbcTx* tx;
    Log* log;
    etymon::Sqlite3* sqlite;
    int64_t nextvalSK;
};

#endif

