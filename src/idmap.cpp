#include <sqlite3.h>
#include <stdexcept>

#include "../etymoncpp/include/util.h"
#include "idmap.h"

static int callback(void *NotUsed, int argc, char **argv, char **azColName) {
    //int i;
    //for(i = 0; i<argc; i++) {
    //    printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    //}
    //printf("\n");
    return 0;
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

static int selectAllNew(void *data, int argc, char **argv, char **azColName){
    pair<string, string> p(argv[1], argv[0]);
    SyncData* syncData = (SyncData*) data;
    syncData->data.push_back(p);
    if (syncData->data.size() % 200000 == 0)
        syncData->sync();
    return 0;
}

IDMap::IDMap(etymon::OdbcEnv* odbc, const string& databaseDSN, Log* log,
        const string& tempPath, ExtractionFiles* tempFiles)
{
    nextvalSK = 1;
    dbc = new etymon::OdbcDbc(odbc, databaseDSN);
    dbt = new DBType(dbc);
    this->log = log;
    tx = new etymon::OdbcTx(dbc);

    //sql = "LOCK ldpsystem.idmap;";
    //log->logDetail(sql);
    //dbc->execDirect(nullptr, sql);

    // Cache data on local file system.
    string cacheFile = tempPath;
    etymon::join(&cacheFile, "idmap_cache");
    //printf("[%s]\n", cacheFile.c_str());
    tempFiles->files.push_back(cacheFile);
    sqlite = new etymon::Sqlite3(cacheFile);

    char *zErrMsg = 0;
    int rc;
    string sql;

    sql = "PRAGMA journal_mode = OFF;";
    rc = sqlite3_exec(sqlite->db, sql.c_str(), callback, 0, &zErrMsg);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    sql = "PRAGMA synchronous = OFF;";
    rc = sqlite3_exec(sqlite->db, sql.c_str(), callback, 0, &zErrMsg);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    sql = "PRAGMA locking_mode = EXCLUSIVE;";
    rc = sqlite3_exec(sqlite->db, sql.c_str(), callback, 0, &zErrMsg);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }

    sql =
        "CREATE TABLE idmap_cache (\n"
        "    id VARCHAR(65535) NOT NULL,\n"
        "    sk BIGINT NOT NULL,\n"
        "    new INTEGER NOT NULL,\n"
        "    PRIMARY KEY (id)\n"
        ");";
    rc = sqlite3_exec(sqlite->db, sql.c_str(), callback, 0, &zErrMsg);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }

    sql = "BEGIN;";
    rc = sqlite3_exec(sqlite->db, sql.c_str(), callback, 0, &zErrMsg);
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
            int64_t skl = stol(sk);
            if (skl >= nextvalSK)
                nextvalSK = skl + 1;
            //printf("(%s, %s)\n", sk.c_str(), id.c_str());
            sql =
                "INSERT INTO idmap_cache (id, sk, new)\n"
                "    VALUES ('" + id + "', " + sk + ", 0);";
            rc = sqlite3_exec(sqlite->db, sql.c_str(), callback, 0, &zErrMsg);
            if( rc != SQLITE_OK ){
                fprintf(stderr, "SQL error: %s\n", zErrMsg);
                sqlite3_free(zErrMsg);
            }
        }
        //printf("Cached\n");
    }

    sql = "COMMIT;";
    rc = sqlite3_exec(sqlite->db, sql.c_str(), callback, 0, &zErrMsg);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }

}

IDMap::~IDMap()
{
    delete tx;
    delete dbt;
    delete dbc;
    delete sqlite;
}

void IDMap::makeSK(const string& table, const char* id, string* sk)
{
    string qid = id;
    etymon::toLower(&qid);
    string sql =
        "SELECT sk FROM idmap_cache\n"
        "    WHERE id = '" + qid + "';";
    log->logDetail(sql);
    char *zErrMsg = 0;
    *sk = "";
    int rc = sqlite3_exec(sqlite->db, sql.c_str(), lookupSK, (void*) sk,
            &zErrMsg);
    if( rc != SQLITE_OK ) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
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
        "INSERT INTO idmap_cache (id, sk, new)\n"
        "    VALUES ('" + string(id) + "', " + *sk + ", 1);";
    rc = sqlite3_exec(sqlite->db, sql.c_str(), callback, 0, &zErrMsg);
    if (rc != SQLITE_OK){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        //throw runtime_error("Unable to map ID: " + string(id));
        sqlite3_free(zErrMsg);
    }
    //printf("(%s, %s)\n", sk->c_str(), id);
}

void IDMap::syncCommit()
{
    string sql = "SELECT id, sk FROM idmap_cache WHERE new = 1;";
    log->logDetail(sql);
    char *zErrMsg = 0;
    SyncData syncData(dbc, log);
    int rc = sqlite3_exec(sqlite->db, sql.c_str(), selectAllNew,
            (void*) &syncData, &zErrMsg);
    if( rc != SQLITE_OK ) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    syncData.sync();
    tx->commit();
}

