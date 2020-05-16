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
    Log(etymon::OdbcDbc* dbc, Level level, bool console, const char* program);
    ~Log();
    void log(Level level, const char* type, const string& table,
            const string& message, double elapsed_time);
    void logDetail(const string& sql);
    void console(const string& sql);
    //void error(const string& message);
private:
    Level level;
    bool cons = false;
    etymon::OdbcDbc* dbc;
    DBType* dbt;
    string program;
};

#endif
