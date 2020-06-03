#ifndef LDP_INIT_H
#define LDP_INIT_H

#include "log.h"

class database_upgrade_options {
public:
    FILE* ulog;
    etymon::odbc_conn* conn;
    string ldp_user;
    string ldpconfig_user;
    string datadir;
};

int64_t latest_database_version();

typedef void (*database_upgrade_array)(database_upgrade_options* opt);

void init_database(etymon::odbc_env* odbc, const string& dbname,
        const string& ldpUser, const string& ldpconfigUser, FILE* err,
        const char* prog);

void upgrade_database(etymon::odbc_env* odbc, const string& dbname,
        const string& ldpUser, const string& ldpconfigUser,
        const string& datadir, FILE* err, const char* prog, bool quiet);

void validate_database_latest_version(etymon::odbc_env* odbc,
        const string& dbname);

void catalog_add_table(etymon::odbc_conn* conn, const string& table);

#endif

