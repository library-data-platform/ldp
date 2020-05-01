#include "../etymoncpp/include/util.h"
#include "util.h"

Log::Log(etymon::OdbcDbc* dbc, Level level, const char* program)
{
    this->dbc = dbc;
    this->level = level;
    this->program = program;
    dbt = new DBType(dbc);
}

Log::~Log()
{
    delete dbt;
}

void Log::logEvent(Level level, const char* event, const string& table,
        const string& message, double elapsed_time)
{
    const char* levelStr;
    switch (level) {
    case Level::fatal:
        fprintf(stderr, "%s: Fatal: %s: %s\n", program.c_str(), event,
                message.c_str());
        levelStr = "fatal";
        break;
    case Level::error:
        fprintf(stderr, "%s: Error: %s: %s\n", program.c_str(), event,
                message.c_str());
        levelStr = "error";
        break;
    case Level::warning:
        fprintf(stderr, "%s: Warning: %s: %s\n", program.c_str(), event,
                message.c_str());
        levelStr = "warning";
        break;
    case Level::info:
        fprintf(stderr, "%s: %s: %s\n", program.c_str(), event,
                message.c_str());
        levelStr = "info";
        break;
    case Level::debug:
        if (this->level != Level::debug && this->level != Level::trace)
            return;
        levelStr = "debug";
        break;
    case Level::trace:
        if (this->level != Level::trace)
            return;
        levelStr = "trace";
        break;
    }
    char elapsed_time_str[255];
    sprintf(elapsed_time_str, "%.4f", elapsed_time);
    string sql =
        "INSERT INTO ldp_system.log\n"
        "    (log_time, level, event, table_name, message, elapsed_time)\n"
        "  VALUES\n"
        "    (" + string(dbt->currentTimestamp()) + ", '" + levelStr + "', '" +
        event + "', '" + table + "', '" + message + "', " +
        elapsed_time_str + ");";
    dbc->execDirect(nullptr, sql);
}

// Old error printing functions

void print(Print level, const Options& opt, const string& str)
{
    string s = str;
    etymon::trim(&s);
    switch (level) {
    case Print::error:
        fprintf(opt.err, "%s: error: %s\n", opt.prog, s.c_str());
        break;
    case Print::warning:
        fprintf(opt.err, "%s: warning: %s\n", opt.prog, s.c_str());
        break;
    //case Print::info:
    //    fprintf(opt.err, "%s: %s\n", opt.prog, s.c_str());
    //    break;
    case Print::verbose:
        if (opt.verbose || opt.debug)
            fprintf(opt.err, "%s: %s\n", opt.prog, s.c_str());
        break;
    case Print::debug:
        if (opt.debug)
            fprintf(opt.err, "%s: %s\n", opt.prog, s.c_str());
        break;
    }
}

void printSQL(Print level, const Options& opt, const string& sql)
{
    print(level, opt, string("sql:\n") + sql);
}

void printSchema(FILE* stream, const Schema& schema)
{
    fprintf(stream, "Module name,Source path,Table name\n");
    for (const auto& table : schema.tables) {
        fprintf(stream, "%s,%s,%s\n", table.moduleName.c_str(),
                table.sourcePath.c_str(), table.tableName.c_str());
    }
}
