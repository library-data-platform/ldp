#ifndef LDP_SCHEMA_H
#define LDP_SCHEMA_H

#include <string>
#include <vector>

#include "log.h"

using namespace std;

enum class column_type {
    bigint,
    boolean,
    id,
    numeric,
    timestamptz,
    varchar
};

class type_counts {
public:
    unsigned int string = 0;
    unsigned int date_time = 0;
    unsigned int number = 0;
    unsigned int integer = 0;
    unsigned int floating = 0;
    unsigned int boolean = 0;
    unsigned int null = 0;
    unsigned int uuid = 0;
    unsigned int max_length = 0;
};

class column_schema {
public:
    string name;
    column_type type;
    unsigned int length = 0;
    string source_name;
    static void type_to_string(column_type type, string* str);
    static bool select_type(ldp_log* lg, const string& table,
                            const string& source_path, const string& field,
                            const type_counts& counts, column_type* ctype);
};

enum class data_source_type {
    rmb,
    rmb_marc
};

class table_schema {
public:
    bool skip = false;
    bool anonymize = false;
    string name;
    string source_spec;
    data_source_type source_type;
    vector<column_schema> columns;
    string module_name;
    string direct_source_table;
};

class ldp_schema {
public:
    vector<table_schema> tables;
    static void make_default_schema(ldp_schema* schema);
};

#endif

