#ifndef LDP_LOG_H
#define LDP_LOG_H

#include <chrono>
#include <string>

#include "../etymoncpp/include/postgres.h"
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
    ldp_log(etymon::pgconn* conn, log_level lv, bool console, bool quiet);
    ~ldp_log();
    void write(log_level lv, const char* type, const string& table,
            const string& message, double elapsed_time);
    void warning(const string& message);
    void trace(const string& message);
    void detail(const string& message);
    void perf(const string& message, double elapsed_time);
private:
    log_level lv;
    bool console = false;
    bool quiet = false;
    etymon::pgconn* conn;
    dbtype* dbt;
};

#endif
