#include "../etymoncpp/include/util.h"
#include "util.h"

void print(Print level, const Options& opt, const string& str)
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
        if (opt.verbose || opt.debug)
            fprintf(opt.err, "%s: %s\n", opt.prog, s.c_str());
        break;
    case Print::debug:
        if (opt.debug)
            fprintf(opt.err, "%s: %s\n", opt.prog, s.c_str());
        break;
    }
}

void printSQL(Print level, const Options& opt, const string& sql)
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
