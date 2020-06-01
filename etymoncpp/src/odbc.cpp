#include <stdexcept>

#include "../include/odbc.h"

namespace etymon {

const char* odbc_str_error(SQLRETURN rc)
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

odbc_env::odbc_env()
{
    SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env);
    SQLSetEnvAttr(env, SQL_ATTR_ODBC_VERSION, (void *) SQL_OV_ODBC3, 0);
}

odbc_env::~odbc_env()
{
    SQLFreeHandle(SQL_HANDLE_ENV, env);
}

odbc_conn::odbc_conn(odbc_env* env, const string& data_source_name)
{
    SQLAllocHandle(SQL_HANDLE_DBC, env->env, &conn);
    string conn_str = "DSN=" + data_source_name + ";";
    SQLRETURN rc = SQLDriverConnect(conn, NULL, (SQLCHAR *) conn_str.c_str(),
            SQL_NTS, NULL, 0, NULL, SQL_DRIVER_COMPLETE);
    if (!SQL_SUCCEEDED(rc))
        throw runtime_error("failed to connect to database: " +
                data_source_name);

    // Set isolation level to serializable.
    rc = SQLSetConnectAttr(conn, SQL_ATTR_TXN_ISOLATION,
            (SQLPOINTER) SQL_TXN_SERIALIZABLE, SQL_IS_UINTEGER);
    if (!SQL_SUCCEEDED(rc))
        throw runtime_error(
                "error setting transaction isolation level in database: " +
                data_source_name);

    dsn = data_source_name;
}

odbc_conn::~odbc_conn()
{
    SQLDisconnect(conn);
    SQLFreeHandle(SQL_HANDLE_DBC, conn);
}

void odbc_conn::get_dbms_name(string* dbms_name)
{
    SQLCHAR dn[256];
    SQLGetInfo(conn, SQL_DBMS_NAME, (SQLPOINTER) dn, sizeof(dn), NULL);
    *dbms_name = (char*) dn;
}

void odbc_conn::exec_direct_stmt(odbc_stmt* stmt, const string& sql)
{
    SQLRETURN rc = SQLExecDirect(stmt->stmt, (SQLCHAR *) sql.c_str(),
            SQL_NTS);
    if (!SQL_SUCCEEDED(rc) && rc != SQL_NO_DATA) {
        //fprintf(stderr, "ERROR: %s\n", odbc_str_error(rc));
        //odbc_str_error_detail(stmt->stmt, SQL_HANDLE_STMT);
        throw runtime_error("Error executing statement in database: " + dsn +
                ": " + odbc_str_error(rc) + ":\n" + sql);
    }
}

void odbc_conn::exec(const string& sql)
{
    odbc_stmt st(this);
    exec_direct_stmt(&st, sql);
}

void odbc_conn::exec_direct(odbc_stmt* stmt, const string& sql)
{
    if (stmt == nullptr)
        exec(sql);
    else
        exec_direct_stmt(stmt, sql);
}

bool odbc_conn::fetch(odbc_stmt* stmt)
{
    SQLRETURN rc = SQLFetch(stmt->stmt);
    if (rc == SQL_NO_DATA)
        return false;
    if (!SQL_SUCCEEDED(rc))
        throw runtime_error("Error fetching data in database: " + dsn + ": " +
                odbc_str_error(rc));
    return true;
}

void odbc_conn::get_data(odbc_stmt* stmt, uint16_t column, string* data)
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

void odbc_conn::start_transaction()
{
    set_auto_commit(false);
}

void odbc_conn::commit()
{
    SQLRETURN rc = SQLEndTran(SQL_HANDLE_DBC, conn, SQL_COMMIT);
    try {
        set_auto_commit(true);
    } catch (runtime_error& e) {}
    if (!SQL_SUCCEEDED(rc))
        throw runtime_error("error committing transaction in database: " + dsn);
}

void odbc_conn::rollback()
{
    SQLRETURN rc = SQLEndTran(SQL_HANDLE_DBC, conn, SQL_ROLLBACK);
    try {
        set_auto_commit(true);
    } catch (runtime_error& e) {}
    if (!SQL_SUCCEEDED(rc))
        throw runtime_error("error rolling back transaction in database: " +
                dsn);
}

void odbc_conn::set_auto_commit(bool auto_commit)
{
    SQLRETURN rc = SQLSetConnectAttr(conn, SQL_ATTR_AUTOCOMMIT,
            auto_commit ?
            (SQLPOINTER) SQL_AUTOCOMMIT_ON : (SQLPOINTER) SQL_AUTOCOMMIT_OFF,
            SQL_IS_UINTEGER);
    if (!SQL_SUCCEEDED(rc))
        throw runtime_error("error setting autocommit in database: " + dsn);
}

odbc_stmt::odbc_stmt(odbc_conn* conn)
{
    SQLAllocHandle(SQL_HANDLE_STMT, conn->conn, &stmt);
}

odbc_stmt::~odbc_stmt()
{
    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
}

odbc_tx::odbc_tx(odbc_conn* conn)
{
    this->conn = conn;
    completed = false;
    this->conn->start_transaction();
}

void odbc_tx::commit()
{
    conn->commit();
    completed = true;
}

void odbc_tx::rollback()
{
    conn->rollback();
    completed = true;
}

}

