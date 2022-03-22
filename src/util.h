#ifndef LDP_UTIL_H
#define LDP_UTIL_H

#include "options.h"

constexpr long unsigned int varchar_size = 67108864;

bool is_uuid(const char* str);

void vacuum_sql(const ldp_options& opt, string* sql);

void comment_sql(const string& table_name, const string& module_name, string* sql);

void print_banner_line(FILE* stream, char ch, int width);

class source_state {
public:
    data_source source;
    string token;
    source_state(data_source source);
    ~source_state();
};

////////////////////////////////////////////////////////////////////////////
// Old error printing functions

enum class Print {
    error,
    warning,
    verbose,
    debug
};

void print(Print level, const ldp_options& opt, const string& str);

#endif

