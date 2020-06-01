#ifndef LDP_SCHEMA_H
#define LDP_SCHEMA_H

#include <string>
#include <vector>

#include "log.h"

using namespace std;

enum class ColumnType {
    bigint,
    boolean,
    id,
    numeric,
    timestamptz,
    varchar
};

class Counts {
public:
    unsigned int string = 0;
    unsigned int dateTime = 0;
    unsigned int number = 0;
    unsigned int integer = 0;
    unsigned int floating = 0;
    unsigned int boolean = 0;
    unsigned int null = 0;
    unsigned int uuid = 0;
};

class ColumnSchema {
public:
    string columnName;
    ColumnType columnType;
    string sourceColumnName;
    static void columnTypeToString(ColumnType type, string* str);
    static bool selectColumnType(log* lg, const string& table,
            const string& source_path, const string& field,
            const Counts& counts, ColumnType* ctype);
};

enum class SourceType {
    rmb,
    rmbMarc
};

class TableSchema {
public:
    bool skip = false;
    bool anonymize = false;
    string tableName;
    string sourcePath;
    SourceType sourceType;
    vector<ColumnSchema> columns;
    string moduleName;
    string directSourceTable;
};

class Schema {
public:
    vector<TableSchema> tables;
    static void make_default_schema(Schema* schema);
};

#endif
