#include <cstring>
#include <stdexcept>
#include <unistd.h>

#include "../etymoncpp/include/util.h"
#include "log.h"

log::log(etymon::odbc_conn* conn, level lv, bool console, bool quiet,
        const char* program)
{
    this->conn = conn;
    this->lv = lv;
    this->console = console;
    this->quiet = quiet;
    this->program = program;
    dbt = new dbtype(conn);
}

log::~log()
{
    delete dbt;
}

void log::write(level lv, const char* type, const string& table,
        const string& message, double elapsed_time)
{
    // Add a prefix to highlight error states.
    string logmsg;
    switch (lv) {
    case level::fatal:
        logmsg = "Fatal: " + message;
        break;
    case level::error:
        logmsg = "Error: " + message;
        break;
    case level::warning:
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
    if (lv != level::detail && message.find('\n') != string::npos)
        printmsg = "\n" + logmsg;
    else
        printmsg = logmsg;
    if (elapsed_time >= 0)
        printmsg += " [" + string(elapsed_time_str) + "]";

    // Print error states and filter messages below selected log level.
    const char* level_str;
    switch (lv) {
    case level::fatal:
        if (!quiet)
            fprintf(stderr, "%s: %s\n", program.c_str(), printmsg.c_str());
        level_str = "fatal";
        break;
    case level::error:
        if (!quiet)
            fprintf(stderr, "%s: %s\n", program.c_str(), printmsg.c_str());
        level_str = "error";
        break;
    case level::warning:
        if (!quiet)
            fprintf(stderr, "%s: %s\n", program.c_str(), printmsg.c_str());
        level_str = "warning";
        break;
    case level::info:
        if (!quiet)
            fprintf(stderr, "%s: %s\n", program.c_str(), printmsg.c_str());
        level_str = "info";
        break;
    case level::debug:
        if (this->lv != level::debug && this->lv != level::trace &&
                this->lv != level::detail)
            return;
        if (console && !quiet)
            fprintf(stderr, "%s\n", printmsg.c_str());
        level_str = "debug";
        break;
    case level::trace:
        if (this->lv != level::trace && this->lv != level::detail)
            return;
        if (console && !quiet)
            fprintf(stderr, "%s\n", printmsg.c_str());
        level_str = "trace";
        break;
    case level::detail:
        if (this->lv != level::detail)
            return;
        if (console && !quiet)
            fprintf(stderr, "%s\n", printmsg.c_str());
        return;
    }

    // Log the message, and print if the log is not available.
    string logmsgEncoded;
    dbt->encode_string_const(logmsg.c_str(), &logmsgEncoded);
    string sql =
        "INSERT INTO ldpsystem.log\n"
        "    (log_time, pid, level, type, table_name, message, elapsed_time)\n"
        "  VALUES\n"
        "    (" + string(dbt->current_timestamp()) + ", " +
        to_string(getpid()) + ", '" + level_str + "', '" + type + "', '" +
        table + "', " + logmsgEncoded + ", " + elapsed_time_str + ");";
    conn->exec(sql);
}

void log::trace(const string& message)
{
    write(level::trace, "", "", message, -1);
}

void log::detail(const string& sql)
{
    write(level::detail, "", "", sql, -1);
}

void log::perf(const string& message, double elapsed_time)
{
    write(level::debug, "perf", "", message, elapsed_time);
}

