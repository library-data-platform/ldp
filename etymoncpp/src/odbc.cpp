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

OdbcDbc::OdbcDbc(OdbcEnv* odbcEnv, const string& dataSourceName)
{
    SQLAllocHandle(SQL_HANDLE_DBC, odbcEnv->env, &dbc);
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

    dsn = dataSourceName;
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

void OdbcDbc::execDirectStmt(OdbcStmt* stmt, const string& sql)
{
    SQLRETURN rc = SQLExecDirect(stmt->stmt, (SQLCHAR *) sql.c_str(),
            SQL_NTS);
    if (!SQL_SUCCEEDED(rc) && rc != SQL_NO_DATA) {
        //fprintf(stderr, "ERROR: %s\n", odbcStrError(rc));
        //odbcStrErrorDetail(stmt->stmt, SQL_HANDLE_STMT);
        throw runtime_error("Error executing statement in database: " + dsn +
                ": " + odbcStrError(rc) + ":\n" + sql);
    }
}

void OdbcDbc::exec(const string& sql)
{
    OdbcStmt st(this);
    execDirectStmt(&st, sql);
}

void OdbcDbc::execDirect(OdbcStmt* stmt, const string& sql)
{
    if (stmt == nullptr)
        exec(sql);
    else
        execDirectStmt(stmt, sql);
}

bool OdbcDbc::fetch(OdbcStmt* stmt)
{
    SQLRETURN rc = SQLFetch(stmt->stmt);
    if (rc == SQL_NO_DATA)
        return false;
    if (!SQL_SUCCEEDED(rc))
        throw runtime_error("Error fetching data in database: " + dsn + ": " +
                odbcStrError(rc));
    return true;
}

void OdbcDbc::getData(OdbcStmt* stmt, uint16_t column, string* data)
{
    SQLLEN indicator;
    char buffer[65535];
    SQLRETURN rc = SQLGetData(stmt->stmt, column, SQL_C_CHAR, buffer,
            sizeof buffer, &indicator);
    if (!SQL_SUCCEEDED(rc) && rc != SQL_NO_DATA)
        throw runtime_error("Error getting data in database: " + dsn);
    if (indicator == SQL_NULL_DATA)
        *data = "NULL";
    else
        *data = buffer;
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
        throw runtime_error("error committing transaction in database: " + dsn);
}

void OdbcDbc::rollback()
{
    SQLRETURN rc = SQLEndTran(SQL_HANDLE_DBC, dbc, SQL_ROLLBACK);
    try {
        setAutoCommit(true);
    } catch (runtime_error& e) {}
    if (!SQL_SUCCEEDED(rc))
        throw runtime_error("error rolling back transaction in database: " +
                dsn);
}

void OdbcDbc::setAutoCommit(bool autoCommit)
{
    SQLRETURN rc = SQLSetConnectAttr(dbc, SQL_ATTR_AUTOCOMMIT,
            autoCommit ?
            (SQLPOINTER) SQL_AUTOCOMMIT_ON : (SQLPOINTER) SQL_AUTOCOMMIT_OFF,
            SQL_IS_UINTEGER);
    if (!SQL_SUCCEEDED(rc))
        throw runtime_error("error setting autocommit in database: " + dsn);
}

OdbcStmt::OdbcStmt(OdbcDbc* odbcDbc)
{
    SQLAllocHandle(SQL_HANDLE_STMT, odbcDbc->dbc, &stmt);
}

OdbcStmt::~OdbcStmt()
{
    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
}

OdbcTx::OdbcTx(OdbcDbc* odbcDbc)
{
    dbc = odbcDbc;
    completed = false;
    dbc->startTransaction();
}

OdbcTx::~OdbcTx()
{
    if (!completed)
        dbc->rollback();
}

void OdbcTx::commit()
{
    dbc->commit();
    completed = true;
}

void OdbcTx::rollback()
{
    dbc->rollback();
    completed = true;
}

}

