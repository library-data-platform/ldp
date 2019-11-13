#include "timer.h"

Timer::Timer(const Options& options)
{
    restart();
    err = options.err;
    prog = options.prog;
}

void Timer::restart()
{
    startTime = chrono::steady_clock::now();
}

void Timer::print(const char* str)
{
    chrono::duration<double> elapsed =
        chrono::steady_clock::now() - startTime;
    fprintf(err, "%s: %s: %.4f s\n", prog, str, elapsed.count());
}

void getCurrentTime(string* str)
{
    time_t now;
    time(&now);
    char t[sizeof "YYYY-mm-dd HH:MM:SS"];
    strftime(t, sizeof t, "%Y-%m-%d %H:%M:%S", localtime(&now));
    *str = t;
}

