#include "../etymoncpp/include/mallocptr.h"
#include "../etymoncpp/include/util.h"
#include "config.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/pointer.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/reader.h"
#include "rapidjson/stringbuffer.h"

constexpr json::ParseFlag pflags = json::kParseTrailingCommasFlag;

ldp_config::ldp_config(const string& conf)
{
    string config_file;
    if (conf != "") {
        config_file = conf;
    } else {
        char* s = getenv("LDPCONFIG");
        config_file = s ? s : "";
        etymon::trim(&config_file);
        if (config_file.empty())
            throw runtime_error("Configuration file not specified");
    }
    // Load and parse JSON file.
    etymon::file f(config_file, "r");
    char* read_buffer = (char*) malloc(67108864);
    etymon::malloc_ptr read_buffer_ptr(read_buffer);
    json::FileReadStream is(f.fp, read_buffer, sizeof(read_buffer));
    jsondoc.ParseStream<pflags>(is);
}

static void throw_invalid_data_type(const string& key,
                                    const string& expected_type)
{
    throw runtime_error(
            "Invalid data type in configuration setting:\n"
            "    Key: " + key + "\n"
            "    Expected type: " + expected_type);
}

void throw_value_out_of_range(const string& key,
                              const string& value,
                              const string& range)
{
    throw runtime_error(
            "Value for configuration setting is out of range:\n"
            "    Key: " + key + "\n"
            "    Value: " + value + "\n"
            "    Range: " + range);
}

static void throw_required_value_not_found(const string& key)
{
    throw runtime_error(
            "Required configuration value not found:\n"
            "    Key: " + key);
}

const json::Value* ldp_config::get_json_pointer(const string& key) const
{
        return json::Pointer(key.c_str()).Get(jsondoc);
}

bool ldp_config::get_string(const string& key, bool required,
                            string* value) const
{
    const json::Value* v = get_json_pointer(key);
    if (v == nullptr) {
        if (required)
            throw_required_value_not_found(key);
        else
            return false;
    }
    if (v->IsString() == false)
        throw_invalid_data_type(key, "string");
    *value = v->GetString();
    return true;
}

bool ldp_config::get_int(const string& key, bool required,
                         int* value) const
{
    const json::Value* v = get_json_pointer(key);
    if (v == nullptr) {
        if (required)
            throw_required_value_not_found(key);
        else
            return false;
    }
    if (v->IsInt() == false)
        throw_invalid_data_type(key, "integer");
    *value = v->GetInt();
    return true;
}

void ldp_config::get_enable_sources(vector<data_source>* enable_sources) const
{
    enable_sources->clear();
    // Loop through enable_sources JSON array.
    int source_index = 0;
    while (true) {
        string key = "/enable_sources/" + to_string(source_index);
        const json::Value* value = get_json_pointer(key);
        if (value == nullptr)
            break;
        if (value->IsString() == false)
            throw_invalid_data_type(key, "string");
        string source_name = value->GetString();

        // TODO Check if source_name is already in enable_sources, and if so,
        // flag the duplication (throw error).

        // Look up source details.
        data_source source;
        source.source_name = source_name;
        string prefix = "/sources/" + source_name + "/";
        // Okapi URL.
        get_string(prefix + "okapi_url", true, &(source.okapi_url));
        // Okapi tenant.
        get_string(prefix + "okapi_tenant", true, &(source.okapi_tenant));
        // Okapi user.
        get_string(prefix + "okapi_user", true, &(source.okapi_user));
        // Okapi password.
        get_string(prefix + "okapi_password", true, &(source.okapi_password));

        // Direct extraction.
        direct_extraction direct;
        // Loop through direct_tables JSON array.
        int direct_index = 0;
        while (true) {
            string key = prefix + "direct_tables/" + to_string(direct_index);
            const json::Value* value = get_json_pointer(key);
            if (value == nullptr)
                break;
            if (value->IsString() == false)
                throw_invalid_data_type(key, "string");
            string table_name = value->GetString();

            if (table_name != "inventory_holdings" &&
                table_name != "inventory_instances" &&
                table_name != "inventory_items" &&
                table_name != "po_receiving_history" &&
                table_name != "perm_permissions" &&
                table_name != "perm_users" &&
                table_name != "srs_marc" &&
                table_name != "srs_error" &&
                table_name != "srs_records") {
                throw runtime_error("Direct extraction not supported for table: " + table_name);
            }
            direct.table_names.push_back(table_name);
            direct_index++;
        }
        // Additional direct extraction settings.
        // Direct database name.
        get_string(prefix + "direct_database_name", false,
                   &(direct.database_name));
        // Direct database host.
        get_string(prefix + "direct_database_host", false,
                   &(direct.database_host));
        // Port number.
        int port = 0;
        bool found = get_int(prefix + "direct_database_port", false, &port);
        if (found) {
            if (1 <= port && port <= 65535) {
                direct.database_port = port;
            } else {
                throw_value_out_of_range(prefix + "direct_database_port",
                                         to_string(port), "1 to 65535");
            }
        } else {
            direct.database_port = 5432;
        }
        // Direct database user.
        get_string(prefix + "direct_database_user", false,
                   &(direct.database_user));
        // Direct database password.
        get_string(prefix + "direct_database_password", false,
                   &(direct.database_password));
        // Add direct extractino data to source.
        source.direct = direct;
        // Append source to list.
        enable_sources->push_back(source);
        source_index++;
    }; // while
}

///////////////////////////////////////////////////////////////////////////////

bool ldp_config::get(const string& key, string* value) const
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

bool ldp_config::get_bool(const string& key, bool* value) const
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

bool ldp_config::old_get_int(const string& key, int* value) const
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

void ldp_config::get_required(const string& key, string* value) const
{
    if (!get(key, value))
        throw runtime_error("Configuration value not found: " + key);
}

void ldp_config::get_optional(const string& key, string* value) const
{
    if (!get(key, value))
        value->clear();
}

