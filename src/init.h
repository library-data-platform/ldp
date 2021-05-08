#ifndef LDP_INIT_H
#define LDP_INIT_H

#include "log.h"
#include "options.h"

class database_upgrade_options {
public:
    FILE* ulog;
    etymon::pgconn* conn;
    string ldp_user;
    string ldpconfig_user;
    string datadir;
};

int64_t latest_database_version();

typedef void (*database_upgrade_array)(database_upgrade_options* opt);

void init_database(const ldp_options& opt, const string& ldp_user, const string& ldpconfig_user);

void upgrade_database(const ldp_options& opt, const string& ldp_user, const string& ldpconfig_user, const string& datadir, bool quiet);

void validate_database_latest_version(const ldp_options& opt);

void catalog_add_table(etymon::pgconn* conn, const string& table);

#endif

