#ifndef LDP_LDP_H
#define LDP_LDP_H

#include "../etymoncpp/include/util.h"
#include "config.h"
#include "options.h"

int main_ldp(int argc, char* const argv[]);

void ldp_exec(ldp_options* opt);
void config_options(const ldp_config& conf, ldp_options* opt);

#endif
