#include "../etymoncpp/include/util.h"
#include "config.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/pointer.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/reader.h"
#include "rapidjson/stringbuffer.h"

constexpr json::ParseFlag pflags = json::kParseTrailingCommasFlag;

config::config(const string& conf)
{
    string config_file;
    if (conf != "") {
        config_file = conf;
    } else {
        char* s = getenv("LDPCONFIG");
        config_file = s ? s : "";
        etymon::trim(&config_file);
        if (config_file.empty())
            throw runtime_error("configuration file not specified");
    }
    // Load and parse JSON file.
    etymon::file f(config_file, "r");
    char read_buffer[65536];
    json::FileReadStream is(f.fp, read_buffer, sizeof(read_buffer));
    jsondoc.ParseStream<pflags>(is);
}

bool config::get(const string& key, string* value) const
{
    if (const json::Value* v = json::Pointer(key.c_str()).Get(jsondoc)) {
        if (v->IsString() == false)
            throw runtime_error(
                    "Unexpected data type in configuration setting:\n"
                    "    Key: " + key + "\n"
                    "    Expected type: string");
        *value = v->GetString();
        return true;
    } else {
        return false;
    }
}

bool config::get_bool(const string& key, bool* value) const
{
    if (const json::Value* v = json::Pointer(key.c_str()).Get(jsondoc)) {
        if (v->IsBool() == false)
            throw runtime_error(
                    "Unexpected data type in configuration setting:\n"
                    "    Key: " + key + "\n"
                    "    Expected type: Boolean");
        *value = v->GetBool();
        return true;
    } else {
        return false;
    }
}

bool config::get_int(const string& key, int* value) const
{
    if (const json::Value* v = json::Pointer(key.c_str()).Get(jsondoc)) {
        if (v->IsInt() == false)
            throw runtime_error(
                    "Unexpected data type in configuration setting:\n"
                    "    Key: " + key + "\n"
                    "    Expected type: integer");
        *value = v->GetInt();
        return true;
    } else {
        return false;
    }
}

void config::get_required(const string& key, string* value) const
{
    if (!get(key, value))
        throw runtime_error("configuration value not found: " + key);
}

void config::get_optional(const string& key, string* value) const
{
    if (!get(key, value)) {
        value->clear();
    }
}

