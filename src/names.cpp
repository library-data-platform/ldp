#include "names.h"

void loadingTableName(const string& table, string* newtable)
{
    *newtable = "ldp_" + table;
}

void latestHistoryTableName(const string& table, string* newtable)
{
    *newtable = "history_latest_" + table;
}

void historyTableName(const string& table, string* newtable)
{
    *newtable = "history." + table;
}


