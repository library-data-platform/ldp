#ifndef ETYMON_ODBC_H
#define ETYMON_ODBC_H

#include <string>
#include <sql.h>
#include <sqlext.h>

using namespace std;

namespace etymon {

const char* odbcStrError(SQLRETURN rc);

class OdbcEnv {
public:
    SQLHENV env;
    OdbcEnv();
    ~OdbcEnv();
};

class OdbcStmt;

class OdbcDbc {
public:
    SQLHDBC dbc;
    string dataSourceName;
    OdbcDbc(const OdbcEnv& odbcEnv, const string& dataSourceName);
    ~OdbcDbc();
    void getDbmsName(string* dbmsName);
    void execDirect(OdbcStmt* stmt, const string& sql);
    void fetch(OdbcStmt* stmt);
    void getData(OdbcStmt* stmt, uint16_t column, string* data);
    void startTransaction();
    void commit();
    void rollback();
private:
    void setAutoCommit(bool autoCommit);
    void execDirectStmt(OdbcStmt* stmt, const string& sql);
};

class OdbcStmt {
public:
    SQLHSTMT stmt;
    OdbcStmt(const OdbcDbc& odbcDbc);
    ~OdbcStmt();
};

}

#endif
