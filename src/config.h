#ifndef LDP_CONFIG_H
#define LDP_CONFIG_H

#include "options.h"
#include "rapidjson/document.h"

using namespace std;
namespace json = rapidjson;

class ldp_config {
public:
    ldp_config(const string& conf);
    void get_enable_sources(vector<data_source>* enable_sources) const;
    bool get_string(const string& key, bool required, string* value) const;
    bool get_int(const string& key, bool required, int* value) const;
    ///////////////////////////////////////////////////////////////////////////
    bool get(const string& key, string* value) const;
    bool old_get_int(const string& key, int* value) const;
    bool get_bool(const string& key, bool* value) const;
    void get_required(const string& key, string* value) const;
    void get_optional(const string& key, string* value) const;
private:
    const json::Value* get_json_pointer(const string& key) const;
    json::Document jsondoc;
};

void throw_value_out_of_range(const string& key,
                              const string& value,
                              const string& range);

#endif

