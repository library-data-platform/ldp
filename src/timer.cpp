#include "timer.h"

timer::timer()
{
    restart();
    err = stderr;
    prog = "ldp";
}

//timer::timer(FILE* err, const char* prog)
//{
//    restart();
//    this->err = err;
//    this->prog = prog;
//}

timer::timer(const options& options)
{
    restart();
    err = options.err;
    prog = options.prog;
}

void timer::restart()
{
    start_time = chrono::steady_clock::now();
}

double timer::elapsed_time() const
{
    chrono::duration<double> elapsed = chrono::steady_clock::now() - start_time;
    return elapsed.count();
}

void timer::print(const char* str) const
{
    chrono::duration<double> elapsed =
        chrono::steady_clock::now() - start_time;
    fprintf(err, "%s: %s: %.4f s\n", prog, str, elapsed.count());
}

void get_current_time(string* str)
{
    time_t now;
    time(&now);
    char t[sizeof "YYYY-mm-dd HH:MM:SS"];
    strftime(t, sizeof t, "%Y-%m-%d %H:%M:%S", localtime(&now));
    *str = t;
}

