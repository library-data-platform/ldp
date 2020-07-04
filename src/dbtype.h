#ifndef LDP_DBTYPE_H
#define LDP_DBTYPE_H

#include "../etymoncpp/include/odbc.h"

enum class dbsys {
    postgresql,
    redshift,
    unknown
};

class dbtype {
public:
    dbtype(etymon::odbc_conn* conn);
    const char* json_type() const;
    const char* current_timestamp() const;
    void rename_sequence(const string& sequence_name,
        const string& new_sequence_name, string* sql) const;
    void create_sequence(const string& sequence_name, int64_t start,
        string* sql) const;
    void auto_increment_type(int64_t start, bool named_sequence,
            const string& sequence_name, string* sql) const;
    void alter_sequence_owned_by(const string& sequence_name,
        const string& table_column_name, string* sql) const;
    void encode_string_const(const char* str, string* newstr) const;
    const char* type_string() const;
    dbsys type() const;
    void redshift_keys(const char* distkey, const char* sortkey,
            string* sql) const;
private:
    void set_type(const string& dbms);
    dbsys dbt;
};

#endif
