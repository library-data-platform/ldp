#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include "addcolumns.h"

namespace fs = std::filesystem;

void get_add_column_filename(const ldp_options& opt, string* filename)
{
    fs::path datadir = opt.datadir;
    fs::path add_column = datadir / "ldp_add_column.conf";
    *filename = add_column;
}

bool valid_ident(const string& s, bool allow_paren)
{
    if (s == "") {
        return false;
    }
    if (!isalpha(s[0]) && s[0] != '_') {
        return false;
    }
    bool open_p = false;
    bool close_p = false;
    for (auto c : s) {
        if (allow_paren) {
            if (c == ',' && !open_p) {
                return false;
            }
            if (!isalnum(c) && c != '_' && c != '(' && c != ')' && c != ',') {
                return false;
            }
        } else {
            if (!isalnum(c) && c != '_') {
                return false;
            }
        }
        if (open_p && c == '(') {
            return false;
        }
        if (close_p) {
            return false;
        }
        if (c == '(') {
            open_p = true;
        }
        if (c == ')') {
            close_p = true;
        }
    }
    if (open_p && !close_p) {
        return false;
    }
    if (close_p && !open_p) {
        return false;
    }
    return true;
}

void add_columns(const ldp_options& opt, ldp_log* lg, etymon::pgconn* conn)
{
    string filename;
    get_add_column_filename(opt, &filename);
    if ( !(fs::exists(filename)) ) {
        return;
    }
    lg->write(log_level::trace, "", "", "adding optional columns", -1);
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
            throw runtime_error(string("adding column: parsing error: ") + line);
        string table_column = split_space[0];
        string datatype = split_space[1];
        if (!valid_ident(datatype, true))
            throw runtime_error(string("adding column: invalid character in \"") + datatype + "\"");
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
            throw runtime_error(string("adding column: parsing error: ") + line);
        string table = split_dot[0];
        if (!valid_ident(table, false))
            throw runtime_error(string("adding column: invalid character in \"") + table + "\"");
        transform(table.begin(), table.end(), table.begin(), [](unsigned char c){ return tolower(c); });
        string column = split_dot[1];
        if (!valid_ident(column, false))
            throw runtime_error(string("adding column: invalid character in \"") + column + "\"");
        transform(column.begin(), column.end(), column.begin(), [](unsigned char c){ return tolower(c); });
        // Add column
        lg->write(log_level::trace, "", "", "adding column: " + table + "." + column + " " + datatype, -1);
        try {
            string sql = "ALTER TABLE public." + table + " ADD COLUMN \"" + column + "\" " + datatype + ";";
            { etymon::pgconn_result r(conn, sql); }
        } catch (runtime_error& e) {}
    }
}

void add_optional_columns(const ldp_options& opt, ldp_log* lg)
{
    etymon::pgconn conn(opt.dbinfo);
    add_columns(opt, lg, &conn);
}

