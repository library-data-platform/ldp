#include <stdexcept>

#include "../include/odbc.h"

namespace etymon {

const char* odbcStrError(SQLRETURN rc)
{
    switch (rc) {
        case SQL_SUCCESS:
            return "SQL_SUCCESS";
        case SQL_SUCCESS_WITH_INFO:
            return "SQL_SUCCESS_WITH_INFO";
        case SQL_ERROR:
            return "SQL_ERROR";
        case SQL_INVALID_HANDLE:
            return "SQL_INVALID_HANDLE";
        case SQL_NO_DATA:
            return "SQL_NO_DATA";
        case SQL_NEED_DATA:
            return "SQL_NEED_DATA";
        case SQL_STILL_EXECUTING:
            return "SQL_STILL_EXECUTING";
        default:
            return "(unknown return code)";
    }
}

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
    SQLRETURN rc = SQLDriverConnect(dbc, NULL, (SQLCHAR *) connStr.c_str(),
            SQL_NTS, NULL, 0, NULL, SQL_DRIVER_COMPLETE);
    if (!SQL_SUCCEEDED(rc))
        throw runtime_error("failed to connect to database: " + dataSourceName);

    // Set isolation level to serializable.
    rc = SQLSetConnectAttr(dbc, SQL_ATTR_TXN_ISOLATION,
            (SQLPOINTER) SQL_TXN_SERIALIZABLE, SQL_IS_UINTEGER);
    if (!SQL_SUCCEEDED(rc))
        throw runtime_error(
                "error setting transaction isolation level in database: " +
                dataSourceName);

    this->dataSourceName = dataSourceName;
}

OdbcDbc::~OdbcDbc()
{
    rollback();
    SQLDisconnect(dbc);
    SQLFreeHandle(SQL_HANDLE_DBC, dbc);
}

void OdbcDbc::getDbmsName(string* dbmsName)
{
    SQLCHAR dn[256];
    SQLGetInfo(dbc, SQL_DBMS_NAME, (SQLPOINTER) dn, sizeof(dn), NULL);
    *dbmsName = (char*) dn;
}

void OdbcDbc::execDirect(const string& sql)
{
    etymon::OdbcStmt stmt(*this);
    SQLRETURN rc = SQLExecDirect(stmt.stmt, (SQLCHAR *) sql.c_str(),
            SQL_NTS);
    if (!SQL_SUCCEEDED(rc) && rc != SQL_NO_DATA) {
        fprintf(stderr, "ERROR: %s\n", odbcStrError(rc));
        //odbcStrErrorDetail(stmt.stmt, SQL_HANDLE_STMT);
        throw runtime_error("error executing statement in database: " +
                dataSourceName + ":\n" + sql);
    }
}

void OdbcDbc::startTransaction()
{
    setAutoCommit(false);
}

void OdbcDbc::commit()
{
    SQLRETURN rc = SQLEndTran(SQL_HANDLE_DBC, dbc, SQL_COMMIT);
    try {
        setAutoCommit(true);
    } catch (runtime_error& e) {}
    if (!SQL_SUCCEEDED(rc))
        throw runtime_error("error committing transaction in database: " +
                dataSourceName);
}

void OdbcDbc::rollback()
{
    SQLRETURN rc = SQLEndTran(SQL_HANDLE_DBC, dbc, SQL_ROLLBACK);
    try {
        setAutoCommit(true);
    } catch (runtime_error& e) {}
    if (!SQL_SUCCEEDED(rc))
        throw runtime_error("error rolling back transaction in database: " +
                dataSourceName);
}

void OdbcDbc::setAutoCommit(bool autoCommit)
{
    SQLRETURN rc = SQLSetConnectAttr(dbc, SQL_ATTR_AUTOCOMMIT,
            autoCommit ?
            (SQLPOINTER) SQL_AUTOCOMMIT_ON : (SQLPOINTER) SQL_AUTOCOMMIT_OFF,
            SQL_IS_UINTEGER);
    if (!SQL_SUCCEEDED(rc))
        throw runtime_error("error setting autocommit in database: " +
                dataSourceName);
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

