#include <stdexcept>

#include "dbtype.h"

//DBType::DBType()
//{
//    dbt = DBT::unknown;
//}

DBType::DBType(etymon::OdbcDbc* dbc)
{
    string dbmsName;
    dbc->getDbmsName(&dbmsName);
    setType(dbmsName);
}

void DBType::setType(const string& dbms)
{
    if (dbms == "PostgreSQL") {
        dbt = DBT::postgresql;
    } else {
        if (dbms == "Redshift") {
            dbt = DBT::redshift;
        } else {
            string err = "Unknown database system: \"";
            err += dbms;
            err += "\"";
            throw runtime_error(err);
        }
    }
}

const char* DBType::jsonType() const
{
    switch (dbt) {
        case DBT::postgresql:
            return "JSON";
        case DBT::redshift:
            return "VARCHAR(65535)";
        default:
            return "(unknown)";
    }
}

const char* DBType::dbType() const
{
    switch (dbt) {
        case DBT::postgresql:
            return "PostgreSQL";
        case DBT::redshift:
            return "Redshift";
        default:
            return "(unknown)";
    }
}

static void encodeStr(const char* str, string* newstr, bool e)
{
    if (e)
        (*newstr) = "E'";
    else
        (*newstr) = '\'';
    const char *p = str;
    char c;
    while ( (c=*p) != '\0') {
        switch (c) {
            case '\\':
                (*newstr) += "\\\\";
                break;
            case '\'':
                (*newstr) += "''";
                break;
            case '\b':
                (*newstr) += "\\b";
                break;
            case '\f':
                (*newstr) += "\\f";
                break;
            case '\n':
                (*newstr) += "\\n";
                break;
            case '\r':
                (*newstr) += "\\r";
                break;
            case '\t':
                (*newstr) += "\\t";
                break;
            default:
                (*newstr) += c;
        }
        p++;
    }
    (*newstr) += '\'';
}

void DBType::encodeStringConst(const char* str, string* newstr) const
{
    switch (dbt) {
        case DBT::postgresql:
            encodeStr(str, newstr, true);
            break;
        case DBT::redshift:
            encodeStr(str, newstr, false);
            break;
        case DBT::unknown:
            (*newstr) = "(unknown)";
            break;
    }
}

void DBType::redshiftKeys(const char* distkey, const char* sortkey,
            string* sql) const
{
    switch (dbt) {
        case DBT::postgresql:
            (*sql) = "";
            break;
        case DBT::redshift:
            (*sql) = string(" DISTKEY(") + distkey +
                string(") COMPOUND SORTKEY(") + sortkey + string(")");
            break;
        case DBT::unknown:
            (*sql) = "(unknown)";
            break;
    }
}


