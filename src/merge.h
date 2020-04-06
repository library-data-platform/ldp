#ifndef LDP_MERGE_H
#define LDP_MERGE_H

#include <string>

#include "../etymoncpp/include/postgres.h"
#include "options.h"
#include "schema.h"

using namespace std;

void mergeTable(const Options& opt, const TableSchema& table,
        etymon::Postgres* db);
void dropTable(const Options& opt, const string& tableName,
        etymon::Postgres* db);
void placeTable(const Options& opt, const TableSchema& table,
        etymon::Postgres* db);
void updateStatus(const Options& opt, const TableSchema& table,
        etymon::Postgres* db);
void dropOldTables(const Options& opt, etymon::Postgres* db);

//void mergeAll(const Options& opt, Schema* schema, etymon::Postgres* db);

#endif
