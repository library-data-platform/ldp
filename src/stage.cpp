#define _LIBCPP_NO_EXPERIMENTAL_DEPRECATION_WARNING_FILESYSTEM

#include <algorithm>
#include <cstdio>
#include <experimental/filesystem>
#include <map>
#include <memory>
#include <regex>

#include "../etymoncpp/include/mallocptr.h"
#include "../etymoncpp/include/postgres.h"
#include "../etymoncpp/include/util.h"
#include "camelcase.h"
#include "dbtype.h"
#include "names.h"
#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/pointer.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/reader.h"
#include "rapidjson/stringbuffer.h"
#include "schema.h"
#include "stage.h"

namespace fs = std::experimental::filesystem;
namespace json = rapidjson;

const int copy_buffer_size = 125000000;

constexpr json::ParseFlag pflags = json::kParseTrailingCommasFlag;

static void expand_column_name(const string& name, string* expanded)
{
    string s = name;
    size_t p = s.find("/");
    if (p != string::npos) {
        s.replace(p, 1, "__");
    }
    *expanded = s;
}

struct name_comparator {
    bool operator()(const json::Value::Member &lhs,
            const json::Value::Member &rhs) const {
        const char* s1 = lhs.name.GetString();
        const char* s2 = rhs.name.GetString();
        if (strcmp(s1, "id") == 0)
            return true;
        if (strcmp(s2, "id") == 0)
            return false;
        return (strcmp(s1, s2) < 0);
    }
};

bool looks_like_date_time(const char* str)
{
    static regex date_time("^\\d{4}-\\d{2}-\\d{2}[T ]\\d{2}:\\d{2}:\\d{2}");
    return regex_search(str, date_time);
}

bool ends_with(string const &str, string const &suffix) {
    if (str.length() >= suffix.length())
	return (0 == str.compare(str.length() - suffix.length(),
				 suffix.length(), suffix));
    else
	return false;
}

bool data_to_filter(const table_schema& table, const string& field)
{
    if (table.name != "course_copyrightstatuses" &&
        table.name != "course_courselistings" &&
        table.name != "course_courses" &&
        table.name != "course_coursetypes" &&
        table.name != "course_departments" &&
        table.name != "course_processingstatuses" &&
        table.name != "course_reserves" &&
        table.name != "course_roles" &&
        table.name != "course_terms")
        return false;
    if (!ends_with(field, "Object") && !ends_with(field, "Objects"))
        return false;
    if (table.name == "course_courselistings" && strncmp(field.data(), "/instructorObjects", 18) == 0)
        return false;
    return true;
}

