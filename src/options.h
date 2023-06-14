#ifndef LDP_OPTIONS_H
#define LDP_OPTIONS_H

#include <map>
#include <string>
#include <vector>

#include "../etymoncpp/include/postgres.h"
#include "../etymoncpp/include/util.h"
#include "dbtype.h"
#include "log.h"

using namespace std;

enum class ldp_command {
    server,
    upgrade_database,
    list_tables,
    list_privileges,
    init_database,
    update,
    update_users,
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
    int database_port;
    string database_user;
    string database_password;
};

class data_source {
public:
    string source_name;
    string okapi_url;
    string okapi_tenant;
    string okapi_user;
    string okapi_password;
    direct_extraction direct;
};

class ldp_options {
public:
    ldp_command command;
    profile set_profile = profile::folio;
    //bool no_update = false;
    bool cli_mode = false;
    string datadir;
    bool extract_only = false;
    string load_from_dir;
    ///////////////////////////////////////////////////////////////////////////
    // NEW SOURCE CONFIG
    vector<data_source> enable_sources;
    ///////////////////////////////////////////////////////////////////////////
    // OLD SOURCE CONFIG
    //string source;
    //string okapi_url;
    //string okapi_tenant;
    //string okapi_user;
    //string okapi_password;
    //direct_extraction direct;
    ///////////////////////////////////////////////////////////////////////////
    deployment_environment deploy_env;
    //string db;
    etymon::pgconn_info dbinfo;
    string superuser;
    string superpassword;
    string ldp_user = "ldp";
    string ldpconfig_user = "ldpconfig";
    //bool unsafe = false;
    string table;
    bool anonymize = true;
    bool record_history = true;
    bool all_indexes = false;
    bool parallel_vacuum = true;
    bool parallel_update = true;
    bool index_large_varchar = false;
    bool savetemps = false;
    //FILE* err = stderr;
    bool verbose = false;  // Deprecated.
    bool debug = false;  // Deprecated.
    log_level lg_level = log_level::debug;
    bool console = false;
    bool quiet = false;
    bool single_process = true;
    bool direct_extraction_no_ssl = false;
    int okapi_timeout = 60;
    size_t page_size = 1000;
    int nargc = 0;
    char **nargv = nullptr;
    bool allow_destructive_tests = false;
};

int evalopt(const etymon::command_args& cargs, ldp_options* opt);
void debug_options(const ldp_options& o);
void config_set_environment(const string& env_str, deployment_environment* env);

#endif
