#include <cstdint>
#include <experimental/filesystem>
#include <fstream>
#include <iostream>
#include <sqlite3.h>
#include <sstream>
#include <stdexcept>

#include "../etymoncpp/include/util.h"
#include "idmap.h"

namespace fs = std::experimental::filesystem;

//static int callback(void* NotUsed, int argc, char** argv, char** azColName)
//{
//    return 0;
//}

static int lookupSK(void *data, int argc, char **argv, char **azColName){
    *((string*) data) = argv[0];
    return 0;
}

class SyncData {
public:
    vector<pair<string, string>> data;
    etymon::OdbcDbc* dbc;
    Log* log;
    SyncData(etymon::OdbcDbc* dbc, Log* log) : dbc(dbc), log(log) {}
    void sync();
};

void SyncData::sync()
{
    if (data.size() == 0)
        return;
    string sql = "INSERT INTO ldpsystem.idmap (sk, id) VALUES";
    bool first = true;
    for (auto& d : data) {
        if (first)
            first = false;
        else
            sql += ",";
        sql += "\n(" + d.first + ",'" + d.second + "')";
        //printf(">> sk = %s; id = %s\n", d.first.c_str(), d.second.c_str());
    }
    sql += ";";
    //printf("%s\n", sql.c_str());
    log->logDetail("INSERT INTO ldpsystem.idmap (sk, id) VALUES (...");
    dbc->execDirect(nullptr, sql);
    //printf("%lu record synced\n", data.size());
    data.clear();
}

static int selectAllNew(void* data, int argc, char** argv, char** azColName)
{
    pair<string, string> p(argv[1], argv[0]);
    SyncData* syncData = (SyncData*) data;
    syncData->data.push_back(p);
    if (syncData->data.size() % 200000 == 0)
        syncData->sync();
    return 0;
}

static int selectMaxSK(void* data, int argc, char** argv, char** azColName)
{
    *((string*) data) = (argv[0] == nullptr ? "0" : argv[0]);
    return 0;
}

/*
void IDMap::createCache(const string& cacheFile)
{
    etymon::Sqlite3 sqlite(cacheFile.c_str());

    char *zErrMsg = 0;
    int rc;
    string sql;

    sql = "PRAGMA journal_mode = OFF;";
    rc = sqlite3_exec(sqlite.db, sql.c_str(), callback, 0, &zErrMsg);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    sql = "PRAGMA synchronous = OFF;";
    rc = sqlite3_exec(sqlite.db, sql.c_str(), callback, 0, &zErrMsg);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    sql = "PRAGMA locking_mode = EXCLUSIVE;";
    rc = sqlite3_exec(sqlite.db, sql.c_str(), callback, 0, &zErrMsg);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }

    sql =
        "CREATE TABLE idmap_cache (\n"
        "    id VARCHAR(65535) NOT NULL,\n"
        "    sk BIGINT NOT NULL,\n"
        "    new INTEGER NOT NULL,\n"
        "    PRIMARY KEY (id),\n"
        "    UNIQUE (sk)\n"
        ");";
    rc = sqlite3_exec(sqlite.db, sql.c_str(), callback, 0, &zErrMsg);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }

    sql = "BEGIN;";
    rc = sqlite3_exec(sqlite.db, sql.c_str(), callback, 0, &zErrMsg);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }

    sql = "SELECT sk, id FROM ldpsystem.idmap;";
    log->logDetail(sql);
    {
        etymon::OdbcStmt stmt(dbc);
        dbc->execDirect(&stmt, sql);
        string sk, id;
        //printf("Caching idmap\n");
        while (dbc->fetch(&stmt)) {
            dbc->getData(&stmt, 1, &sk);
            dbc->getData(&stmt, 2, &id);
            etymon::toLower(&id);
            //int64_t skl = stol(sk);
            int64_t skl;
            {
                stringstream stream(sk);
                stream >> skl;
            }
            if (skl >= nextvalSK)
                nextvalSK = skl + 1;
            //printf("(%s, %s)\n", sk.c_str(), id.c_str());
            sql =
                "INSERT INTO idmap_cache (id, sk, new)\n"
                "    VALUES ('" + id + "', " + sk + ", 0);";
            rc = sqlite3_exec(sqlite.db, sql.c_str(), callback, 0, &zErrMsg);
            if( rc != SQLITE_OK ){
                fprintf(stderr, "SQL error: %s\n", zErrMsg);
                sqlite3_free(zErrMsg);
            }
        }
    }

    sql = "COMMIT;";
    rc = sqlite3_exec(sqlite.db, sql.c_str(), callback, 0, &zErrMsg);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }

}
*/

static void makeCachePath(const string& datadir, string* path)
{
    fs::path dd = datadir;
    fs::path cachedir = dd / "cache";
    fs::create_directories(cachedir);
    fs::path cachepath = cachedir / "idmap.db";
    *path = cachepath;
}