// Collect statistics and anonymize data
void process_json_record(const table_schema& table,
                         json::Document* root,
                         json::Value* node,
                         bool collect_stats,
                         field_set* drop_fields,
                         const string& field,
                         unsigned int depth,
                         map<string,type_counts>* stats,
                         bool obj)
{
    switch (node->GetType()) {
        case json::kNullType:
            if (collect_stats && (depth == 1 || (depth == 2 && obj)))
                (*stats)[field.c_str() + 1].null++;
            break;
        case json::kTrueType:
        case json::kFalseType:
            if (drop_fields->find(table.name, field))
                json::Pointer(field.c_str()).Set(*root, false);
            if (collect_stats && (depth == 1 || (depth == 2 && obj)))
                (*stats)[field.c_str() + 1].boolean++;
            break;
        case json::kNumberType:
            if (drop_fields->find(table.name, field))
                json::Pointer(field.c_str()).Set(*root, 0);
            if (collect_stats && (depth == 1 || (depth == 2 && obj))) {
                (*stats)[field.c_str() + 1].number++;
                if (node->IsInt() || node->IsUint() || node->IsInt64() ||
                        node->IsUint64())
                    (*stats)[field.c_str() + 1].integer++;
                else
                    (*stats)[field.c_str() + 1].floating++;
            }
            break;
        case json::kStringType:
            if (drop_fields->find(table.name, field))
                json::Pointer(field.c_str()).Set(*root, "");
            if (collect_stats && (depth == 1 || (depth == 2 && obj))) {
                (*stats)[field.c_str() + 1].string++;
                if (is_uuid(node->GetString()))
                    (*stats)[field.c_str() + 1].uuid++;
                if (looks_like_date_time(node->GetString()))
                    (*stats)[field.c_str() + 1].date_time++;
                size_t slen = strlen(node->GetString());
                if (slen > (*stats)[field.c_str() + 1].max_length)
                    (*stats)[field.c_str() + 1].max_length = slen;
            }
            break;
        case json::kArrayType:
            if (drop_fields->find(table.name, field)) {
		json::Pointer(field.c_str()).Set(*root, json::kNullType);
		break;
            }
            if (data_to_filter(table, field)) {
		json::Pointer(field.c_str()).Set(*root, json::kNullType);
		break;
            }
            {
                int x = 0;
                for (json::Value::ValueIterator i = node->Begin();
                        i != node->End(); ++i) {
                    string new_field = field;
                    new_field += '/';
                    new_field += to_string(x);
                    process_json_record(table, root, i, collect_stats, drop_fields, new_field, depth + 1, stats, false);
                    x++;
                }
            }
            break;
        case json::kObjectType:
            if (drop_fields->find(table.name, field)) {
		json::Pointer(field.c_str()).Set(*root, json::kNullType);
		break;
            }
            if (data_to_filter(table, field)) {
		json::Pointer(field.c_str()).Set(*root, json::kNullType);
		break;
            }
            sort(node->MemberBegin(), node->MemberEnd(), name_comparator());
            for (json::Value::MemberIterator i = node->MemberBegin();
                    i != node->MemberEnd(); ++i) {
                string new_field = field;
                new_field += '/';
                new_field += i->name.GetString();
                process_json_record(table, root, &(i->value), collect_stats, drop_fields, new_field, depth + 1, stats, true);
            }
            break;
        default:
            break;
    }
}

/* *
  * \brief  Main ETL processor for JSON data.
  *
  * This class handles most of the ETL processing for a FOLIO interface.
  * The large JSON files that have been retrieved from Okapi are
  * streamed in and parsed into individual JSON object records, in order
  * that only a single record needs to be held in memory at a time.
  * Several functions are performed during two passes over the data.  In
  * pass 1:  Statistics are collected on the data types, and a table
  * schema is generated based on the results.  In pass 2:  (i) Some data
  * are removed or altered as part of anonymization of personal data.
  * (ii) Each JSON object is normalized to enable later comparison with
  * historical data.  (iii) SQL insert statements are generated and
  * submitted to the database to stage the data for merging.
  */
class JSONHandler : public json::BaseReaderHandler<json::UTF8<>, JSONHandler> {
public:
    int pass;
    const ldp_options& opt;
    ldp_log* lg;
    int level = 0;
    bool active = false;
    string record;
    const table_schema& table;
    // Collection of statistics
    map<string,type_counts>* stats;
    // Loading to database
    etymon::pgconn* conn;
    const dbtype& dbt;
    field_set* drop_fields = nullptr;
    size_t record_count = 0;
    size_t total_record_count = 0;
    string* copy_buffer;
    JSONHandler(int pass,
                const ldp_options& options,
                ldp_log* lg,
                const table_schema& table,
                etymon::pgconn* conn,
                const dbtype& dbt,
                field_set* drop_fields,
                map<string,type_counts>* statistics,
                string* copy_buffer) :
        pass(pass),
        opt(options),
        lg(lg),
        table(table),
        stats(statistics),
        conn(conn),
        dbt(dbt),
        drop_fields(drop_fields),
        copy_buffer(copy_buffer) {}
    bool Default();
    bool StartObject();
    bool EndObject(json::SizeType memberCount);
    bool StartArray();
    bool EndArray(json::SizeType elementCount);
    bool Key(const char* str, json::SizeType length, bool copy);
    bool String(const char* str, json::SizeType length, bool copy);
    bool Int(int i);
    bool Uint(unsigned u);
    bool Bool(bool b);
    bool Null();
    bool Int64(int64_t i);
    bool Uint64(uint64_t u);
    bool Double(double d);
};

bool JSONHandler::Default()
{
    lg->warning("json parser: default method invoked");
    return true;
}

bool JSONHandler::StartObject()
{
    if (level == 2) {
        record = '{';
    } else {
        if (level > 2)
            record += '{';
    }
    level++;
    return true;
}

static void begin_copy_batch()
{
    // NOP
}

