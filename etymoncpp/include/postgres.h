#ifndef ETYMON_POSTGRES_H
#define ETYMON_POSTGRES_H

#include <string>
#include <libpq-fe.h>

using namespace std;

namespace etymon {

class Postgres {
public:
    PGconn* conn;
    Postgres(const string& host, const string& port, const string& user,
            const string& password, const string& dbname,
            const string& sslmode);
    ~Postgres();
};

class PostgresResult {
public:
    PGresult* result;
    PostgresResult(Postgres* postgres, const string& command);
    ~PostgresResult();
};

class PostgresResultAsync {
public:
    PGresult* result;
    PostgresResultAsync(Postgres* postgres);
    ~PostgresResultAsync();
};

}

#endif
