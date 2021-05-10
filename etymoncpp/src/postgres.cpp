#include <stdexcept>

#include "../include/postgres.h"

namespace etymon {

static void debug_notice_processor(void *arg, const char *message)
{
    // NOP
}

pgconn::pgconn(const pgconn_info& info)
{
    string conninfo = "host=" + info.dbhost + " port=" + to_string(info.dbport) + " user=" + info.dbuser +
        " password=" + info.dbpasswd + " dbname=" + info.dbname + " sslmode=" + info.dbsslmode;
    conn = PQconnectdb(conninfo.c_str());
    if (conn == nullptr || PQstatus(conn) == CONNECTION_BAD) {
        string err = PQerrorMessage(conn);
        if (conn != nullptr)
            PQfinish(conn);
        throw runtime_error(err);
    }
    PQsetNoticeProcessor(conn, debug_notice_processor, (void*) nullptr);
}

pgconn::~pgconn()
{
    PQfinish(conn);
}

pgconn_result::pgconn_result(pgconn* postgres, const string& command)
{
    result = PQexec(postgres->conn, command.c_str());
    if (result == nullptr || PQresultStatus(result) == PGRES_FATAL_ERROR) {
        string err = PQresultErrorMessage(result);
        if (result != nullptr)
            PQclear(result);
        throw runtime_error(err);
    }
}

pgconn_result::~pgconn_result()
{
    PQclear(result);
}

pgconn_result_async::pgconn_result_async(pgconn* postgres)
{
    result = PQgetResult(postgres->conn);
    if (result != nullptr && PQresultStatus(result) == PGRES_FATAL_ERROR) {
        string err = PQresultErrorMessage(result);
        if (result != nullptr)
            PQclear(result);
        while (true) {
            result = PQgetResult(postgres->conn);
            if (result != nullptr)
                PQclear(result);
            else
                break;
        }
        throw runtime_error(err);
    }
}

pgconn_result_async::~pgconn_result_async()
{
    if (result != nullptr)
        PQclear(result);
}

}

