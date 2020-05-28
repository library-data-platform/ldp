#ifndef LDP_OPTIONS_H
#define LDP_OPTIONS_H

#include <string>

#include "../etymoncpp/include/util.h"
#include "dbtype.h"
#include "log.h"

using namespace std;

class DirectExtraction {
public:
    vector<string> tableNames;
    string databaseName;
    string databaseHost;
    string databasePort;
    string databaseUser;
    string databasePassword;
};

class Options {
public:
    string command;
    bool cliMode = false;
    bool init = false;
    string datadir;
    bool extractOnly = false;
    string loadFromDir;
    string source;
    string okapiURL;
    string okapiTenant;
    string okapiUser;
    string okapiPassword;
    //string extractDir;
    DirectExtraction direct;
    string db;
    // Temporary
    string target = "(ldp database)";
    string databaseName = "ldpdev";
    string databaseType = "postgresql";
    string databaseHost = "localhost";
    string databasePort = "5432";
    string ldpAdmin = "ldpadmin";
    string ldpAdminPassword = "nassargres";
    //string target;
    //string databaseName;
    //string databaseType;
    //string databaseHost;
    //string databasePort;
    //string ldpAdmin;
    //string ldpAdminPassword;
    string ldpUser = "ldp";
    string ldpconfigUser = "ldpconfig";
    //DBType dbtype;
    bool unsafe = false;
    string table;
    //bool nossl = false;
    bool disableAnonymization;
    bool savetemps = false;
    FILE* err = stderr;
    bool verbose = false;  // Deprecated.
    bool debug = false;  // Deprecated.
    Level logLevel = Level::debug;
    bool console = false;
    //bool version = false;
    size_t pageSize = 1000;
    int nargc = 0;
    char **nargv = nullptr;
    const char* prog = "ldp";
};

int evalopt(const etymon::CommandArgs& cargs, Options* opt);
void debugOptions(const Options& o);

#endif
