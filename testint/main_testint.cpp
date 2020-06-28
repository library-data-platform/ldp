#define CATCH_CONFIG_RUNNER

#include "../src/config.h"
#include "../src/options.h"
#include "../src/ldp.h"
#include "../test/test.h"

using namespace std;
using namespace Catch::clara;

string datadir;

void safety_checks(char* argv0)
{
    ldp_options opt;
    ldp_config conf(datadir + "/ldpconf.json");
    config_options(conf, &opt);
    if (opt.deploy_env != deployment_environment::testing &&
            opt.deploy_env != deployment_environment::development) {
        fprintf(stderr,
                "%s: Invalid configuration for integration tests:\n"
                "    Configuration setting: deployment_environment\n",
                argv0);
        exit(1);
    }
    if (!opt.allow_destructive_tests) {
        fprintf(stderr,
                "%s: Invalid configuration for integration tests:\n"
                "    Configuration setting: allow_destructive_tests\n",
                argv0);
        exit(1);
    }
}

int main(int argc, char* argv[]) {

    Catch::Session session;

    auto cli = session.cli()
        | Opt(datadir, "path")
        ["-D"]
        ("Specify LDP data directory");

    session.cli(cli);

    int r = session.applyCommandLine(argc, argv);
    if (r)
        return r;

    safety_checks(argv[0]);

    return session.run();
}

