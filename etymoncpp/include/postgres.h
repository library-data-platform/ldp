#ifndef ETYMON_POSTGRES_H
#define ETYMON_POSTGRES_H

#include <string>
#include <libpq-fe.h>

using namespace std;

namespace etymon {

class pgconn_info {
public:
    string dbname;
    string dbhost;
    int dbport;
    string dbuser;
    string dbpasswd;
    string dbsslmode;
};

class pgconn {
public:
    PGconn* conn;
    pgconn(const pgconn_info& info);
    ~pgconn();
};

class pgconn_result {
public:
    PGresult* result;
    pgconn_result(pgconn* postgres, const string& command);
    ~pgconn_result();
};

class pgconn_result_async {
public:
    PGresult* result;
    pgconn_result_async(pgconn* postgres);
    ~pgconn_result_async();
};

}

#endif
