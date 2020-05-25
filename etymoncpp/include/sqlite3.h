#ifndef ETYMON_SQLITE3_H
#define ETYMON_SQLITE3_H

#include <string>
#include <sqlite3.h>

using namespace std;

namespace etymon {

class sqlite_db {
public:
    sqlite3* db;
    string filename;
    sqlite_db(const string& filename);
    ~sqlite_db();
    void exec(const string& sql);
    void exec(const string& sql, int (*callback)(void*,int,char**,char**),
            void* data);
};

}

#endif
