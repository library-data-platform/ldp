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

typedef void (*database_upgrade_array)(database_upgrade_options* opt);

void init_upgrade(etymon::odbc_env* odbc, const string& dbname,
        const string& ldpUser, const string& ldpconfigUser,
        const string& datadir, FILE* err, const char* prog);

void catalog_add_table(etymon::odbc_conn* conn, const string& table);

#endif

