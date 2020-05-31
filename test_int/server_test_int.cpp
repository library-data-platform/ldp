#include <experimental/filesystem>

#include "../etymoncpp/include/util.h"
#include "../src/server.h"
#include "../src/test.h"

namespace fs = std::experimental::filesystem;

TEST_CASE( "Test update", "[update]" ) {
    options opt;
    opt.cli_mode = true;
    opt.quiet = true;
    opt.command = "update";
    opt.datadir = datadir.data();
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
    opt.table = "user_groups";
    opt.savetemps = true;
    CHECK_NOTHROW( run_opt(&opt) );
    fs::path user_groups_file = fs::path(datadir) / "tmp" / "update" /
        "user_groups_count.txt";
    CHECK( fs::exists(user_groups_file) );
}

string json_data_type_inconsistency = R"(
{
  "usergroups" : [
    {
      "group" : 100,
      "desc" : "Staff Member",
      "id" : "3684a786-6671-4268-8ed0-9db82ebca60b",
      "metadata" : {
        "createdDate" : "2019-11-26T03:33:53.995+0000",
        "updatedDate" : "2019-11-26T03:33:53.995+0000"
      }
    },
    {
      "group" : "faculty",
      "desc" : "Faculty Member",
      "id" : "503a81cd-6c26-400f-b620-14c08943697c",
      "metadata" : {
        "createdDate" : "2019-11-26T03:33:54.019+0000",
        "updatedDate" : "2019-11-26T03:33:54.019+0000"
      }
    },
    {
      "group" : "graduate",
      "desc" : "Graduate Student",
      "id" : "ad0bc554-d5bc-463c-85d1-5562127ae91b",
      "metadata" : {
        "createdDate" : "2019-11-26T03:33:53.990+0000",
        "updatedDate" : "2019-11-26T03:33:53.990+0000"
      }
    },
    {
      "group" : "undergrad",
      "desc" : "Undergraduate Student",
      "id" : "bdc2b6d4-5ceb-4a12-ab46-249b9a68473e",
      "metadata" : {
        "createdDate" : "2019-11-26T03:33:53.982+0000",
        "updatedDate" : "2019-11-26T03:33:53.982+0000"
      }
    }
  ],
  "totalRecords" : 4
}
)";

TEST_CASE( "Test inconsistent JSON data types", "[update]" ) {
    fs::path update_dir = fs::path(datadir) / "tmp" / "update";
    fs::remove_all(update_dir);
    fs::create_directories(update_dir);
    {
        etymon::file f(update_dir / "user_groups_0.json", "w");
        fputs(json_data_type_inconsistency.data(), f.fp);
    }
    {
        etymon::file f(update_dir / "user_groups_count.txt", "w");
        fputs("1\n", f.fp);
    }
    options opt;
    opt.cli_mode = true;
    opt.quiet = true;
    opt.command = "update";
    opt.datadir = datadir.data();
    opt.table = "user_groups";
    opt.load_from_dir = update_dir;
    CHECK_NOTHROW( run_opt(&opt) );
    fs::remove_all(update_dir);
}

