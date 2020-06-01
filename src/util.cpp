#include <cstring>

#include "../etymoncpp/include/util.h"
#include "util.h"

bool isUUID(const char* str)
{
    if (strlen(str) != 36)
        return false;
    for (int x = 0; x < 36; x++) {
        char c = str[x];
        if (x == 8 || x == 13 || x == 18 || x == 23) {
            if (c != '-')
                return false;
        } else {
            switch (c) {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            case 'A':
            case 'B':
            case 'C':
            case 'D':
            case 'E':
            case 'F':
            case 'a':
            case 'b':
            case 'c':
            case 'd':
            case 'e':
            case 'f':
                break;
            default:
                return false;
            }
        }
    }
    return true;
}

void print_banner_line(FILE* stream, char ch, int width)
{
    for (int x = 0; x < width; x++)
        fputc(ch, stream);
    fputc('\n', stream);
}

////////////////////////////////////////////////////////////////////////////
// Old error printing functions

void print(Print level, const options& opt, const string& str)
{
    string s = str;
    etymon::trim(&s);
    switch (level) {
    case Print::error:
        fprintf(opt.err, "%s: error: %s\n", opt.prog, s.c_str());
        break;
    case Print::warning:
        fprintf(opt.err, "%s: warning: %s\n", opt.prog, s.c_str());
        break;
    //case Print::info:
    //    fprintf(opt.err, "%s: %s\n", opt.prog, s.c_str());
    //    break;
    case Print::verbose:
        if (opt.log_level == level::trace)
            fprintf(opt.err, "%s: %s\n", opt.prog, s.c_str());
        break;
    case Print::debug:
        if (opt.log_level == level::trace)
            fprintf(opt.err, "%s: %s\n", opt.prog, s.c_str());
        break;
    }
}

void printSQL(Print level, const options& opt, const string& sql)
{
    print(level, opt, string("sql:\n") + sql);
}

void printSchema(FILE* stream, const Schema& schema)
{
    fprintf(stream, "Module name,Source path,Table name\n");
    for (const auto& table : schema.tables) {
        fprintf(stream, "%s,%s,%s\n", table.moduleName.c_str(),
                table.sourcePath.c_str(), table.tableName.c_str());
    }
}
