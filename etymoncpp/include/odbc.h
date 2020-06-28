#ifndef ETYMON_ODBC_H
#define ETYMON_ODBC_H

#include <string>
#include <sql.h>
#include <sqlext.h>

using namespace std;

namespace etymon {

const char* odbc_str_error(SQLRETURN rc);

class odbc_env {
public:
    SQLHENV env;
    odbc_env();
    ~odbc_env();
};

class odbc_stmt;

class odbc_conn {
public:
    SQLHDBC conn;
    string dsn;
    odbc_conn(odbc_env* env, const string& data_source_name);
    ~odbc_conn();
    void get_dbms_name(string* dbms_name);
    void exec(const string& sql);
    void exec_direct(odbc_stmt* stmt, const string& sql);
    bool fetch(odbc_stmt* stmt);
    void get_data(odbc_stmt* stmt, uint16_t column, string* data);
private:
    void exec_direct_stmt(odbc_stmt* stmt, const string& sql);
};

class odbc_stmt {
public:
    SQLHSTMT stmt;
    odbc_stmt(odbc_conn* conn);
    ~odbc_stmt();
};

class odbc_tx {
public:
    odbc_conn* conn;
    odbc_tx(odbc_conn* conn);
    ~odbc_tx();
    void commit();
    void rollback();
private:
    bool completed;
};

}

#endif
