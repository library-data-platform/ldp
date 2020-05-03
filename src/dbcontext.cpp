#include "dbcontext.h"

DBContext::DBContext(etymon::OdbcDbc* dbc, DBType* dbt, Log* log)
{
    this->dbc = dbc;
    this->dbt = dbt;
    this->log = log;
}