void IDMap::schemaUpgradeRemoveNewColumn(const string& datadir)
{
    string filename;
    makeCachePath(datadir, &filename);
    if (!fs::exists(filename))
        return;
    etymon::Sqlite3 cache(filename);
    string sql = "PRAGMA synchronous = EXTRA;";
    cache.exec(sql);
    sql = "PRAGMA locking_mode = EXCLUSIVE;";
    cache.exec(sql);
    sql = "BEGIN;";
    cache.exec(sql);
    sql = "ALTER TABLE idmap_cache RENAME TO old_idmap;";
    cache.exec(sql);
    sql =
        "CREATE TABLE idmap_cache (\n"
        "    id VARCHAR(65535) NOT NULL,\n"
        "    sk BIGINT NOT NULL,\n"
        "    PRIMARY KEY (id),\n"
        "    UNIQUE (sk)\n"
        ");";
    cache.exec(sql);
    sql = "INSERT INTO idmap_cache SELECT id, sk FROM old_idmap;";
    cache.exec(sql);
    sql = "DROP TABLE old_idmap;";
    cache.exec(sql);
    sql = "COMMIT;";
    cache.exec(sql);
    sql = "VACUUM;";
    cache.exec(sql);
}

int64_t IDMap::ldpSelectMaxSK()
{
    string sql = "SELECT MAX(sk) FROM ldpsystem.idmap;";
    log->detail(sql);
    etymon::OdbcStmt stmt(dbc);
    dbc->execDirect(&stmt, sql);
    int64_t maxSK = 0;
    if (dbc->fetch(&stmt)) {
        string msk;
        dbc->getData(&stmt, 1, &msk);
        {
            stringstream stream(msk);
            stream >> maxSK;
        }
    }
    return maxSK;
}

int64_t IDMap::cacheSelectMaxSK()
{
    string sql = "SELECT MAX(sk) FROM idmap_cache;";
    log->detail(sql);
    string msk;
    cache->exec(sql, selectMaxSK, (void*) &msk);
    int64_t maxSK = 0;
    if (msk != "") {
        {
            stringstream stream(msk);
            stream >> maxSK;
        }
    }
    return maxSK;
}

void IDMap::down(int64_t startSK)
{
    string sql = "BEGIN;";
    cache->exec(sql);
    sql =
        "SELECT sk, id FROM ldpsystem.idmap WHERE sk >= " +
        to_string(startSK) + ";";
    log->detail(sql);
    {
        etymon::OdbcStmt stmt(dbc);
        dbc->execDirect(&stmt, sql);
        string sk, id;
        while (dbc->fetch(&stmt)) {
            dbc->getData(&stmt, 1, &sk);
            dbc->getData(&stmt, 2, &id);
            int64_t skl;
            {
                stringstream stream(sk);
                stream >> skl;
            }
            //if (skl >= nextvalSK)
            //    nextvalSK = skl + 1;
            sql =
                "INSERT INTO idmap_cache (id, sk)\n"
                "    VALUES ('" + id + "', " + sk + ");";
            cache->exec(sql);
        }
    }
    sql = "COMMIT;";
    cache->exec(sql);
}

void IDMap::up(int64_t startSK)
{
    etymon::OdbcTx tx(dbc);
    string sql = "SELECT id, sk FROM idmap_cache WHERE sk >= " +
        to_string(startSK) + ";";
    log->detail(sql);
    SyncData syncData(dbc, log);
    cache->exec(sql, selectAllNew, (void*) &syncData);
    syncData.sync();  // Sync any remaining data.
    tx.commit();
}

void IDMap::syncUp()
{
    int64_t ldpMaxSK = ldpSelectMaxSK();
    log->detail("IDMap::sync(): LDP max(sk) = " + to_string(ldpMaxSK));
    int64_t cacheMaxSK = cacheSelectMaxSK();
    log->detail("IDMap::sync(): Cache max(sk) = " + to_string(cacheMaxSK));

    if (ldpMaxSK == cacheMaxSK) {
        log->trace("Synchronizing cache: idmap: 0 rows");
        nextvalSK = ldpMaxSK + 1;
        return;
    }

    if (ldpMaxSK > cacheMaxSK) {
        throw runtime_error("Cache out of sync with remote database");
    } else {
        log->trace("Synchronizing cache: idmap: sending " +
                to_string(cacheMaxSK - ldpMaxSK) + " rows");
        up(ldpMaxSK + 1);
        nextvalSK = cacheMaxSK + 1;
    }
}

