#ifndef LDP_OPTIONS_H
#define LDP_OPTIONS_H

#include <string>

#include "../etymoncpp/include/util.h"
#include "dbtype.h"
#include "log.h"

using namespace std;

class direct_extraction {
public:
    vector<string> table_names;
    string database_name;
    string database_host;
    string database_port;
    string database_user;
    string database_password;
};

class options {
public:
    string command;
    bool cli_mode = false;
    bool upgrade_database = false;
    string datadir;
    bool extract_only = false;
    string load_from_dir;
    string source;
    string okapi_url;
    string okapi_tenant;
    string okapi_user;
    string okapi_password;
    //string extractDir;
    direct_extraction direct;
    string db;
    // Temporary
    string target = "(ldp database)";
    string database_name = "ldpdev";
    string database_type = "postgresql";
    string database_host = "localhost";
    string database_port = "5432";
    string ldp_admin = "ldpadmin";
    string ldp_admin_password = "nassargres";
    //string target;
    //string databaseName;
    //string databaseType;
    //string databaseHost;
    //string databasePort;
    //string ldpAdmin;
    //string ldpAdminPassword;
    string ldp_user = "ldp";
    string ldpconfig_user = "ldpconfig";
    //DBType dbtype;
    bool unsafe = false;
    string table;
    //bool nossl = false;
    bool disable_anonymization;
    bool savetemps = false;
    FILE* err = stderr;
    bool verbose = false;  // Deprecated.
    bool debug = false;  // Deprecated.
    Level log_level = Level::debug;
    bool console = false;
    bool quiet = false;
    //bool version = false;
    size_t page_size = 1000;
    int nargc = 0;
    char **nargv = nullptr;
    const char* prog = "ldp";
};

int evalopt(const etymon::CommandArgs& cargs, options* opt);
void debug_options(const options& o);

#endif
