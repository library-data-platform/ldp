#ifndef LDP_UTIL_H
#define LDP_UTIL_H

#include <chrono>
#include <string>

#include "options.h"
#include "schema.h"

using namespace std;

bool isUUID(const char* str);

void print_banner_line(FILE* stream, char ch, int width);

////////////////////////////////////////////////////////////////////////////
// Old error printing functions

enum class Print {
    error,
    warning,
    verbose,
    debug
};

void print(Print level, const ldp_options& opt, const string& str);
void printSQL(Print level, const ldp_options& opt, const string& sql);

void printSchema(FILE* stream, const ldp_schema& schema);

#endif
