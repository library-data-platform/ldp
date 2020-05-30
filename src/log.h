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
    Log(etymon::odbc_conn* conn, Level level, bool console, bool quiet,
            const char* program);
    ~Log();
    void log(Level level, const char* type, const string& table,
            const string& message, double elapsed_time);
    void trace(const string& message);
    void logDetail(const string& sql);
    void detail(const string& sql);
    void perf(const string& message, double elapsed_time);
private:
    Level level;
    bool console = false;
    bool quiet = false;
    etymon::odbc_conn* conn;
    DBType* dbt;
    string program;
};

#endif
