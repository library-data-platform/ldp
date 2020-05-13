#ifndef ETYMON_SQLITE3_H
#define ETYMON_SQLITE3_H

#include <string>
#include <sqlite3.h>

using namespace std;

namespace etymon {

class Sqlite3 {
public:
    sqlite3* db;
    string filename;
    Sqlite3(const string& filename);
    ~Sqlite3();
};

}

#endif
