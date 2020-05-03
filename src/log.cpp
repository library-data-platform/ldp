#include <string.h>

#include "../etymoncpp/include/odbc.h"
#include "../etymoncpp/include/util.h"
#include "dbtype.h"
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

void Log::log(Level level, const char* event, const string& table,
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

    // Format elapsed time for logging.
    char elapsed_time_str[255];
    if (elapsed_time < 0)
        strcpy(elapsed_time_str, "NULL");
    else
        sprintf(elapsed_time_str, "%.4f", elapsed_time);

    // Log the message, and print if the log is not available.
    string sql =
        "INSERT INTO ldp_system.log\n"
        "    (log_time, level, event, table_name, message, elapsed_time)\n"
        "  VALUES\n"
        "    (" + string(dbt->currentTimestamp()) + ", '" + levelStr + "', '" +
        event + "', '" + table + "', '" + logmsg + "', " +
        elapsed_time_str + ");";
    try {
        dbc->execDirect(nullptr, sql);
    } catch (runtime_error& e) {
        if (level == Level::debug || level == Level::trace)
            fprintf(stderr, "%s: %s\n", program.c_str(), printmsg.c_str());
    }
}