static void end_copy_batch(const ldp_options& opt, ldp_log* lg,
                        const string& table, string* buffer,
                        etymon::pgconn* conn)
{
    int r = PQputCopyData(conn->conn, buffer->data(), buffer->length());
    if (r == -1) {
        throw runtime_error(PQerrorMessage(conn->conn));
    }
    if (r != 1) {
        lg->warning("copy data result code: " + to_string(r));
    }
    buffer->clear();
}

static void writeTuple(const ldp_options& opt, ldp_log* lg, const dbtype& dbt,
        const table_schema& table, const json::Document& doc,
        size_t* record_count, size_t* total_record_count, string* copy_buffer)
{
    //if (*record_count > 0)
    //    *insert_buffer += ',';
    //*insert_buffer += '(';

    const char* id = nullptr;
    if (doc.HasMember("id") && doc["id"].IsString()) {
        id = doc["id"].GetString();
    }
    if (id == nullptr && doc.HasMember("notificationId") && doc["notificationId"].IsString()) {
        id = doc["notificationId"].GetString();
    }
    if (id == nullptr)
        throw runtime_error("required string field \"id\" not found in record");

    // id
    string idenc;
    dbt.encode_copy(id, &idenc);
    *copy_buffer += idenc;
    *copy_buffer += '\t';

    string s;
    double d;
    for (const auto& column : table.columns) {
        if (column.name == "id")
            continue;
        const char* sourceColumnName = column.source_name.c_str();
        const json::Value* val;
        size_t p = column.source_name.find("/");
        if (p != string::npos) {
            string path = string("/") + sourceColumnName;
            val = json::Pointer(path.data()).Get(doc);
            if (val == nullptr) {
                *copy_buffer += "\\N\t";
                continue;
            }
        } else {
            if (doc.HasMember(sourceColumnName) == false) {
                *copy_buffer += "\\N\t";
                continue;
            }
            val = &(doc[sourceColumnName]);
        }
        const json::Value& jsonValue = *val;
        if (jsonValue.IsNull()) {
            *copy_buffer += "\\N\t";
            continue;
        }
        switch (column.type) {
        case column_type::bigint:
            *copy_buffer += to_string(jsonValue.GetInt());
            break;
        case column_type::boolean:
            *copy_buffer += ( jsonValue.GetBool() ?  "1" : "0" );
            break;
        case column_type::numeric:
            d = jsonValue.GetDouble();
            s = to_string(d);
            if (d > 10000000000.0) {
                lg->write(log_level::warning, "", "",
                          "Numeric value exceeds 10^10:\n"
                          "    Table: " + table.name + "\n"
                          "    Column: " + column.name + "\n"
                          "    ID: " + id + "\n"
                          "    Value: " + to_string(d) + "\n"
                          "    Action: Value set to 0", -1);
                s = "0";
            }
            *copy_buffer += s;
            break;
        case column_type::id:
        case column_type::timestamptz:
        case column_type::varchar:
            dbt.encode_copy(jsonValue.GetString(), &s);
            // Check if varchar exceeds maximum string length.
            if (s.length() >= varchar_size - 1) {
                lg->write(log_level::warning, "", "",
                        "String length exceeds database limit:\n"
                        "    Table: " + table.name + "\n"
                        "    Column: " + column.name + "\n"
                        "    ID: " + id + "\n"
                        "    Action: Value set to NULL", -1);
                s = "\\N";
            }
            *copy_buffer += s;
            break;
        }
        *copy_buffer += '\t';
    }

    string data;
    json::StringBuffer json_text;
    json::PrettyWriter<json::StringBuffer> writer(json_text);
    doc.Accept(writer);
    dbt.encode_copy(json_text.GetString(), &data);
    // Check if pretty-printed JSON exceeds maximum string length.
    if (data.length() > varchar_size - 1) {
        // Formatted JSON object size exceeds database limit.  Try
        // compact-printed JSON.
        json::StringBuffer json_text;
        json::Writer<json::StringBuffer> writer(json_text);
        doc.Accept(writer);
        dbt.encode_copy(json_text.GetString(), &data);
        if (data.length() > varchar_size - 1) {
            lg->write(log_level::warning, "", "",
                    "JSON object size exceeds database limit:\n"
                    "    Table: " + table.name + "\n"
                    "    ID: " + id + "\n"
                    "    Action: Value for column \"data\" set to NULL", -1);
            data = "\\N";
        }
    }

    //print(Print::warning, opt, "storing record as:\n" + data + "\n");

    *copy_buffer += data;
    *copy_buffer += "\n";
    (*record_count)++;
    (*total_record_count)++;
    //if (*total_record_count % 100000 == 0)
    //    fprintf(stderr, "%zu\n", *total_record_count);
}

