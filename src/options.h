#ifndef LDP_OPTIONS_H
#define LDP_OPTIONS_H

#include <string>

#include "dbtype.h"

using namespace std;

class Options {
public:
    bool etl = false;
    string loadFromDir;
    string extract;
    string okapiURL;
    string okapiTenant;
    string okapiUser;
    string okapiPassword;
    string extractDir;
    string load;
    string databaseName;
    string databaseType;
    string databaseHost;
    string databasePort;
    string databaseUser;
    string databasePassword;
    DBType dbtype;
    bool unsafe = false;
    bool nossl = false;
    //string disableAnonymization;
    bool savetemps = false;
    FILE* err = stderr;
    bool verbose = false;
    bool debug = false;
    //bool version = false;
    bool help = false;
    size_t pageSize = 1000;
    string config;
    int nargc = 0;
    char **nargv = nullptr;
    const char* prog = "ldp";
};

int evalopt(int argc, char *argv[], Options *opt);
void debugOptions(const Options& o);

#endif
