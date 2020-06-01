#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <stdexcept>
#include "options.h"
#include "err.h"

void config_set_profile(const string& profile_str, profile* prof)
{
    if (profile_str == "folio") {
        *prof = profile::folio;
        return;
    }
    if (profile_str == "") {
        *prof = profile::none;
        return;
    }
    throw runtime_error("Unknown profile: " + profile_str);
}

static void evaloptlong(char* name, char* arg, options* opt)
{
    if (!strcmp(name, "extract-only")) {
        opt->extract_only = true;
        return;
    }
    if (!strcmp(name, "sourcedir")) {
        opt->load_from_dir = arg;
        return;
    }
    if (!strcmp(name, "table")) {
        opt->table = arg;
        return;
    }
    if (!strcmp(name, "profile")) {
        config_set_profile(arg, &(opt->set_profile));
        return;
    }
    if (!strcmp(name, "savetemps")) {
        opt->savetemps = true;
        return;
    }
    if (!strcmp(name, "debug")) {
        opt->debug = true;
        return;
    }
    if (!strcmp(name, "trace")) {
        opt->log_level = level::trace;
        return;
    }
    if (!strcmp(name, "detail")) {
        opt->log_level = level::detail;
        return;
    }
    if (!strcmp(name, "console")) {
        opt->console = true;
        return;
    }
    if (!strcmp(name, "quiet")) {
        opt->quiet = true;
        return;
    }
}

void config_set_command(const string& command_str, ldp_command* command)
{
    if (command_str == "server") {
        *command = ldp_command::server;
        return;
    }
    if (command_str == "upgrade-database") {
        *command = ldp_command::upgrade_database;
        return;
    }
    if (command_str == "init") {
        *command = ldp_command::init;
        return;
    }
    if (command_str == "update") {
        *command = ldp_command::update;
        return;
    }
    if (command_str == "help") {
        *command = ldp_command::help;
        return;
    }
    throw runtime_error("Unknown command: " + command_str);
}

int evalopt(const etymon::command_args& cargs, options *opt)
{
    static struct option longopts[] = {
        { "console",      no_argument,       NULL, 0   },
        { "debug",        no_argument,       NULL, 0   },
        { "detail",       no_argument,       NULL, 0   },
        { "extract-only", no_argument,       NULL, 0   },
        { "profile",      required_argument, NULL, 0   },
        { "quiet",        no_argument,       NULL, 0   },
        { "sourcedir",    required_argument, NULL, 0   },
        { "savetemps",    no_argument,       NULL, 0   },
        { "table",        required_argument, NULL, 0   },
        { "trace",        no_argument,       NULL, 0   },
        { "verbose",      no_argument,       NULL, 'v' },
        { 0,              0,                 0,    0   }
    };
    int g, x;

    string command_str = cargs.command;
    config_set_command(command_str, &(opt->command));
    if (opt->command != ldp_command::server)
        opt->cli_mode = true;

    while (1) {
        int longindex = 0;
        g = getopt_long(cargs.argc, cargs.argv,
                "D:hv",
                longopts, &longindex);
        if (g == -1)
            break;
        if (g == '?' || g == ':')
            return -1;
        switch (g) {
            case 0:
                evaloptlong((char *) longopts[longindex].name, optarg, opt);
                break;
            case 'D':
                opt->datadir = optarg;
                break;
            case 'v':
                opt->verbose = true;
                break;
        }
    }
    if (optind < cargs.argc) {
        opt->nargv = cargs.argv + optind;
        opt->nargc = cargs.argc - optind;
    }
    if (opt->nargc > 0) {
        printf("%s: unrecognized extra arguments `", opt->prog);
        for (x = 0; x < opt->nargc; x++) {
            if (x > 0)
                printf(" ");
            printf("%s", opt->nargv[x]);
        }
        printf("'\n");
        return -1;
    }
    return 0;
}

void config_set_environment(const string& env_str, deployment_environment* env)
{
    if (env_str == "production") {
        *env = deployment_environment::production;
        return;
    }
    if (env_str == "staging") {
        *env = deployment_environment::staging;
        return;
    }
    if (env_str == "testing") {
        *env = deployment_environment::testing;
        return;
    }
    if (env_str == "development") {
        *env = deployment_environment::development;
        return;
    }
    throw runtime_error("Unknown deployment environment: " + env_str);
}

