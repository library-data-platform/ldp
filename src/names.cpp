#include "names.h"

void loadingTableName(const string& table, string* newtable)
{
    *newtable = "load_" + table;
}

void latestHistoryTableName(const string& table, string* newtable)
{
    *newtable = "latest_" + table;
}

void historyTableName(const string& table, string* newtable)
{
    *newtable = "history." + table;
}


