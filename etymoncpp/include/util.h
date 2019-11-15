#ifndef ETYMON_UTIL_H
#define ETYMON_UTIL_H

#include <string>
#include <vector>

using namespace std;

namespace etymon {

class File {
public:
    FILE* file;
    File(const string& pathname, const char* mode);
    ~File();
};

class CommandArgs {
public:
    string command;
    int argc;
    char** argv;
    CommandArgs(int argc, char* const argv[]);
    ~CommandArgs();
};

bool fileExists(const string& filename);

void join(string* s1, const string& s2);
void trim(string* s);
void split(const string& str, char delim, vector<string>* v);
void prefixLines(string* str, const char* prefix);

}

#endif
