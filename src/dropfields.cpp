#define _LIBCPP_NO_EXPERIMENTAL_DEPRECATION_WARNING_FILESYSTEM

#include <algorithm>
#include <experimental/filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include "dropfields.h"

namespace fs = std::experimental::filesystem;

void get_drop_field_filename(const ldp_options& opt, string* filename)
{
    fs::path datadir = opt.datadir;
    fs::path drop_field = datadir / "ldp_drop_field.conf";
    *filename = drop_field;
}

void read_drop_fields(const ldp_options& opt, ldp_log* lg, field_set* drop_fields)
{
    pair<string,string> p;
    string filename;
    get_drop_field_filename(opt, &filename);
    if ( !(fs::exists(filename)) ) {
        return;
    }
    ifstream infile(filename);
    string line;
    while (getline(infile, line)) {
        etymon::trim(&line);
        if (line == "")
            continue;
        // Parse table and field
        vector<string> split_space;
        stringstream ss_space(line);
        string split_space_token;
        while (getline(ss_space, split_space_token, ' ')) {
            etymon::trim(&split_space_token);
            if (split_space_token != "")
                split_space.push_back(split_space_token);
        }
        if (split_space.size() != 2)
            throw runtime_error(string("drop field list: parsing error: ") + line);
        string table = split_space[0];
        string field = split_space[1];
        transform(table.begin(), table.end(), table.begin(), [](unsigned char c){ return tolower(c); });
        p = pair<string,string>(table, field);
        drop_fields->fields.insert(p);
    }
}

