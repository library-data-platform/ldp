#ifndef LDP_MERGE_H
#define LDP_MERGE_H

#include <string>

#include "../etymoncpp/include/postgres.h"
#include "options.h"
#include "schema.h"

using namespace std;

void create_latest_history_table(const ldp_options& opt, ldp_log* lg,
                                 const table_schema& table,
                                 etymon::pgconn* conn);
void drop_latest_history_table(const ldp_options& opt, ldp_log* lg,
                                 const table_schema& table,
                                 etymon::pgconn* conn);

void merge_table(const ldp_options& opt, ldp_log* lg, const table_schema& table,
                 etymon::pgconn* conn, const dbtype& dbt);
void drop_table(const ldp_options& opt, ldp_log* lg, const string& tableName,
                etymon::pgconn* conn);
void place_table(const ldp_options& opt, ldp_log* lg, const table_schema& table,
                 etymon::pgconn* conn);

#endif
