#include <cstring>
#include <stdexcept>
#include <unistd.h>

#include "../etymoncpp/include/util.h"
#include "log.h"

void ldp_log::init(log_level lv, bool console, bool quiet)
{
    this->lv = lv;
    this->console = console;
    this->quiet = quiet;
    dbt = new dbtype(conn);
}

ldp_log::ldp_log(etymon::pgconn* conn, log_level lv, bool console, bool quiet)
{
    this->dbinfo = nullptr;
    this->conn = conn;
    this->init(lv, console, quiet);
}

ldp_log::ldp_log(log_level lv, bool console, bool quiet, const etymon::pgconn_info* dbinfo)
{
    this->dbinfo = dbinfo;
    this->conn = nullptr;
    this->init(lv, console, quiet);
}

ldp_log::~ldp_log()
{
    delete dbt;
}

void ldp_log::write(log_level lv, const char* type, const string& table,
        const string& message, double elapsed_time)
{
    // Add a prefix to highlight error states.
    string logmsg;
    switch (lv) {
    case log_level::fatal:
        logmsg = "fatal: " + message;
        break;
    case log_level::error:
        logmsg = "error: " + message;
        break;
    case log_level::warning:
        logmsg = "warning: " + message;
        break;
    default:
        logmsg = message;
    }

    // Format elapsed time for logging.
    char elapsed_time_str[255];
    if (elapsed_time < 0)
        strcpy(elapsed_time_str, "NULL");
    else
        sprintf(elapsed_time_str, "%.0f", elapsed_time);

    // For printing, prefix with '\n' if the message has multiple lines.
    string printmsg;
    if (lv != log_level::detail && message.find('\n') != string::npos) {
        printmsg = "\n" + logmsg;
    } else {
        printmsg = logmsg;
    }
    if (elapsed_time >= 0) {
        // Time: 41.490 ms
        printmsg += " (time: " + string(elapsed_time_str) + " s)";
    }

    // Print error states and filter messages below selected log level.
    const char* level_str;
    switch (lv) {
    case log_level::fatal:
        if (!quiet)
            fprintf(stderr, "ldp: %s\n", printmsg.c_str());
        level_str = "fatal";
        break;
    case log_level::error:
        if (!quiet)
            fprintf(stderr, "ldp: %s\n", printmsg.c_str());
        level_str = "error";
        break;
    case log_level::warning:
        if (!quiet)
            fprintf(stderr, "ldp: %s\n", printmsg.c_str());
        level_str = "warning";
        break;
    case log_level::info:
        if (!quiet)
            fprintf(stderr, "ldp: %s\n", printmsg.c_str());
        level_str = "info";
        break;
    case log_level::debug:
        if (this->lv != log_level::debug && this->lv != log_level::trace &&
                this->lv != log_level::detail)
            return;
        if (console && !quiet)
            fprintf(stderr, "ldp: %s\n", printmsg.c_str());
        level_str = "debug";
        break;
    case log_level::trace:
        if (this->lv != log_level::trace && this->lv != log_level::detail)
            return;
        if (console && !quiet)
            fprintf(stderr, "ldp: %s\n", printmsg.c_str());
        level_str = "trace";
        return;
    case log_level::detail:
        if (this->lv != log_level::detail)
            return;
        if (console && !quiet)
            fprintf(stderr, "%s\n", printmsg.c_str());
        return;
    default:
        level_str = "";
    }

    // Log the message, and print if the log is not available.
    string logmsg_encoded;
    dbt->encode_string_const(logmsg.c_str(), &logmsg_encoded);
    string sql =
        "INSERT INTO dbsystem.log\n"
        "    (log_time, pid, level, type, table_name, message, elapsed_time)\n"
        "  VALUES\n"
        "    (" + string(dbt->current_timestamp()) + ", " +
        to_string(getpid()) + ", '" + level_str + "', '" + type + "', '" +
        table + "', " + logmsg_encoded + ", " + elapsed_time_str + ");";

    if (conn == nullptr) {
        etymon::pgconn c(*dbinfo);
        { etymon::pgconn_result r(&c, sql); }
    } else {
        { etymon::pgconn_result r(conn, sql); }
    }
}

void ldp_log::warning(const string& message)
{
    write(log_level::warning, "", "", message, -1);
}

void ldp_log::trace(const string& message)
{
    write(log_level::trace, "", "", message, -1);
}

void ldp_log::detail(const string& message)
{
    write(log_level::detail, "", "", message, -1);
}

void ldp_log::perf(const string& message, double elapsed_time)
{
    write(log_level::debug, "perf", "", message, elapsed_time);
}

