#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <stdexcept>
#include "options.h"
#include "err.h"

static void validate(const options& opt)
{
    if (opt.verbose) {
        fprintf(stderr, "ldp:    ---------------------------------------------"
                "-----------------------\n");
        fprintf(stderr,
                "ldp:    Verbose output (--verbose or -v) is deprecated.  "
                "Logs are now\n"
                "ldp:    recorded in table: ldpsystem.log\n");
        fprintf(stderr, "ldp:    ---------------------------------------------"
                "-----------------------\n");
    }
    if (opt.debug) {
        fprintf(stderr, "ldp:    ---------------------------------------------"
                "-----------------------\n");
        fprintf(stderr,
                "ldp:    Debugging output (--debug) is deprecated.  "
                "Logs are now recorded\n"
                "ldp:    in table: ldpsystem.log\n"
                "ldp:    The \"--trace\" option enables detailed logging.\n");
        fprintf(stderr, "ldp:    ---------------------------------------------"
                "-----------------------\n");
    }

    if (opt.command != "server" &&
            opt.command != "upgrade-database" &&
            opt.command != "update" &&
            opt.command != "help" &&
            opt.command != "")
        throw runtime_error("unknown command: " + opt.command);
}

static void evaloptlong(char *name, char *arg, options* opt)
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
    if (!strcmp(name, "savetemps")) {
        opt->savetemps = true;
        return;
    }
    if (!strcmp(name, "debug")) {
        opt->debug = true;
        return;
    }
    if (!strcmp(name, "trace")) {
        opt->log_level = Level::trace;
        return;
    }
    if (!strcmp(name, "detail")) {
        opt->log_level = Level::detail;
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

int evalopt(const etymon::command_args& cargs, options *opt)
{
    static struct option longopts[] = {
        { "extract-only", no_argument,       NULL, 0   },
        { "sourcedir",    required_argument, NULL, 0   },
        { "table",        required_argument, NULL, 0   },
        { "verbose",      no_argument,       NULL, 'v' },
        { "debug",        no_argument,       NULL, 0   },
        { "trace",        no_argument,       NULL, 0   },
        { "detail",       no_argument,       NULL, 0   },
        { "console",      no_argument,       NULL, 0   },
        //{ "unsafe",       no_argument,       NULL, 0   },
        { "savetemps",    no_argument,       NULL, 0   },
        { "quiet",        no_argument,       NULL, 0   },
        { 0,              0,                 0,    0   }
    };
    int g, x;

    opt->command = cargs.command;
    if (opt->command == "load")
        opt->command = "update";
    if (opt->command != "server")
        opt->cli_mode = true;
    if (opt->command == "upgrade-database")
        opt->upgrade_database = true;

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
                evaloptlong( (char *) longopts[longindex].name,
                        optarg, opt);
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
    validate(*opt);
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

