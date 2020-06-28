#include "names.h"

void loading_table_name(const string& table, string* newtable)
{
    *newtable = "ldp_" + table;
}

void latest_history_table_name(const string& table, string* newtable)
{
    *newtable = "history_latest_" + table;
}

void history_table_name(const string& table, string* newtable)
{
    *newtable = "history." + table;
}


