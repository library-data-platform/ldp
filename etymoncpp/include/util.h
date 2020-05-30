#ifndef ETYMON_UTIL_H
#define ETYMON_UTIL_H

#include <string>
#include <vector>

using namespace std;

namespace etymon {

class file {
public:
    FILE* fp;
    file(const string& pathname, const char* mode);
    ~file();
};

class command_args {
public:
    string command;
    int argc;
    char** argv;
    command_args(int argc, char* const argv[]);
    ~command_args();
};

//bool fileExists(const string& filename);

void join(string* s1, const string& s2);
void trim(string* s);
void to_lower(string* s);
void to_upper(string* s);
void split(const string& str, char delim, vector<string>* v);
void prefix_lines(string* str, const char* prefix);

}

#endif
