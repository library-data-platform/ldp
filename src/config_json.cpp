#include "../etymoncpp/include/util.h"
#include "config_json.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/pointer.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/reader.h"
#include "rapidjson/stringbuffer.h"

constexpr json::ParseFlag pflags = json::kParseTrailingCommasFlag;

Config::Config(const string& config)
{
    string configFile;
    if (config != "") {
        configFile = config;
    } else {
        char* s = getenv("LDPCONFIG");
        configFile = s ? s : "";
        etymon::trim(&configFile);
        if (configFile.empty())
            throw runtime_error("configuration file not specified");
    }
    // Load and parse JSON file.
    etymon::File f(configFile, "r");
    char readBuffer[65536];
    json::FileReadStream is(f.file, readBuffer, sizeof(readBuffer));
    jsondoc.ParseStream<pflags>(is);
}

bool Config::get(const string& key, string* value) const
{
    if (const json::Value* v = json::Pointer(key.c_str()).Get(jsondoc)) {
        *value = v->GetString();
        return true;
    } else {
        return false;
    }
}

void Config::getRequired(const string& key, string* value) const
{
    if (!get(key, value)) {
        string err = "Configuration value not found at \"";
        err += key;
        err += '\"';
        throw runtime_error(err);
    }
}

void Config::getOptional(const string& key, string* value) const
{
    if (!get(key, value)) {
        value->clear();
    }
}

