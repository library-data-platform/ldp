#ifndef LDP_TIMER_H
#define LDP_TIMER_H

#include <cstdio>
#include <chrono>
#include "options.h"

using namespace std;

class Timer {
public:
    chrono::steady_clock::time_point startTime;
    FILE* err;
    const char* prog;
    Timer(const Options& options);
    void restart();
    void print(const char* str);
};

void getCurrentTime(string* str);

#endif
