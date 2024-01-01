#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include "users.h"

namespace fs = std::filesystem;

void get_users_filename(const ldp_options& opt, string* filename)
{
    fs::path datadir = opt.datadir;
    fs::path users = datadir / "ldp_users.conf";
    *filename = users;
}

bool read_users(const ldp_options& opt, ldp_log* lg, vector<string>* users, bool warn_no_config)
{
    string filename;
    get_users_filename(opt, &filename);
    if ( !(fs::exists(filename)) ) {
        if (warn_no_config) {
            fprintf(stderr, "ldp: config file \"%s\" not found\n", filename.data());
            return false;
        } else {
            return true;
        }
    }
    if (opt.superuser == "" || opt.superpassword == "") {
        lg->write(log_level::warning, "server", "", "user names in \"" + filename + "\" ignored because database_super_user or database_super_password is not set", -1);
        return false;
    }
    vector<string> usr;
    ifstream infile(filename);
    string line;
    while (getline(infile, line)) {
        etymon::trim(&line);
        if (line == "")
            continue;
        string u = line;
        transform(u.begin(), u.end(), u.begin(), [](unsigned char c){ return tolower(c); });
        usr.push_back(u);
    }
    etymon::pgconn_info dbi = opt.dbinfo;
    dbi.dbuser = opt.superuser;
    dbi.dbpasswd = opt.superpassword;
    etymon::pgconn conn(dbi);
    bool ok = true;
    for (auto& u : usr) {
        try {
            string sql = "GRANT USAGE ON SCHEMA public TO " + u + ";";
            lg->detail(sql);
            { etymon::pgconn_result r(&conn, sql); }
            sql = "GRANT USAGE ON SCHEMA history TO " + u + ";";
            lg->detail(sql);
            { etymon::pgconn_result r(&conn, sql); }
            sql = "GRANT CREATE, USAGE ON SCHEMA local TO " + u + ";";
            lg->detail(sql);
            { etymon::pgconn_result r(&conn, sql); }
            sql = "GRANT SELECT ON ALL TABLES IN SCHEMA public TO " + u + ";";
            lg->detail(sql);
            { etymon::pgconn_result r(&conn, sql); }
            sql = "GRANT SELECT ON ALL TABLES IN SCHEMA history TO " + u + ";";
            lg->detail(sql);
            { etymon::pgconn_result r(&conn, sql); }
            users->push_back(u);
        } catch (runtime_error& e) {
            ok = false;
            lg->write(log_level::error, "server", "", "unable to set permissions for user \"" + u + "\"", -1);
        }
    }
    return ok;
}

