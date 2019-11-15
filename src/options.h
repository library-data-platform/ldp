#ifndef LDP_OPTIONS_H
#define LDP_OPTIONS_H

#include <string>

#include "../etymoncpp/include/util.h"
#include "dbtype.h"

using namespace std;

class Options {
public:
    string command;
    string loadFromDir;
    string okapi;
    string okapiURL;
    string okapiTenant;
    string okapiUser;
    string okapiPassword;
    string extractDir;
    string database;
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
    size_t pageSize = 1000;
    string config;
    int nargc = 0;
    char **nargv = nullptr;
    const char* prog = "ldp";
};

int evalopt(const etymon::CommandArgs& cargs, Options *opt);
void debugOptions(const Options& o);

#endif
