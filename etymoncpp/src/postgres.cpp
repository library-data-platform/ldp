#include <stdexcept>

#include "../include/postgres.h"

namespace etymon {

Postgres::Postgres(const string& host, const string& port, const string& user,
        const string& password, const string& dbname,
        const string& sslmode)
{
    string conninfo = "host=" + host + " port=" + port + " user=" + user +
        " password=" + password + " dbname=" + dbname + " sslmode=" + sslmode;
    conn = PQconnectdb(conninfo.c_str());
    if (conn == nullptr || PQstatus(conn) == CONNECTION_BAD) {
        string err = PQerrorMessage(conn);
        if (conn != nullptr)
            PQfinish(conn);
        throw runtime_error(err);
    }
}

Postgres::~Postgres()
{
    PQfinish(conn);
}


PostgresResult::PostgresResult(Postgres* postgres, const string& command)
{
    result = PQexec(postgres->conn, command.c_str());
    if (result == nullptr || PQresultStatus(result) == PGRES_FATAL_ERROR) {
        string err = PQresultErrorMessage(result);
        if (result != nullptr)
            PQclear(result);
        throw runtime_error(err);
    }
}

PostgresResult::~PostgresResult()
{
    PQclear(result);
}

}