void IDMap::syncDown()
{
    int64_t ldpMaxSK = ldpSelectMaxSK();
    log->detail("IDMap::sync(): LDP max(sk) = " + to_string(ldpMaxSK));
    int64_t cacheMaxSK = cacheSelectMaxSK();
    log->detail("IDMap::sync(): Cache max(sk) = " + to_string(cacheMaxSK));

    if (ldpMaxSK == cacheMaxSK) {
        log->trace("Synchronizing cache: idmap: 0 rows");
        nextvalSK = ldpMaxSK + 1;
        return;
    }

    if (ldpMaxSK > cacheMaxSK) {
        log->trace("Synchronizing cache: idmap: receiving " +
                to_string(ldpMaxSK - cacheMaxSK) + " rows");
        down(cacheMaxSK + 1);
        nextvalSK = ldpMaxSK + 1;
    } else {
        log->trace("Synchronizing cache: idmap: clearing " +
                to_string(cacheMaxSK - ldpMaxSK) + " rows");
        string sql = "DELETE FROM idmap_cache WHERE sk > " +
            to_string(ldpMaxSK) + ";";
        log->detail(sql);
        cache->exec(sql);
        nextvalSK = ldpMaxSK + 1;
    }
}

IDMap::IDMap(etymon::OdbcEnv* odbc, const string& databaseDSN, Log* log,
        const string& tempPath, const string& datadir)
{
    nextvalSK = 1;
    dbc = new etymon::OdbcDbc(odbc, databaseDSN);
    dbt = new DBType(dbc);
    this->log = log;
    //tx = new etymon::OdbcTx(dbc);

    string filename;
    makeCachePath(datadir, &filename);
    cache = new etymon::Sqlite3(filename);
    string sql = "PRAGMA synchronous = EXTRA;";
    log->detail(sql);
    cache->exec(sql);
    sql = "PRAGMA locking_mode = EXCLUSIVE;";
    log->detail(sql);
    cache->exec(sql);
    sql =
        "CREATE TABLE IF NOT EXISTS idmap_cache (\n"
        "    id VARCHAR(65535) NOT NULL,\n"
        "    sk BIGINT NOT NULL,\n"
        "    PRIMARY KEY (id),\n"
        "    UNIQUE (sk)\n"
        ");";
    log->detail(sql);
    cache->exec(sql);

    syncDown();

    //if (!create) {
    //    string sql = "SELECT MAX(sk) FROM idmap_cache;";
    //    log->logDetail(sql);
    //    char *zErrMsg = 0;
    //    string msk;
    //    int rc = sqlite3_exec(cache->db, sql.c_str(), selectMaxSK,
    //            (void*) &msk, &zErrMsg);
    //    if( rc != SQLITE_OK ) {
    //        fprintf(stderr, "SQL error: %s\n", zErrMsg);
    //        sqlite3_free(zErrMsg);
    //    }
    //    int64_t maxSK;
    //    {
    //        stringstream stream(msk);
    //        stream >> maxSK;
    //    }
    //    nextvalSK = maxSK + 1;
    //}
    //printf("nextvalSK = %llu\n", nextvalSK);
}

IDMap::~IDMap()
{
    //delete tx;
    delete dbt;
    delete dbc;
    delete cache;
    //{ ofstream output(cacheSync); }
}

void IDMap::makeSK(const string& table, const char* id, string* sk)
{
    string qid = id;
    etymon::toLower(&qid);
    string sql =
        "SELECT sk FROM idmap_cache\n"
        "    WHERE id = '" + qid + "';";
    log->detail(sql);
    *sk = "";
    cache->exec(sql, lookupSK, (void*) sk);
    //printf("SK = %s\n", sk->c_str());
    //if (*sk == "") {
    //    fprintf(stderr, "sk not found\n");
    //    fprintf(stderr, "%s\n", qid.c_str());
    //}
    if (*sk != "")
        return;
    // The sk was not found; so we add it.
    *sk = to_string(nextvalSK);
    nextvalSK++;
    sql =
        "INSERT INTO idmap_cache (id, sk)\n"
        "    VALUES ('" + string(id) + "', " + *sk + ");";
    log->detail(sql);
    cache->exec(sql);
    //printf("(%s, %s)\n", sk->c_str(), id);
}

void IDMap::syncCommit()
{
    syncUp();
}

void IDMap::vacuum()
{
    log->trace("Rewriting cache: idmap");
    string sql = "VACUUM;";
    log->detail(sql);
    cache->exec(sql);
}

//void IDMap::syncCommit()
//{
//    string sql = "SELECT id, sk FROM idmap_cache WHERE new = 1;";
//    log->logDetail(sql);
//    char *zErrMsg = 0;
//    SyncData syncData(dbc, log);
//    int rc = sqlite3_exec(cache->db, sql.c_str(), selectAllNew,
//            (void*) &syncData, &zErrMsg);
//    if( rc != SQLITE_OK ) {
//        fprintf(stderr, "SQL error: %s\n", zErrMsg);
//        sqlite3_free(zErrMsg);
//    }
//    syncData.sync();  // Sync any remaining data.
//    sql = "UPDATE idmap_cache SET new = 0;";
//    rc = sqlite3_exec(cache->db, sql.c_str(), callback, 0, &zErrMsg);
//    if (rc != SQLITE_OK){
//        fprintf(stderr, "SQL error: %s\n", zErrMsg);
//        //throw runtime_error("Unable to map ID: " + string(id));
//        sqlite3_free(zErrMsg);
//    }
//    tx->commit();
//}