bool JSONHandler::EndObject(json::SizeType memberCount)
{
    if (level == 3) {

        record += '}';

        if (pass == 2) {
            string brief = record;
            if (brief.length() > 80) {
                brief = brief.substr(0, 80) + "...";
            }
            switch (opt.lg_level) {
            case log_level::trace:
                lg->trace(table.name + ": " + brief);
                break;
            case log_level::detail:
                lg->detail(table.name + ": " + record);
                break;
            default:
                break;
            }
        }

        char* buffer = strdup(record.c_str());
        etymon::malloc_ptr bufferptr(buffer);

        json::Document doc;
        doc.ParseInsitu<pflags>(buffer);

        bool collect_stats = (pass == 1);
        string path;
        // Collect statistics and anonymize data.
        process_json_record(table, &doc, &doc, collect_stats, drop_fields, path, 0, stats, true);

        if (pass == 2) {

            if (copy_buffer->length() > (copy_buffer_size - 2000000)) {
                end_copy_batch(opt, lg, table.name, copy_buffer, conn);
                lg->trace(table.name + ": staged group: " + to_string(record_count) + " records");
                begin_copy_batch();
                record_count = 0;
            }

            writeTuple(opt, lg, dbt, table, doc, &record_count, &total_record_count, copy_buffer);
        }

    } else {
        if (level > 3)
            record += "},";
    }
    level--;
    return true;
}

bool JSONHandler::StartArray()
{
    if (level == 1) {
        active = true;
        if (pass == 2)
            begin_copy_batch();
    } else {
        if (level > 1)
            record += '[';
    }
    level++;
    return true;
}

bool JSONHandler::EndArray(json::SizeType elementCount)
{
    if (level == 2) {
        active = false;
        if (record_count > 0)
            if (pass == 2) {
                end_copy_batch(opt, lg, table.name, copy_buffer, conn);
                lg->trace(table.name + ": staged group: " + to_string(record_count) + " records");
                lg->trace(table.name + ": end of staging");
            }
    } else {
        if (level > 2)
            record += "],";
    }
    level--;
    return true;
}

void encode_json(const char* str, string* newstr)
{
    char buffer[8];
    const char *p = str;
    char c;
    while ( (c=*p) != '\0') {
        switch (c) {
            case '"':
                *newstr += "\\\"";
                break;
            case '\\':
                (*newstr) += "\\\\";
                break;
            case '\b':
                *newstr += "\\b";
                break;
            case '\f':
                *newstr += "\\f";
                break;
            case '\n':
                *newstr += "\\n";
                break;
            case '\r':
                *newstr += "\\r";
                break;
            case '\t':
                *newstr += "\\t";
                break;
            default:
                if ( 0 <= ((int) c) && ((int) c) <= 31 ) {
                    sprintf(buffer, "\\u%04X", (unsigned char) c);
                    *newstr += buffer;
                } else {
                    *newstr += c;
                }
        }
        p++;
    }
}

bool JSONHandler::Key(const char* str, json::SizeType length, bool copy)
{
    record += '\"';
    string enc_str;
    encode_json(str, &enc_str);
    record += enc_str;
    record += "\":";
    return true;
}

bool JSONHandler::String(const char* str, json::SizeType length, bool copy)
{
    if (active && (level > 2) ) {
        record += '\"';
        string enc_str;
        encode_json(str, &enc_str);
        record += enc_str;
        record += "\",";
    }
    return true;
}

bool JSONHandler::Int(int i)
{
    if ( active && (level > 2) ) {
        record += to_string(i);
        record += ',';
    }
    return true;
}

bool JSONHandler::Uint(unsigned u)
{
    if ( active && (level > 2) ) {
        record += to_string(u);
        record += ',';
    }
    return true;
}

