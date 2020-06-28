#ifndef LDP_INITUTIL_H
#define LDP_INITUTIL_H

#include <string>

#include "../etymoncpp/include/odbc.h"
#include "dbtype.h"

using namespace std;

void create_main_table_sql(const string& table_name, etymon::odbc_conn* conn,
                           const dbtype& dbt, string* sql);

void create_history_table_sql(const string& table_name,
                              etymon::odbc_conn* conn, const dbtype& dbt,
                              string* sql);

void grant_select_on_table_sql(const string& table, const string& user,
                               etymon::odbc_conn* conn, string* sql);

void add_table_to_catalog_sql(etymon::odbc_conn* conn, const string& table,
                              string* sql);

#endif

