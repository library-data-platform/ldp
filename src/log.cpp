#include <cstring>
#include <stdexcept>
#include <unistd.h>

#include "../etymoncpp/include/util.h"
#include "log.h"

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

void Log::log(Level level, const char* type, const string& table,
        const string& message, double elapsed_time)
{
    // Add a prefix to highlight error states.
    string logmsg;
    switch (level) {
    case Level::fatal:
        logmsg = "Fatal: " + message;
        break;
    case Level::error:
        logmsg = "Error: " + message;
        break;
    case Level::warning:
        logmsg = "Warning: " + message;
        break;
    default:
        logmsg = message;
    }

    // For printing, prefix with '\n' if the message has multiple lines.
    string printmsg;
    if (message.find('\n') != string::npos)
        printmsg = "\n" + logmsg;
    else
        printmsg = logmsg;

    // Print error states and filter messages below selected log level.
    const char* levelStr;
    switch (level) {
    case Level::fatal:
        fprintf(stderr, "%s: %s\n", program.c_str(), printmsg.c_str());
        levelStr = "fatal";
        break;
    case Level::error:
        fprintf(stderr, "%s: %s\n", program.c_str(), printmsg.c_str());
        levelStr = "error";
        break;
    case Level::warning:
        fprintf(stderr, "%s: %s\n", program.c_str(), printmsg.c_str());
        levelStr = "warning";
        break;
    case Level::info:
        fprintf(stderr, "%s: %s\n", program.c_str(), printmsg.c_str());
        levelStr = "info";
        break;
    case Level::debug:
        if (this->level != Level::debug && this->level != Level::trace &&
                this->level != Level::detail)
            return;
        levelStr = "debug";
        break;
    case Level::trace:
        if (this->level != Level::trace && this->level != Level::detail)
            return;
        levelStr = "trace";
        break;
    case Level::detail:
        if (this->level != Level::detail)
            return;
        levelStr = "detail";
        break;
    }

    // Format elapsed time for logging.
    char elapsed_time_str[255];
    if (elapsed_time < 0)
        strcpy(elapsed_time_str, "NULL");
    else
        sprintf(elapsed_time_str, "%.4f", elapsed_time);

    // Log the message, and print if the log is not available.
    string logmsgEncoded;
    dbt->encodeStringConst(logmsg.c_str(), &logmsgEncoded);
    string sql =
        "INSERT INTO ldpsystem.log\n"
        "    (log_time, pid, level, type, table_name, message, elapsed_time)\n"
        "  VALUES\n"
        "    (" + string(dbt->currentTimestamp()) + ", " + to_string(getpid()) +
        ", '" + levelStr + "', '" + type + "', '" + table + "', " +
        logmsgEncoded + ", " + elapsed_time_str + ");";
    try {
        dbc->execDirect(nullptr, sql);
    } catch (runtime_error& e) {
        if (level == Level::debug || level == Level::trace ||
                level == Level::detail)
            fprintf(stderr, "%s: %s\n", program.c_str(), printmsg.c_str());
    }
}

void Log::logSQL(const string& sql)
{
    log(Level::detail, "", "", sql, -1);
}

//void Log::error(const string& message)
//{
//    log(Level::error, "server", "", message, -1);
//    throw runtime_error(message);
//}


