#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <stdexcept>
#include "options.h"
#include "err.h"

static void validate(const Options& opt)
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
            opt.command != "init" &&
            opt.command != "update" &&
            opt.command != "help" &&
            opt.command != "")
        throw runtime_error("unknown command: " + opt.command);

    //if (opt.command == "update") {
    //    if (opt.source == "" && opt.loadFromDir == "")
    //        throw runtime_error("update requires --source or --sourcedir");
    //}

    //if (opt.nossl && !opt.unsafe)
    //    throw runtime_error("--nossl requires --unsafe");
    if (opt.extractOnly && !opt.unsafe)
        throw runtime_error("--extract-only requires --unsafe");
    if (opt.savetemps && !opt.unsafe)
        throw runtime_error("--savetemps requires --unsafe");
    if (opt.loadFromDir != "" && !opt.unsafe)
        throw runtime_error("--sourcedir requires --unsafe");
    if (opt.table != "" && !opt.unsafe)
        throw runtime_error("--table requires --unsafe");

    //if (opt.loadFromDir != "" && opt.source != "")
    //    throw runtime_error(
    //            "--source and --sourcedir cannot both be specified");
}

static void evaloptlong(char *name, char *arg, Options* opt)
{
    if (!strcmp(name, "extract-only")) {
        opt->extractOnly = true;
        return;
    }
    if (!strcmp(name, "sourcedir")) {
        opt->loadFromDir = arg;
        return;
    }
    if (!strcmp(name, "table")) {
        opt->table = arg;
        return;
    }
    //if (!strcmp(name, "source")) {
    //    opt->source = arg;
    //    return;
    //}
    if (!strcmp(name, "target")) {
        opt->target = arg;
        return;
    }
    //if (!strcmp(name, "config")) {
    //    opt->config = arg;
    //    return;
    //}
    if (!strcmp(name, "unsafe")) {
        opt->unsafe = true;
        return;
    }
    //if (!strcmp(name, "nossl")) {
    //    opt->nossl = true;
    //    return;
    //}
    if (!strcmp(name, "savetemps")) {
        opt->savetemps = true;
        return;
    }
    if (!strcmp(name, "debug")) {
        opt->debug = true;
        return;
    }
    if (!strcmp(name, "trace")) {
        opt->logLevel = Level::trace;
        return;
    }
    if (!strcmp(name, "detail")) {
        opt->logLevel = Level::detail;
        return;
    }
    if (!strcmp(name, "console")) {
        opt->console = true;
        return;
    }
    //if (!strcmp(name, "version")) {
    //    opt->version = true;
    //    return;
    //}
}

int evalopt(const etymon::CommandArgs& cargs, Options *opt)
{
    static struct option longopts[] = {
        { "extract-only", no_argument,       NULL, 0   },
        { "sourcedir",    required_argument, NULL, 0   },
        { "table",        required_argument, NULL, 0   },
        //{ "source",    required_argument, NULL, 0   },
        //{ "target",    required_argument, NULL, 0   },
        //{ "config",    required_argument, NULL, 0   },
        { "verbose",      no_argument,       NULL, 'v' },
        { "debug",        no_argument,       NULL, 0   },
        { "trace",        no_argument,       NULL, 0   },
        { "detail",       no_argument,       NULL, 0   },
        { "console",      no_argument,       NULL, 0   },
        { "unsafe",       no_argument,       NULL, 0   },
        //{ "nossl",     no_argument,       NULL, 0   },
        { "savetemps",    no_argument,       NULL, 0   },
        //{ "version",   no_argument,       NULL, 0   },
        { 0,              0,                 0,    0   }
    };
    int g, x;

    opt->command = cargs.command;
    if (opt->command == "load")
        opt->command = "update";
    if (opt->command != "server")
        opt->cliMode = true;
    if (opt->command == "init")
        opt->init = true;

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

