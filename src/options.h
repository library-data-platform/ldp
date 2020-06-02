#ifndef LDP_OPTIONS_H
#define LDP_OPTIONS_H

#include <string>

#include "../etymoncpp/include/util.h"
#include "dbtype.h"
#include "log.h"

using namespace std;

enum class ldp_command {
    server,
    upgrade_database,
    init,
    update,
    help
};

enum class profile {
    folio,
    none
};

enum class deployment_environment {
    production,
    staging,
    testing,
    development
};

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
    ldp_command command;
    profile set_profile = profile::none;
    bool cli_mode = false;
    string datadir;
    bool extract_only = false;
    string load_from_dir;
    string source;
    string okapi_url;
    string okapi_tenant;
    string okapi_user;
    string okapi_password;
    direct_extraction direct;
    deployment_environment deploy_env;
    string db;
    string ldp_user = "ldp";
    string ldpconfig_user = "ldpconfig";
    //bool unsafe = false;
    string table;
    bool disable_anonymization;
    bool savetemps = false;
    FILE* err = stderr;
    bool verbose = false;  // Deprecated.
    bool debug = false;  // Deprecated.
    level log_level = level::debug;
    bool console = false;
    bool quiet = false;
    size_t page_size = 1000;
    int nargc = 0;
    char **nargv = nullptr;
    const char* prog = "ldp";
    bool allow_destructive_tests = false;
};

int evalopt(const etymon::command_args& cargs, options* opt);
void debug_options(const options& o);
void config_set_environment(const string& env_str, deployment_environment* env);

#endif
