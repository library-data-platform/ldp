#ifndef LDP_CONFIG_H
#define LDP_CONFIG_H

#include "rapidjson/document.h"

using namespace std;
namespace json = rapidjson;

class Config {
public:
    Config(const string& config);
    bool get(const string& key, string* value) const;
    void getRequired(const string& key, string* value) const;
    void getOptional(const string& key, string* value) const;
private:
    json::Document jsondoc;
};

#endif

