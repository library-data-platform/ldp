#ifndef LDP_DBTYPE_H
#define LDP_DBTYPE_H

#include <string>

#include "../etymoncpp/include/odbc.h"

using namespace std;

enum class DBT {
    postgresql,
    redshift,
    unknown
};

class DBType {
public:
    //DBType();
    DBType(etymon::OdbcDbc* dbc);
    const char* jsonType() const;
    const char* currentTimestamp() const;
    const char* timestamp0() const;
    const char* autoIncrement() const;
    void encodeStringConst(const char* str, string* newstr) const;
    const char* dbType() const;
    void redshiftKeys(const char* distkey, const char* sortkey,
            string* sql) const;
private:
    void setType(const string& dbtype);
    DBT dbt;
};

#endif
