#ifndef LDP_USERS_H
#define LDP_USERS_H

#include "options.h"

bool read_users(const ldp_options& opt, ldp_log* lg, vector<string>* users, bool warn_no_config);

#endif


