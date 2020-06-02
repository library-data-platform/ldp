#ifndef LDP_LDP_H
#define LDP_LDP_H

#include "../etymoncpp/include/util.h"
#include "config.h"
#include "options.h"

int main_cli(int argc, char* const argv[]);

void ldp_main(ldp_options* opt);
void fill_options(const config& conf, ldp_options* opt);

#endif
