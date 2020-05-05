#ifndef LDP_LOG_H
#define LDP_LOG_H

#include <chrono>
#include <string>

#include "../etymoncpp/include/odbc.h"
#include "schema.h"
#include "dbtype.h"

using namespace std;

enum class Level {
    fatal,
    error,
    warning,
    info,
    debug,
    trace,
    detail
};

class Log {
public:
    Log(etymon::OdbcDbc* dbc, Level level, const char* program);
    ~Log();
    void log(Level level, const char* type, const string& table,
            const string& message, double elapsed_time);
private:
    Level level;
    etymon::OdbcDbc* dbc;
    DBType* dbt;
    string program;
};

#endif