bool JSONHandler::Int64(int64_t i)
{
    if ( active && (level > 2) ) {
        record += to_string(i);
        record += ',';
    }
    return true;
}

bool JSONHandler::Uint64(uint64_t u)
{
    if ( active && (level > 2) ) {
        record += to_string(u);
        record += ',';
    }
    return true;
}

bool JSONHandler::Double(double d)
{
    if ( active && (level > 2) ) {
        record += to_string(d);
        record += ',';
    }
    return true;
}

bool JSONHandler::Bool(bool b)
{
    if ( active && (level > 2) )
        record += b ? "true," : "false,";
    return true;

}

bool JSONHandler::Null()
{
    if ( active && (level > 2) )
        record += "null,";
    return true;
}

size_t read_page_count(const data_source& source, ldp_log* lg,
                       const string& load_dir, const string& table_name)
{
    string filename = load_dir;
    etymon::join(&filename, table_name);
    if (source.source_name != "")
        filename += "_" + source.source_name;
    filename += "_count.txt";
    if ( !(fs::exists(filename)) ) {
        lg->write(log_level::warning, "", "", "File not found: " + filename, -1);
        return 0;
    }
    etymon::file f(filename, "r");
    size_t count;
    int r = fscanf(f.fp, "%zu", &count);
    if (r < 1 || r == EOF)
        throw runtime_error("unable to read page count from " + filename);
    return count;
}

static void stage_page(const ldp_options& opt, ldp_log* lg, int pass,
                       const table_schema& table,
                       etymon::pgconn* conn, const dbtype &dbt,
                       map<string,type_counts>* stats, const string& filename,
                       char* read_buffer, size_t read_buffer_size,
                       field_set* drop_fields)
{
    json::Reader reader;
    etymon::file f(filename, "r");
    json::FileReadStream is(f.fp, read_buffer, read_buffer_size);

    if (pass == 2) {
        string loading_table;
        loading_table_name(table.name, &loading_table);
        string sql = "COPY " + loading_table + " FROM STDIN;";
        { etymon::pgconn_result r(conn, sql); }
    }

    {
        string copy_buffer;
        copy_buffer.reserve(copy_buffer_size);
        JSONHandler handler(pass, opt, lg, table, conn, dbt, drop_fields, stats, &copy_buffer);
        reader.Parse(is, handler);
    }

    if (pass == 2) {
        int r = PQputCopyEnd(conn->conn, nullptr);
        if (r == -1) {
            throw runtime_error(PQerrorMessage(conn->conn));
        }
        if (r != 1) {
            lg->warning("copy end result code: " + to_string(r));
        }
        PGresult* res = PQgetResult(conn->conn);
        if (res == nullptr || PQresultStatus(res) == PGRES_FATAL_ERROR) {
            string err = PQresultErrorMessage(res);
            if (res != nullptr) {
                PQclear(res);
            }
            throw runtime_error(err);
        }
        PQclear(res);
    }
}

static void compose_data_file_path(const string& load_dir,
                                   const table_schema& table,
                                   const string& source_name,
                                   const string& suffix, string* path)
{
    *path = load_dir;
    etymon::join(path, table.name);
    if (source_name != "")
        *path += ("_" + source_name);
    *path += suffix;
}

