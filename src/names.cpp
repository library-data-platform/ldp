#include "names.h"

void stagingTableName(const string& table, string* newtable)
{
    (*newtable) = "stage_";
    (*newtable) += table;
}

void loadingTableName(const string& table, string* newtable)
{
    (*newtable) = "load_";
    (*newtable) += table;
}


