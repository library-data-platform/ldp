#define CATCH_CONFIG_RUNNER

#include "../test.h"

using namespace std;
using namespace Catch::clara;

string datadir;

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

    return session.run();
}

