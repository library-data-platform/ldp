#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <stdexcept>
#include "options.h"
#include "err.h"

static void check_conflicts(const Options& opt)
{
    if (opt.etl) {
        if (opt.extract == "" && opt.loadFromDir == "")
            throw runtime_error("--etl requires --extract or --dir");
        if (opt.load == "")
            throw runtime_error("--etl requires --load");
    }

    if (opt.nossl && !opt.unsafe)
        throw runtime_error("--nossl requires --unsafe");
    if (opt.savetemps && !opt.unsafe)
        throw runtime_error("--savetemps requires --unsafe");
    if (opt.loadFromDir != "" && !opt.unsafe)
        throw runtime_error("--dir requires --unsafe");

    if (opt.loadFromDir != "" && opt.extract != "")
        throw runtime_error("--extract and --dir cannot both be specified");
}

static void evaloptlong(char *name, char *arg, Options *opt)
{
    if (!strcmp(name, "etl")) {
        opt->etl = true;
        return;
    }
    if (!strcmp(name, "dir")) {
        opt->loadFromDir = arg;
        return;
    }
    if (!strcmp(name, "extract")) {
        opt->extract = arg;
        return;
    }
    if (!strcmp(name, "load")) {
        opt->load = arg;
        return;
    }
    if (!strcmp(name, "config")) {
        opt->config = arg;
        return;
    }
    if (!strcmp(name, "unsafe")) {
        opt->unsafe = true;
        return;
    }
    if (!strcmp(name, "nossl")) {
        opt->nossl = true;
        return;
    }
    if (!strcmp(name, "savetemps")) {
        opt->savetemps = true;
        return;
    }
    if (!strcmp(name, "debug")) {
        opt->debug = true;
        opt->verbose = true;
        return;
    }
    //if (!strcmp(name, "version")) {
    //    opt->version = true;
    //    return;
    //}
}

int evalopt(int argc, char *argv[], Options* opt)
{
    static struct option longopts[] = {
        { "etl",       no_argument,       NULL, 0   },
        { "dir",       required_argument, NULL, 0   },
        { "extract",   required_argument, NULL, 0   },
        { "load",      required_argument, NULL, 0   },
        { "config",    required_argument, NULL, 0   },
        { "verbose",   no_argument,       NULL, 'v' },
        { "debug",     no_argument,       NULL, 0   },
        { "unsafe",    no_argument,       NULL, 0   },
        { "nossl",     no_argument,       NULL, 0   },
        { "savetemps", no_argument,       NULL, 0   },
        //{ "version",   no_argument,       NULL, 0   },
        { "help",      no_argument,       NULL, 'h' },
        { 0,           0,                 0,    0   }
    };
    int g, x;

    while (1) {
        int longindex = 0;
        g = getopt_long(argc, argv,
                "hv",
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
            case 'h':
                opt->help = true;
                break;
            case 'v':
                opt->verbose = true;
                break;
        }
    }
    if (optind < argc) {
        opt->nargv = argv + optind;
        opt->nargc = argc - optind;
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
    check_conflicts(*opt);
    return 0;
}

void debugOptions(const Options& opt)
{
    fprintf(stderr, "%s: option: etl = %d\n", opt.prog, opt.etl);
    fprintf(stderr, "%s: option: loadFromDir = %s\n", opt.prog,
            opt.loadFromDir.c_str());
    fprintf(stderr, "%s: option: extract = %s\n", opt.prog,
            opt.extract.c_str());
    fprintf(stderr, "%s: option: okapiURL = %s\n", opt.prog,
            opt.okapiURL.c_str());
    fprintf(stderr, "%s: option: okapiTenant = %s\n", opt.prog,
            opt.okapiTenant.c_str());
    fprintf(stderr, "%s: option: okapiUser = %s\n", opt.prog,
            opt.okapiUser.c_str());
    fprintf(stderr, "%s: option: extractDir = %s\n", opt.prog,
            opt.extractDir.c_str());
    fprintf(stderr, "%s: option: load = %s\n", opt.prog, opt.load.c_str());
    fprintf(stderr, "%s: option: databaseName = %s\n", opt.prog,
            opt.databaseName.c_str());
    fprintf(stderr, "%s: option: databaseType = %s\n", opt.prog,
            opt.databaseType.c_str());
    fprintf(stderr, "%s: option: databaseHost = %s\n", opt.prog,
            opt.databaseHost.c_str());
    fprintf(stderr, "%s: option: databasePort = %s\n", opt.prog,
            opt.databasePort.c_str());
    fprintf(stderr, "%s: option: databaseUser = %s\n", opt.prog,
            opt.databaseUser.c_str());
    fprintf(stderr, "%s: option: dbtype = %s\n", opt.prog, opt.dbtype.dbType());
    //fprintf(stderr, "%s: option: err = ", opt.prog);
    //if (opt.err == stdout)
    //    fprintf(stderr, "stdout\n");
    //else if (opt.err == stderr)
    //    fprintf(stderr, "stderr\n");
    //else
    //    fprintf(stderr, "%p\n", opt.err);
    fprintf(stderr, "%s: option: unsafe = %d\n", opt.prog, opt.unsafe);
    fprintf(stderr, "%s: option: nossl = %d\n", opt.prog, opt.nossl);
    fprintf(stderr, "%s: option: savetemps = %d\n", opt.prog, opt.savetemps);
    fprintf(stderr, "%s: option: config = %s\n", opt.prog, opt.config.c_str());
    fprintf(stderr, "%s: option: verbose = %d\n", opt.prog, opt.verbose);
    fprintf(stderr, "%s: option: debug = %d\n", opt.prog, opt.debug);
    //fprintf(stderr, "%s: option: version = %d\n", opt.prog, opt.version);
    fprintf(stderr, "%s: option: help = %d\n", opt.prog, opt.help);
}

