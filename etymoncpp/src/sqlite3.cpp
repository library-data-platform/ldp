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

}


