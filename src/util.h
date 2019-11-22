#ifndef LDP_UTIL_H
#define LDP_UTIL_H

#include <string>

#include "options.h"

using namespace std;

enum class Print {
    error,
    warning,
    //info,
    verbose,
    debug
};

void print(Print level, const Options& opt, const string& str);
void printSQL(Print level, const Options& opt, const string& sql);

#endif
