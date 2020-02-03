#include <stdexcept>

#include "dbtype.h"

DBType::DBType()
{
    dbt = DBT::unknown;
}

void DBType::setType(const string& dbtype)
{
    if (dbtype == "postgresql" || dbtype == "postgres") {
        dbt = DBT::postgresql;
    } else {
        if (dbtype == "redshift") {
            dbt = DBT::redshift;
        } else {
            string err = "Unknown database type \"";
            err += dbtype;
            err += "\"";
            throw runtime_error(err);
        }
    }
}

const char* DBType::jsonType() const
{
    switch (dbt) {
        case DBT::postgresql:
            //return "JSON";
            return "VARCHAR(65535)";
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
            return "postgresql";
        case DBT::redshift:
            return "redshift";
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


