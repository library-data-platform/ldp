#ifndef LDP_LOG_H
#define LDP_LOG_H

#include <chrono>
#include <string>

#include "../etymoncpp/include/odbc.h"
#include "dbtype.h"

using namespace std;

enum class log_level {
    fatal,
    error,
    warning,
    info,
    debug,
    trace,
    detail
};

class ldp_log {
public:
    ldp_log(etymon::odbc_conn* conn, log_level lv, bool console, bool quiet,
            const char* program);
    ~ldp_log();
    void write(log_level lv, const char* type, const string& table,
            const string& message, double elapsed_time);
    void trace(const string& message);
    void detail(const string& sql);
    void perf(const string& message, double elapsed_time);
private:
    log_level lv;
    bool console = false;
    bool quiet = false;
    etymon::odbc_conn* conn;
    dbtype* dbt;
    string program;
};

#endif
