#ifndef LDP_SERVER_H
#define LDP_SERVER_H

#include "../etymoncpp/include/util.h"
#include "config.h"
#include "options.h"

int main_cli(int argc, char* const argv[]);

void run_opt(options* opt);
void fill_options(const config& conf, options* opt);

#endif
