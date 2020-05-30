#include <cstring>
#include <stdexcept>
#include <unistd.h>

#include "../etymoncpp/include/util.h"
#include "log.h"

Log::Log(etymon::odbc_conn* conn, Level level, bool console, bool quiet,
        const char* program)
{
    this->conn = conn;
    this->level = level;
    this->console = console;
    this->quiet = quiet;
    this->program = program;
    dbt = new DBType(conn);
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

    // Format elapsed time for logging.
    char elapsed_time_str[255];
    if (elapsed_time < 0)
        strcpy(elapsed_time_str, "NULL");
    else
        sprintf(elapsed_time_str, "%.4f", elapsed_time);

    // For printing, prefix with '\n' if the message has multiple lines.
    string printmsg;
    if (level != Level::detail && message.find('\n') != string::npos)
        printmsg = "\n" + logmsg;
    else
        printmsg = logmsg;
    if (elapsed_time >= 0)
        printmsg += " [" + string(elapsed_time_str) + "]";

    // Print error states and filter messages below selected log level.
    const char* levelStr;
    switch (level) {
    case Level::fatal:
        if (!quiet)
            fprintf(stderr, "%s: %s\n", program.c_str(), printmsg.c_str());
        levelStr = "fatal";
        break;
    case Level::error:
        if (!quiet)
            fprintf(stderr, "%s: %s\n", program.c_str(), printmsg.c_str());
        levelStr = "error";
        break;
    case Level::warning:
        if (!quiet)
            fprintf(stderr, "%s: %s\n", program.c_str(), printmsg.c_str());
        levelStr = "warning";
        break;
    case Level::info:
        if (!quiet)
            fprintf(stderr, "%s: %s\n", program.c_str(), printmsg.c_str());
        levelStr = "info";
        break;
    case Level::debug:
        if (this->level != Level::debug && this->level != Level::trace &&
                this->level != Level::detail)
            return;
        if (console && !quiet)
            fprintf(stderr, "%s\n", printmsg.c_str());
        levelStr = "debug";
        break;
    case Level::trace:
        if (this->level != Level::trace && this->level != Level::detail)
            return;
        if (console && !quiet)
            fprintf(stderr, "%s\n", printmsg.c_str());
        levelStr = "trace";
        break;
    case Level::detail:
        if (this->level != Level::detail)
            return;
        if (console && !quiet)
            fprintf(stderr, "%s\n", printmsg.c_str());
        return;
    }

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
    conn->execDirect(nullptr, sql);
}

void Log::trace(const string& message)
{
    log(Level::trace, "", "", message, -1);
}

void Log::logDetail(const string& sql)
{
    detail(sql);
}

void Log::detail(const string& sql)
{
    log(Level::detail, "", "", sql, -1);
}

void Log::perf(const string& message, double elapsed_time)
{
    log(Level::debug, "perf", "", message, elapsed_time);
}