void index_loaded_table(ldp_log* lg, const table_schema& table, etymon::pgconn* conn, dbtype* dbt, bool index_large_varchar)
{
    lg->detail("creating indexes: " + table.name);
    // If there is no table schema, define a primary key on (id) and return.
    if (table.columns.size() == 0) {
        string sql =
            "ALTER TABLE " + table.name + "\n"
            "    ADD PRIMARY KEY (id);";
        lg->detail(sql);
        try {
            { etymon::pgconn_result r(conn, sql); }
        } catch (runtime_error& e) {
            lg->write(log_level::warning, "server", "", e.what(), -1);
        }
        return;
    }
    // If there is a table schema, define the primary key or indexes.
    for (const auto& column : table.columns) {
        if (column.name == "id") {
            string sql =
                "ALTER TABLE " + table.name + "\n"
                "    ADD PRIMARY KEY (id);";
            lg->detail(sql);
            try {
                { etymon::pgconn_result r(conn, sql); }
            } catch (runtime_error& e) {
                lg->write(log_level::warning, "server", "", e.what(), -1);
            }
        } else {
            // in postgres, index columns except column "data"
            if (dbt->type() == dbsys::postgresql && column.name != "data") {
                string colname;
                expand_column_name(column.name, &colname);
                // create btree index unless column is a large varchar
                if (column.type == column_type::varchar && column.length > 500) {
                    // create hash index if index_large_varchar is enabled
                    if (index_large_varchar) {
                        string sql = "CREATE INDEX ON " + table.name + " USING HASH (\"" + colname + "\");";
                        lg->detail(sql);
                        try {
                            { etymon::pgconn_result r(conn, sql); }
                        } catch (runtime_error& e) {
                            lg->write(log_level::warning, "server", "", "Unable to create hash index: table=" + table.name + " column=" + colname, -1);
                        }
                    }
                } else {
                    string sql = "CREATE INDEX ON " + table.name + " (\"" + colname + "\");";
                    lg->detail(sql);
                    try {
                        { etymon::pgconn_result r(conn, sql); }
                    } catch (runtime_error& e) {
                        lg->write(log_level::warning, "server", "", "Unable to create B-tree index: table=" + table.name + " column=" + colname, -1);
                    }
                }
            }
        }
    }
}

static void create_loading_table(const ldp_options& opt, ldp_log* lg,
                                 const table_schema& table,
                                 etymon::pgconn* conn, const dbtype& dbt, vector<string>* users)
{
    string loading_table;
    loading_table_name(table.name, &loading_table);

    string sql = "DROP TABLE IF EXISTS " + loading_table + ";";
    lg->detail(sql);
    { etymon::pgconn_result r(conn, sql); }

    string rskeys;
    dbt.redshift_keys("id", "id", &rskeys);
    sql = "CREATE TABLE ";
    sql += loading_table;
    sql += " (\n"
        "    id VARCHAR(36) NOT NULL,\n";
    string column_type;
    for (const auto& column : table.columns) {
        if (column.name != "id") {
            string colname;
            expand_column_name(column.name, &colname);
            sql += "    \"";
            sql += colname;
            sql += "\" ";
            if (column.type == column_type::varchar)
                column_type = "VARCHAR(" + to_string(column.length) + ")";
            else
                column_schema::type_to_string(column.type, &column_type);
            sql += column_type;
            sql += ",\n";
        }
    }
    sql += string("    data ") + dbt.json_type() + "\n"
        ")" + rskeys + ";";
    lg->write(log_level::detail, "", "", sql, -1);
    { etymon::pgconn_result r(conn, sql); }

    // // Add comment on table.
    // comment_sql(loading_table, table.module_name, &sql);
    // lg->write(log_level::detail, "", "", "Setting comment on table: " + table.name, -1);
    // { etymon::pgconn_result r(conn, sql); }

    sql = "GRANT SELECT ON " + loading_table + " TO " + opt.ldpconfig_user + ";";
    lg->detail(sql);
    { etymon::pgconn_result r(conn, sql); }
    sql = "GRANT SELECT ON " + loading_table + " TO " + opt.ldp_user + ";";
    lg->detail(sql);
    { etymon::pgconn_result r(conn, sql); }
    for (auto& u : *users) {
        sql = "GRANT SELECT ON " + loading_table + " TO " + u + ";";
        lg->detail(sql);
        { etymon::pgconn_result r(conn, sql); }
        sql = "GRANT SELECT ON " + loading_table + " TO " + u + ";";
        lg->detail(sql);
        { etymon::pgconn_result r(conn, sql); }
    }
}

