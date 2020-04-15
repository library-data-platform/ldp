#include <stdexcept>

#include "../include/odbc.h"

namespace etymon {

OdbcEnv::OdbcEnv()
{
    SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &(this->env));
    SQLSetEnvAttr(this->env, SQL_ATTR_ODBC_VERSION, (void *) SQL_OV_ODBC3, 0);
}

OdbcEnv::~OdbcEnv()
{
    SQLFreeHandle(SQL_HANDLE_ENV, this->env);
}

OdbcDbc::OdbcDbc(const OdbcEnv& odbcEnv, const string& dataSourceName)
{
    SQLAllocHandle(SQL_HANDLE_DBC, odbcEnv.env, &(this->dbc));
    string connStr = "DSN=" + dataSourceName + ";";
    SQLRETURN r = SQLDriverConnect(this->dbc, NULL, (SQLCHAR *) connStr.c_str(),
            SQL_NTS, NULL, 0, NULL, SQL_DRIVER_COMPLETE);
    if (!SQL_SUCCEEDED(r))
        throw runtime_error("failed to connect to database: " + dataSourceName);
    // Set AUTOCOMMIT_OFF.
    r = SQLSetConnectAttr(this->dbc, SQL_ATTR_AUTOCOMMIT,
            (SQLPOINTER)SQL_AUTOCOMMIT_OFF, SQL_IS_UINTEGER);
    if (!SQL_SUCCEEDED(r))
        throw runtime_error("error setting AUTOCOMMIT_OFF in database: " +
                dataSourceName);
    this->dataSourceName = dataSourceName;
    ///////////////////////////////////////////////////////////////////////////
    //SQLCHAR dbms_name[256];
    //SQLGetInfo(this->dbc, SQL_DBMS_NAME, (SQLPOINTER) dbms_name,
    //        sizeof(dbms_name), NULL);
    //fprintf(stderr, "DBMS_NAME: %s\n", dbms_name);
    ///////////////////////////////////////////////////////////////////////////
}

void OdbcDbc::commit()
{
    SQLRETURN r = SQLEndTran(SQL_HANDLE_DBC, this->dbc, SQL_COMMIT);
    if (!SQL_SUCCEEDED(r))
        throw runtime_error("error committing transaction in database: " +
                this->dataSourceName);
}

void OdbcDbc::rollback()
{
    SQLRETURN r = SQLEndTran(SQL_HANDLE_DBC, this->dbc, SQL_ROLLBACK);
    if (!SQL_SUCCEEDED(r))
        throw runtime_error("error rolling back transaction in database: " +
                this->dataSourceName);
}

OdbcDbc::~OdbcDbc()
{
    SQLDisconnect(this->dbc);
    SQLFreeHandle(SQL_HANDLE_DBC, this->dbc);
}

OdbcStmt::OdbcStmt(const OdbcDbc& odbcDbc)
{
    SQLAllocHandle(SQL_HANDLE_STMT, odbcDbc.dbc, &(this->stmt));
}

OdbcStmt::~OdbcStmt()
{
    SQLFreeHandle(SQL_HANDLE_STMT, this->stmt);
}

}

