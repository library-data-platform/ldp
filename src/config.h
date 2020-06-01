#ifndef LDP_CONFIG_H
#define LDP_CONFIG_H

#include "rapidjson/document.h"

using namespace std;
namespace json = rapidjson;

class config {
public:
    config(const string& conf);
    bool get(const string& key, string* value) const;
    bool get_int(const string& key, int* value) const;
    bool get_bool(const string& key, bool* value) const;
    void get_required(const string& key, string* value) const;
    void get_optional(const string& key, string* value) const;
private:
    json::Document jsondoc;
};

#endif

