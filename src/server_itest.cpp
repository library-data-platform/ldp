#include <experimental/filesystem>

#include "test.h"
#include "server.h"

namespace fs = std::experimental::filesystem;

TEST_CASE( "Test update", "[update]" ) {
    options opt;
    opt.cli_mode = true;
    opt.quiet = true;
    opt.command = "update";
    opt.datadir = datadir.data();
    opt.unsafe = true;
    opt.table = "user_groups";
    CHECK_NOTHROW( run_opt(&opt) );
    fs::path update_dir = fs::path(datadir) / "tmp" / "update";
    CHECK_FALSE( fs::exists(update_dir) );
}

TEST_CASE( "Test update with savetemps", "[update]" ) {
    options opt;
    opt.cli_mode = true;
    opt.quiet = true;
    opt.command = "update";
    opt.datadir = datadir.data();
    opt.unsafe = true;
    opt.table = "user_groups";
    opt.savetemps = true;
    CHECK_NOTHROW( run_opt(&opt) );
    fs::path user_groups_file = fs::path(datadir) / "tmp" / "update" /
        "user_groups_count.txt";
    CHECK( fs::exists(user_groups_file) );
}

