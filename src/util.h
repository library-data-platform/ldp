#ifndef LDP_UTIL_H
#define LDP_UTIL_H

#include <chrono>
#include <string>

#include "options.h"
#include "schema.h"

using namespace std;

enum class Level {
    fatal,
    error,
    warning,
    info,
    debug,
    trace
};

class Log {
public:
    Log(etymon::OdbcDbc* dbc, Level level, const char* program);
    ~Log();
    void logEvent(Level level, const char* event, const string& table,
            const string& message, double elapsed_time);
private:
    Level level;
    etymon::OdbcDbc* dbc;
    DBType* dbt;
    string program;
};

// Old error printing functions

enum class Print {
    error,
    warning,
    //info,
    verbose,
    debug
};

void print(Print level, const Options& opt, const string& str);
void printSQL(Print level, const Options& opt, const string& sql);

void printSchema(FILE* stream, const Schema& schema);

#endif
