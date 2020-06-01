#ifndef LDP_TIMER_H
#define LDP_TIMER_H

#include <cstdio>
#include <chrono>

#include "options.h"

using namespace std;

class timer {
public:
    chrono::steady_clock::time_point start_time;
    FILE* err;
    const char* prog;
    timer();
    //Timer(FILE* err, const char* prog);
    timer(const options& options);
    void restart();
    double elapsed_time() const;
    void print(const char* str) const;
};

void get_current_time(string* str);

#endif
