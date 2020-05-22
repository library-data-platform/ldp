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
    log->detail(sql + " ...");
    bool first = true;
    for (auto& d : data) {
        if (first)
            first = false;
        else
            sql += ",";
        sql += "\n(" + d.first + ",'" + d.second + "')";
    }
    sql += ";";
    dbc->execDirect(nullptr, sql);
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
    string sql =
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
            sql =
                "INSERT INTO idmap_cache (id, sk)\n"
                "    VALUES ('" + id + "', " + sk + ");";
            cache->exec(sql);
        }
    }
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
    int64_t cacheMaxSK = cacheSelectMaxSK();
    int64_t ldpMaxSK = ldpSelectMaxSK();
    log->detail("Cache (idmap): max sk = " + to_string(cacheMaxSK) +
            " (" + to_string(ldpMaxSK) + ")");

    if (ldpMaxSK == cacheMaxSK) {
        nextvalSK = ldpMaxSK + 1;
        return;
    }

    if (ldpMaxSK > cacheMaxSK) {
        throw runtime_error("Cache (idmap) out of sync with remote database");
    } else {
        log->trace("Cache (idmap): sync up: " +
                to_string(cacheMaxSK - ldpMaxSK));
        up(ldpMaxSK + 1);
        nextvalSK = cacheMaxSK + 1;
    }
}

void IDMap::syncDown()
{
    int64_t cacheMaxSK = cacheSelectMaxSK();
    int64_t ldpMaxSK = ldpSelectMaxSK();
    log->detail("Cache (idmap): max sk = " + to_string(cacheMaxSK) +
            " (" + to_string(ldpMaxSK) + ")");

    if (ldpMaxSK == cacheMaxSK) {
        nextvalSK = ldpMaxSK + 1;
        return;
    }

    if (ldpMaxSK > cacheMaxSK) {
        log->trace("Cache (idmap): sync down: " +
                to_string(ldpMaxSK - cacheMaxSK));
        down(cacheMaxSK + 1);
        nextvalSK = ldpMaxSK + 1;
    } else {
        log->trace("Cache (idmap): clear: " + to_string(cacheMaxSK - ldpMaxSK));
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

    sql = "BEGIN;";
    cache->exec(sql);
    syncDown();
    sql = "COMMIT;";
    cache->exec(sql);

    sql = "BEGIN;";
    cache->exec(sql);
}

IDMap::~IDMap()
{
    delete dbt;
    delete dbc;
    delete cache;
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
}

void IDMap::syncCommit()
{
    string sql = "COMMIT;";
    cache->exec(sql);
    syncUp();
}

void IDMap::vacuum()
{
    log->trace("Rewriting cache: idmap");
    string sql = "VACUUM;";
    log->detail(sql);
    cache->exec(sql);
}

