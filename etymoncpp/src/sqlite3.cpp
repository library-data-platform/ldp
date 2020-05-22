#include <stdexcept>

#include "../include/sqlite3.h"

namespace etymon {

Sqlite3::Sqlite3(const string& filename)
{
    this->filename = filename;
    int rc = sqlite3_open(filename.c_str(), &db);
    if (rc)
        throw runtime_error("Unable to open database: " + filename + ": " +
                sqlite3_errmsg(db));
}

Sqlite3::~Sqlite3()
{
    sqlite3_close(db);
}

static int execCallback(void *NotUsed, int argc, char **argv, char **azColName)
{
    return 0;
}

void Sqlite3::exec(const string& sql)
{
    char *errstr = 0;
    int rc = sqlite3_exec(db, sql.c_str(), execCallback, 0, &errstr);
    if (rc != SQLITE_OK) {
        string err = errstr;
        sqlite3_free(errstr);
        throw runtime_error("Error in database: " + filename + ":\n" + err);
    }
}

void Sqlite3::exec(const string& sql, int (*callback)(void*,int,char**,char**),
            void* data)
{
    char *errstr = 0;
    int rc = sqlite3_exec(db, sql.c_str(), callback, data, &errstr);
    if (rc != SQLITE_OK) {
        string err = errstr;
        sqlite3_free(errstr);
        throw runtime_error("Error in database: " + filename + ":\n" + err);
    }
}

}

