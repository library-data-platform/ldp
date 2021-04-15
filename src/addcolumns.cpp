#define _LIBCPP_NO_EXPERIMENTAL_DEPRECATION_WARNING_FILESYSTEM

#include <algorithm>
#include <experimental/filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include "addcolumns.h"

namespace fs = std::experimental::filesystem;

void get_addcolumns_filename(const ldp_options& opt, string* filename)
{
    fs::path datadir = opt.datadir;
    fs::path addcolumns = datadir / "ldp_column.conf";
    *filename = addcolumns;
}

void add_columns(const ldp_options& opt, ldp_log* lg, etymon::odbc_conn* conn)
{
    string filename;
    get_addcolumns_filename(opt, &filename);
    if ( !(fs::exists(filename)) ) {
        lg->write(log_level::detail, "", "", "Skip adding columns: file not found: " + filename, -1);
        return;
    }
    lg->write(log_level::trace, "", "", "Adding optional columns", -1);
    ifstream infile(filename);
    string line;
    while (getline(infile, line)) {
        etymon::trim(&line);
        if (line == "")
            continue;
        // Parse table.column and type
        vector<string> split_space;
        stringstream ss_space(line);
        string split_space_token;
        while (getline(ss_space, split_space_token, ' ')) {
            etymon::trim(&split_space_token);
            if (split_space_token != "")
                split_space.push_back(split_space_token);
        }
        if (split_space.size() != 2)
            throw runtime_error(string("Error parsing optional column: ") + line);
        string table_column = split_space[0];
        string datatype = split_space[1];
        transform(datatype.begin(), datatype.end(), datatype.begin(), [](unsigned char c){ return tolower(c); });
        if (datatype == "varchar")
            datatype = "varchar(1)";
        // Parse table and column
        vector<string> split_dot;
        stringstream ss_dot(table_column);
        string split_dot_token;
        while (getline(ss_dot, split_dot_token, '.')) {
            split_dot.push_back(split_dot_token);
        }
        if (split_dot.size() != 2)
            throw runtime_error(string("Error parsing optional column: ") + line);
        string table = split_dot[0];
        transform(table.begin(), table.end(), table.begin(), [](unsigned char c){ return tolower(c); });
        string column = split_dot[1];
        transform(column.begin(), column.end(), column.begin(), [](unsigned char c){ return tolower(c); });
        // Add column
        lg->write(log_level::detail, "", "", "Adding column: " + table + "." + column + " " + datatype, -1);
        try {
            string sql = "ALTER TABLE public." + table + " ADD COLUMN \"" + column + "\" " + datatype + ";";
            conn->exec(sql);
        } catch (runtime_error& e) {}
    }
    return;
}

void add_optional_columns(const ldp_options& opt, ldp_log* lg, etymon::odbc_env* odbc)
{
    etymon::odbc_conn conn(odbc, opt.db);
    add_columns(opt, lg, &conn);
}

