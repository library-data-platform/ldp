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
    DBType(etymon::OdbcDbc* dbc);
    const char* currentTimestamp() const;
    void renameSequence(const string& sequenceName,
        const string& newSequenceName, string* sql) const;
    void createSequence(const string& sequenceName, int64_t start,
        string* sql) const;
    void autoIncrementType(int64_t start, bool namedSequence,
            const string& sequenceName, string* sql) const;
    void alterSequenceOwnedBy(const string& sequenceName,
        const string& tableColumnName, string* sql) const;
    void encodeStringConst(const char* str, string* newstr) const;
    const char* dbType() const;
    void redshiftKeys(const char* distkey, const char* sortkey,
            string* sql) const;
private:
    void setType(const string& dbtype);
    DBT dbt;
};

#endif
