#include <cstdint>
#include <experimental/filesystem>
#include <fstream>
#include <iostream>
#include <sqlite3.h>
#include <sstream>
#include <stdexcept>

#include "../etymoncpp/include/util.h"
#include "idmap.h"
#include "timer.h"

namespace fs = std::experimental::filesystem;

static void make_cache_path(const string& datadir, string* path)
{
    fs::path dd = datadir;
    fs::path cachedir = dd / "cache";
    fs::create_directories(cachedir);
    fs::path cachepath = cachedir / "idmap.db";
    *path = cachepath;
}

idmap::idmap(etymon::OdbcEnv* odbc, const string& dbname, Log* log,
        const string& datadir)
{
    nextvalSK = 1;
    dbc = new etymon::OdbcDbc(odbc, dbname);
    dbt = new DBType(dbc);
    this->log = log;
#ifdef PERF
    make_sk_time = 0;
#endif

    string filename;
    make_cache_path(datadir, &filename);
    cache = new etymon::sqlite_db(filename);
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

idmap::~idmap()
{
#ifdef PERF
    log->perf("make_sk", make_sk_time);
#endif
    delete dbt;
    delete dbc;
    delete cache;
}

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

void idmap::schemaUpgradeRemoveNewColumn(const string& datadir)
{
    string filename;
    make_cache_path(datadir, &filename);
    if (!fs::exists(filename))
        return;
    etymon::sqlite_db cache(filename);
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

int64_t idmap::ldpSelectMaxSK()
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

int64_t idmap::cacheSelectMaxSK()
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

void idmap::down(int64_t startSK)
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

void idmap::addIndexes(etymon::OdbcDbc* conn, Log* lg)
{
    string sql =
        "ALTER TABLE ldpsystem.idmap\n"
        "    ADD CONSTRAINT idmap_pkey PRIMARY KEY (sk);";
    if (lg != nullptr)
        lg->detail(sql);
    conn->execDirect(nullptr, sql);

    sql =
        "ALTER TABLE ldpsystem.idmap\n"
        "    ADD CONSTRAINT idmap_id_key UNIQUE (id);";
    if (lg != nullptr)
        lg->detail(sql);
    conn->execDirect(nullptr, sql);
}

void idmap::removeIndexes(etymon::OdbcDbc* conn, Log* lg)
{
    string sql =
        "ALTER TABLE ldpsystem.idmap DROP CONSTRAINT idmap_id_key;";
    if (lg != nullptr)
        lg->detail(sql);
    conn->execDirect(nullptr, sql);

    sql =
        "ALTER TABLE ldpsystem.idmap DROP CONSTRAINT idmap_pkey;";
    if (lg != nullptr)
        lg->detail(sql);
    conn->execDirect(nullptr, sql);
}

void idmap::up(int64_t startSK)
{
    string sql = "SELECT id, sk FROM idmap_cache WHERE sk >= " +
        to_string(startSK) + ";";
    log->detail(sql);
    SyncData syncData(dbc, log);
    cache->exec(sql, selectAllNew, (void*) &syncData);
    syncData.sync();  // Sync any remaining data.
}

void idmap::syncUp()
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
        etymon::OdbcTx tx(dbc);
        bool withoutIndexes = (cacheMaxSK - ldpMaxSK) > ldpMaxSK;
        if (withoutIndexes)
            idmap::removeIndexes(dbc, log);
        up(ldpMaxSK + 1);
        if (withoutIndexes)
            idmap::addIndexes(dbc, log);
        nextvalSK = cacheMaxSK + 1;
        tx.commit();
    }
}

void idmap::syncDown()
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

void idmap::make_sk(const string& table, const char* id, string* sk)
{
#ifdef PERF
    Timer timer;
#endif

    string qid = id;
    etymon::toLower(&qid);
    string sql =
        "SELECT sk FROM idmap_cache\n"
        "    WHERE id = '" + qid + "';";
    log->detail(sql);
    *sk = "";
    cache->exec(sql, lookupSK, (void*) sk);
    if (*sk == "") {
        // The sk was not found; so we add it.
        *sk = to_string(nextvalSK);
        nextvalSK++;
        sql =
            "INSERT INTO idmap_cache (id, sk)\n"
            "    VALUES ('" + string(id) + "', " + *sk + ");";
        log->detail(sql);
        cache->exec(sql);
    }

#ifdef PERF
    make_sk_time += timer.elapsedTime();
#endif
}

void idmap::syncCommit()
{
    string sql = "COMMIT;";
    cache->exec(sql);
    syncUp();
}

void idmap::vacuum()
{
    log->trace("Rewriting cache: idmap");
    string sql = "VACUUM;";
    log->detail(sql);
    cache->exec(sql);
}

