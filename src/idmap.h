#ifndef LDP_IDMAP_H
#define LDP_IDMAP_H

#include <string>

#include "../etymoncpp/include/odbc.h"
#include "../etymoncpp/include/sqlite3.h"
#include "dbtype.h"
#include "extract.h"
#include "log.h"

/**
 * \brief Cache for mapping between identifiers and surrogate keys.
 *
 * Data updates involve many lookups of native identifiers to find or
 * create integer surrogate keys.  The mapping is stored in the main
 * database, but it is also cached by the server.  This class handles
 * cache creation and synchronization, and provides a lookup method.
 *
 * When this class is instantiated, it immediately creates or
 * synchronizes the cache.  The make_sk() method can then be called
 * repeatedly to look up or create surrogate keys.  Finally,
 * sync_commit() should be called to resynchronize the cache with the
 * main database.
 */
class idmap {
public:
    idmap(etymon::OdbcEnv* odbc, const string& dbname, Log* log,
            const string& datadir);
    ~idmap();
    void make_sk(const string& table, const char* id, string* sk);
    void syncCommit();
    void vacuum();
    static void addIndexes(etymon::OdbcDbc* conn, Log* lg);
    static void removeIndexes(etymon::OdbcDbc* conn, Log* lg);
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
    double make_sk_time;
#endif
};

#endif

