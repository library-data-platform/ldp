#ifndef LDP_MERGE_H
#define LDP_MERGE_H

#include <string>

#include "../etymoncpp/include/postgres.h"
#include "options.h"
#include "schema.h"

using namespace std;

void mergeTable(const Options& opt, Log* log, const TableSchema& table,
        etymon::odbc_env* odbc, etymon::odbc_conn* conn, const DBType& dbt);
void dropTable(const Options& opt, Log* log, const string& tableName,
        etymon::odbc_conn* conn);
void placeTable(const Options& opt, Log* log, const TableSchema& table,
        etymon::odbc_conn* conn);
void updateStatus(const Options& opt, const TableSchema& table,
        etymon::odbc_conn* conn);
//void dropOldTables(const Options& opt, Log* log, etymon::odbc_conn* conn);

//void mergeAll(const Options& opt, Schema* schema, etymon::Postgres* db);

#endif
