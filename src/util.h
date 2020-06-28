#ifndef LDP_UTIL_H
#define LDP_UTIL_H

#include "options.h"

bool is_uuid(const char* str);

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

