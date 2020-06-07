#ifndef LDP_MERGE_H
#define LDP_MERGE_H

#include <string>

#include "../etymoncpp/include/postgres.h"
#include "options.h"
#include "schema.h"

using namespace std;

void mergeTable(const ldp_options& opt, ldp_log* lg, const table_schema& table,
        etymon::odbc_env* odbc, etymon::odbc_conn* conn, const dbtype& dbt);
void dropTable(const ldp_options& opt, ldp_log* lg, const string& tableName,
        etymon::odbc_conn* conn);
void placeTable(const ldp_options& opt, ldp_log* lg, const table_schema& table,
        etymon::odbc_conn* conn);
void updateStatus(const ldp_options& opt, const table_schema& table,
        etymon::odbc_conn* conn);

#endif