bool stage_table_1(const ldp_options& opt,
                   const vector<source_state>& source_states,
                   ldp_log* lg,
                   table_schema* table,
                   etymon::pgconn* conn,
                   dbtype* dbt,
                   const string& load_dir,
                   field_set* drop_fields,
                   char* read_buffer,
                   vector<string>* users)
{
    map<string,type_counts> stats;

    for (auto& state : source_states) {
        size_t page_count = read_page_count(state.source, lg, load_dir,
                                            table->name);

        lg->write(log_level::detail, "", "",
                  "staging: " + table->name + ": page count: " +
                  to_string(page_count), -1);

        for (size_t page = 0; page < page_count; page++) {
            string path;
            compose_data_file_path(load_dir, *table, state.source.source_name,
                                   "_" + to_string(page) + ".json", &path);
            lg->write(log_level::detail, "", "", "staging: " + table->name + ": analyze: page: " + to_string(page), -1);
            stage_page(opt, lg, 1, *table, conn, *dbt, &stats, path,
                       read_buffer, sizeof read_buffer, drop_fields);
        }
    }

    if (opt.load_from_dir != "") {
        string path;
        compose_data_file_path(load_dir, *table, "", "_test.json", &path);
        if (fs::exists(path)) {
            lg->write(log_level::detail, "", "", "staging: " + table->name + ": analyze: test file", -1);
            stage_page(opt, lg, 1, *table, conn, *dbt, &stats,
                       path, read_buffer, sizeof read_buffer,
                       drop_fields);
        }
    }

    for (const auto& [field, counts] : stats) {
        lg->write(log_level::detail, "", "",
                  "Stats: in field: " + field, -1);
        lg->write(log_level::detail, "", "",
                  "Stats: string: " + to_string(counts.string), -1);
        lg->write(log_level::detail, "", "",
                  "Stats: datetime: " + to_string(counts.date_time), -1);
        lg->write(log_level::detail, "", "",
                  "Stats: bool: " + to_string(counts.boolean), -1);
        lg->write(log_level::detail, "", "",
                  "Stats: number: " + to_string(counts.number), -1);
        lg->write(log_level::detail, "", "",
                  "Stats: int: " + to_string(counts.integer), -1);
        lg->write(log_level::detail, "", "",
                  "Stats: float: " + to_string(counts.floating), -1);
        lg->write(log_level::detail, "", "",
                  "Stats: null: " + to_string(counts.null), -1);
        lg->write(log_level::detail, "", "",
                  "Stats: max_length: " + to_string(counts.max_length),
                  -1);
    }

    for (const auto& [field, counts] : stats) {
        if (table->source_type == data_source_type::srs_marc_records && field != "id") {
            continue;
        }
        if (table->source_type == data_source_type::srs_error_records && field != "id" && field != "description") {
            continue;
        }
        column_schema column;
        bool ok =
            column_schema::select_type(lg, table->name,
                                       table->source_spec, field, counts,
                                       &column.type);
        if (!ok)
            return false;
        string type_str;
        column_schema::type_to_string(column.type, &type_str);
        column.length = max( (unsigned int) 1, counts.max_length);
        string newattr;
        decode_camel_case(field.c_str(), &newattr);
        lg->write(log_level::detail, "", "",
                  string("Column: ") + newattr + string(" ") + type_str,
                  -1);
        column.name = newattr;
        column.source_name = field;
        table->columns.push_back(column);
    }
    create_loading_table(opt, lg, *table, conn, *dbt, users);

    return true;
}

bool stage_table_2(const ldp_options& opt,
                   const vector<source_state>& source_states,
                   ldp_log* lg,
                   table_schema* table,
                   etymon::pgconn* conn,
                   dbtype* dbt,
                   const string& load_dir,
                   field_set* drop_fields,
                   char* read_buffer)
{
    map<string,type_counts> stats;

    for (auto& state : source_states) {
        size_t page_count = read_page_count(state.source, lg, load_dir,
                                            table->name);

        lg->write(log_level::detail, "", "",
                  "staging: " + table->name + ": page count: " +
                  to_string(page_count), -1);

        for (size_t page = 0; page < page_count; page++) {
            string path;
            compose_data_file_path(load_dir, *table, state.source.source_name,
                                   "_" + to_string(page) + ".json", &path);
            lg->write(log_level::detail, "", "", "staging: " + table->name + ": load: page: " + to_string(page), -1);
            stage_page(opt, lg, 2, *table, conn, *dbt, &stats, path,
                       read_buffer, sizeof read_buffer,
                       drop_fields);
        }
    }

    if (opt.load_from_dir != "") {
        string path;
        compose_data_file_path(load_dir, *table, "", "_test.json", &path);
        if (fs::exists(path)) {
            lg->write(log_level::detail, "", "", "staging: " + table->name + ": load: test file", -1);
            stage_page(opt, lg, 2, *table, conn, *dbt, &stats,
                       path, read_buffer, sizeof read_buffer,
                       drop_fields);
        }
    }

    return true;
}
