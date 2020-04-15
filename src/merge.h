#ifndef LDP_MERGE_H
#define LDP_MERGE_H

#include <string>

#include "../etymoncpp/include/postgres.h"
#include "options.h"
#include "schema.h"

using namespace std;

void mergeTable(const Options& opt, const TableSchema& table,
        etymon::OdbcDbc* dbc, const DBType& dbt);
void dropTable(const Options& opt, const string& tableName,
        etymon::OdbcDbc* dbc);
void placeTable(const Options& opt, const TableSchema& table,
        etymon::OdbcDbc* dbc);
void updateStatus(const Options& opt, const TableSchema& table,
        etymon::OdbcDbc* dbc);
void dropOldTables(const Options& opt, etymon::OdbcDbc* dbc);

//void mergeAll(const Options& opt, Schema* schema, etymon::Postgres* db);

#endif
