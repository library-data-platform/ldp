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
    odbc_conn(odbc_env* odbcEnv, const string& dataSourceName);
    ~odbc_conn();
    void getDbmsName(string* dbmsName);
    void exec(const string& sql);
    void execDirect(odbc_stmt* stmt, const string& sql);
    bool fetch(odbc_stmt* stmt);
    void getData(odbc_stmt* stmt, uint16_t column, string* data);
    void startTransaction();
    void commit();
    void rollback();
private:
    void setAutoCommit(bool autoCommit);
    void execDirectStmt(odbc_stmt* stmt, const string& sql);
};

class odbc_stmt {
public:
    SQLHSTMT stmt;
    odbc_stmt(odbc_conn* odbcDbc);
    ~odbc_stmt();
};

class odbc_tx {
public:
    odbc_conn* conn;
    odbc_tx(odbc_conn* odbcDbc);
    ~odbc_tx();
    void commit();
    void rollback();
private:
    bool completed;
};

}

#endif
