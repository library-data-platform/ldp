#include <stdexcept>

#include "../include/odbc.h"

namespace etymon {

OdbcEnv::OdbcEnv()
{
    SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env);
    SQLSetEnvAttr(env, SQL_ATTR_ODBC_VERSION, (void *) SQL_OV_ODBC3, 0);
}

OdbcEnv::~OdbcEnv()
{
    SQLFreeHandle(SQL_HANDLE_ENV, env);
}

OdbcDbc::OdbcDbc(const OdbcEnv& odbcEnv, const string& dataSourceName)
{
    SQLAllocHandle(SQL_HANDLE_DBC, odbcEnv.env, &dbc);
    string connStr = "DSN=" + dataSourceName + ";";
    SQLRETURN r = SQLDriverConnect(dbc, NULL, (SQLCHAR *) connStr.c_str(),
            SQL_NTS, NULL, 0, NULL, SQL_DRIVER_COMPLETE);
    if (!SQL_SUCCEEDED(r))
        throw runtime_error("failed to connect to database: " + dataSourceName);
    // Set AUTOCOMMIT_OFF.
    r = SQLSetConnectAttr(dbc, SQL_ATTR_AUTOCOMMIT,
            (SQLPOINTER)SQL_AUTOCOMMIT_OFF, SQL_IS_UINTEGER);
    if (!SQL_SUCCEEDED(r))
        throw runtime_error("error setting AUTOCOMMIT_OFF in database: " +
                dataSourceName);
    this->dataSourceName = dataSourceName;
}

void OdbcDbc::getDbmsName(string* dbmsName) const
{
    SQLCHAR dn[256];
    SQLGetInfo(dbc, SQL_DBMS_NAME, (SQLPOINTER) dn, sizeof(dn), NULL);
    *dbmsName = (char*) dn;
}

void OdbcDbc::execDirect(const string& sql)
{
    etymon::OdbcStmt stmt(*this);
    SQLRETURN r = SQLExecDirect(stmt.stmt, (SQLCHAR *) sql.c_str(),
            SQL_NTS);
    if (!SQL_SUCCEEDED(r))
        throw runtime_error("error executing statement in database: " +
                dataSourceName + ":\n" + sql);
}

void OdbcDbc::commit()
{
    SQLRETURN r = SQLEndTran(SQL_HANDLE_DBC, dbc, SQL_COMMIT);
    if (!SQL_SUCCEEDED(r))
        throw runtime_error("error committing transaction in database: " +
                dataSourceName);
}

void OdbcDbc::rollback()
{
    SQLRETURN r = SQLEndTran(SQL_HANDLE_DBC, dbc, SQL_ROLLBACK);
    if (!SQL_SUCCEEDED(r))
        throw runtime_error("error rolling back transaction in database: " +
                dataSourceName);
}

OdbcDbc::~OdbcDbc()
{
    SQLDisconnect(dbc);
    SQLFreeHandle(SQL_HANDLE_DBC, dbc);
}

OdbcStmt::OdbcStmt(const OdbcDbc& odbcDbc)
{
    SQLAllocHandle(SQL_HANDLE_STMT, odbcDbc.dbc, &stmt);
}

OdbcStmt::~OdbcStmt()
{
    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
}

}

