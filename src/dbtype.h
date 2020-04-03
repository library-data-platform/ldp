#ifndef LDP_DBTYPE_H
#define LDP_DBTYPE_H

#include <string>

using namespace std;

enum class DBT {
    postgresql,
    redshift,
    unknown
};

class DBType {
public:
    DBType();
    void setType(const string& dbtype);
    const char* jsonType() const;
    void encodeStringConst(const char* str, string* newstr) const;
    const char* dbType() const;
    void redshiftKeys(const char* distkey, const char* sortkey,
            string* sql) const;
private:
    DBT dbt;
};

#endif
