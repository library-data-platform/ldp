#include <algorithm>
#include <cstdio>
#include <map>
#include <memory>
#include <regex>

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
#include "util.h"
#include "stage_json.h"

//using namespace std;
namespace json = rapidjson;

constexpr json::ParseFlag pflags = json::kParseTrailingCommasFlag;

bool possiblePersonalData(const string& field)
{
    const char* f = field.c_str();
    return (
            (strcmp(f, "/id") != 0) &&
            (strcmp(f, "/active") != 0) &&
            (strcmp(f, "/type") != 0) &&
            (strcmp(f, "/patronGroup") != 0) &&
            (strcmp(f, "/enrollmentDate") != 0) &&
            (strcmp(f, "/expirationDate") != 0) &&
            (strcmp(f, "/meta") != 0) &&
            (strcmp(f, "/proxyFor") != 0) &&
            (strcmp(f, "/createdDate") != 0) &&
            (strcmp(f, "/updatedDate") != 0) &&
            (strcmp(f, "/metadata") != 0) &&
            (strcmp(f, "/tags") != 0)
           );
}

struct NameComparator {
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

bool looksLikeDateTime(const char* str)
{
    static regex dateTime(
            "\\d{4}-\\d{2}-\\d{2}T\\d{2}:\\d{2}:\\d{2}("
            "(\\.\\d{3}\\+\\d{4})|"
            "(Z)"
            ")"
            );
    return regex_match(str, dateTime);
}

void processJSONRecord(json::Document* root, json::Value* node,
        bool anonymizeTable, const string& path, unsigned int depth,
        map<string,Counts>* stats)
{
    switch (node->GetType()) {
        case json::kNullType:
            if (depth == 1) {
                (*stats)[path.c_str() + 1].null++;
            }
            break;
        case json::kTrueType:
        case json::kFalseType:
            if (anonymizeTable && possiblePersonalData(path.c_str())) {
                json::Pointer(path.c_str()).Set(*root, false);
            }
            if (depth == 1) {
                (*stats)[path.c_str() + 1].boolean++;
            }
            break;
        case json::kNumberType:
            if (anonymizeTable && possiblePersonalData(path.c_str())) {
                json::Pointer(path.c_str()).Set(*root, 0);
            }
            if (depth == 1) {
                (*stats)[path.c_str() + 1].number++;
                if (node->IsInt() || node->IsUint() || node->IsInt64() ||
                        node->IsUint64())
                    (*stats)[path.c_str() + 1].integer++;
                else
                    (*stats)[path.c_str() + 1].floating++;
            }
            break;
        case json::kStringType:
            if (anonymizeTable && possiblePersonalData(path.c_str())) {
                json::Pointer(path.c_str()).Set(*root, "");
            }
            if (depth == 1) {
                (*stats)[path.c_str() + 1].string++;
                if (looksLikeDateTime(node->GetString()))
                    (*stats)[path.c_str() + 1].dateTime++;
            }
            break;
        case json::kArrayType:
            {
                int x = 0;
                for (json::Value::ValueIterator i = node->Begin();
                        i != node->End(); ++i) {
                    string newpath = path;
                    newpath += '/';
                    newpath += to_string(x);
                    processJSONRecord(root, i, anonymizeTable, newpath,
                            depth + 1, stats);
                    x++;
                }
            }
            break;
        case json::kObjectType:
            sort(node->MemberBegin(), node->MemberEnd(), NameComparator());
            for (json::Value::MemberIterator i = node->MemberBegin();
                    i != node->MemberEnd(); ++i) {
                string newpath = path;
                newpath += '/';
                newpath += i->name.GetString();
                processJSONRecord(root, &(i->value), anonymizeTable, newpath,
                        depth + 1, stats);
            }
            break;
        default:
            break;
    }
}

/**
  * \brief  Main ETL processor for JSON data.
  *
  * This class handles most of the ETL processing for a FOLIO interface.
  * Several functions are performed:  (1) The large JSON file retrieved
  * from Okapi is streamed in and parsed into individual JSON object
  * records, in order that only a single record needs to be held in
  * memory at a time.  (2) Some data are removed or altered as part of
  * anonymization of personal data.  (3) Statistics are collected on the
  * data types, and a table schema is generated based on the results.
  * (4) Each JSON object is normalized to enable later comparison with
  * historical data.  (5) SQL insert statements are generated and
  * submitted to the database for initial staging of the data.
  */
class JSONHandler :
    public json::BaseReaderHandler<json::UTF8<>, JSONHandler> {
public:
    const Options& opt;
    const TableSchema& tableSchema;
    etymon::Postgres* db;
    bool active = false;
    string record;
    int level = 0;
    map<string,Counts>* stats;
    size_t recordCount = 0;
    string insertBuffer;
    JSONHandler(const Options& options, const TableSchema& table,
            etymon::Postgres* database, map<string,Counts>* statistics) :
        opt(options), tableSchema(table), db(database), stats(statistics) { }
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

static void createStagingTable(const Options& opt, const string& table,
        etymon::Postgres* db)
{
    string stagingTable;
    stagingTableName(table, &stagingTable);
    string sql = ""
        "CREATE TEMP TABLE " + stagingTable + " (\n"
        "    id VARCHAR(65535),\n"
        "    data " + opt.dbtype.jsonType() + "\n"
        ");";
    printSQL(Print::debug, opt, sql);
    { etymon::PostgresResult result(db, sql); }
}

static void beginInserts(const string& table, string* buffer)
{
    string stagingTable;
    stagingTableName(table, &stagingTable);
    *buffer = "INSERT INTO " + stagingTable + " VALUES ";
}

static void endInserts(const Options& opt, string* buffer, etymon::Postgres* db)
{
    *buffer += ";\n";
    printSQL(Print::debug, opt, *buffer);
    { etymon::PostgresResult result(db, *buffer); }
    buffer->clear();
}

bool JSONHandler::EndObject(json::SizeType memberCount)
{
    if (level == 3) {
        record += '}';

        //if (opt.debug)
        //    fprintf(opt.err, 
        //            "New record parsed for \"%s\":\n%s\n",
        //            tableSchema.tableName.c_str(),
        //            record.c_str());

        // TODO Wrap buffer in a class.
        char* buffer = strdup(record.c_str());

        json::Document doc;
        doc.ParseInsitu<pflags>(buffer);

        // See:  https://rapidjson.org/md_doc_pointer.html
        // https://stackoverflow.com/questions/40833243/rapidjson-pretty-print-using-json-string-as-input-to-the-writer
        //if (Value* id = Pointer("/id").Get(d))
        //    printf("id = %s\n", id->GetString());

        //printf("id = %s\n", d["id"].GetString());
        //Pointer("/id").Set(d, "ANONYMIZED");
        //printf("id = %s\n", d["id"].GetString());

        //Pointer("/personal/lastName").Set(d, "ANONYMIZED");
        //Pointer("/personal/firstName").Set(d, "ANONYMIZED");

        string path;
        bool anonymizeTable =
            (strcmp(tableSchema.tableName.c_str(), "users") == 0);
        processJSONRecord(&doc, &doc, anonymizeTable, path, 0, stats);

        json::StringBuffer jsondata;
        json::PrettyWriter<json::StringBuffer> writer(jsondata);
        doc.Accept(writer);

        //if (opt.debug)
        //    fprintf(opt.err, "Record ready for staging:\n%s\n",
        //            jsondata.GetString());

        //const char* id = doc["id"].GetString();

        if (insertBuffer.length() > 16500000) {
            endInserts(opt, &insertBuffer, db);
            beginInserts(tableSchema.tableName, &insertBuffer);
            recordCount = 0;
        }

        string id;
        const char* idc = doc["id"].GetString();
        opt.dbtype.encodeStringConst(idc, &id);
        string data;
        const char* datac = jsondata.GetString();
        opt.dbtype.encodeStringConst(datac, &data);

        // Check if JSON object exceeds maximum string length (65535).
        if (data.length() >= 65535) {
            print(Print::warning, opt,
                    "json object size exceeds database limit: " +
                    tableSchema.sourcePath + ": " + idc);
            data = "NULL";
        }

        if (recordCount > 0)
            insertBuffer += ',';
        insertBuffer += '(';
        insertBuffer += id;
        insertBuffer += ",";
        insertBuffer += data;
        insertBuffer += ')';
        recordCount++;

        free(buffer);

        //Document document;
        //document.Parse<pflags>(record.c_str());
        //printf("id = %s\n", document["id"].GetString());
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
        beginInserts(tableSchema.tableName, &insertBuffer);
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
        if (recordCount > 0)
            endInserts(opt, &insertBuffer, db);
    } else {
        if (level > 2)
            record += "],";
    }
    level--;
    return true;
}

bool JSONHandler::Key(const char* str, json::SizeType length, bool copy)
{
    record += '\"';
    record += str;
    record += "\":";
    return true;
}

static void encodeJSON(const char* str, string* newstr)
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
                if (isprint(c)) {
                    *newstr += c;
                } else {
                    sprintf(buffer, "\\u%04X", (unsigned char) c);
                    *newstr += buffer;
                }
        }
        p++;
    }
}

bool JSONHandler::String(const char* str, json::SizeType length, bool copy)
{
    if (active && (level > 2) ) {
        record += '\"';
        string encStr;
        encodeJSON(str, &encStr);
        record += encStr;
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

size_t readPageCount(const string& loadDir, const string& tableName)
{
    string filename = loadDir;
    etymon::join(&filename, tableName);
    filename += "_count.txt";
    if ( !(etymon::fileExists(filename)) )
        return 0;
    etymon::File f(filename, "r");
    size_t count;
    int r = fscanf(f.file, "%zu", &count);
    if (r < 1 || r == EOF)
        throw runtime_error("unable to read page count from " + filename);
    return count;
}

static void stagePage(const Options& opt, const TableSchema& tableSchema,
        etymon::Postgres* db, map<string,Counts>* stats,
        const string& filename, char* readBuffer, size_t readBufferSize)
{
    json::Reader reader;
    etymon::File f(filename, "r");
    json::FileReadStream is(f.file, readBuffer, readBufferSize);
    JSONHandler handler(opt, tableSchema, db, stats);
    reader.Parse(is, handler);
}

static void stageTable(const Options& opt, TableSchema* table,
        etymon::Postgres* db, const string& loadDir)
{
    size_t pageCount = readPageCount(loadDir, table->tableName);
    if (pageCount == 0) {
        print(Print::verbose, opt, "no data found: " + table->sourcePath);
        table->skip = true;
        return;
    }

    print(Print::debug, opt, "staging: " + table->tableName +
            ": page count: " + to_string(pageCount));

    createStagingTable(opt, table->tableName, db);

    map<string,Counts> stats;
    char readBuffer[65536];

    for (size_t page = 0; page < pageCount; page++) {
        string filename = loadDir;
        etymon::join(&filename, table->tableName);
        filename += "_" + to_string(page) + ".json";
        print(Print::debug, opt, "staging: " + table->tableName + ": page: " +
                to_string(page));
        stagePage(opt, *table, db, &stats, filename, readBuffer,
                sizeof readBuffer);
    }

    if (opt.loadFromDir != "") {
        string filename = loadDir;
        etymon::join(&filename, table->tableName);
        filename += "_test.json";
        if (etymon::fileExists(filename)) {
            print(Print::debug, opt, "staging: " + table->tableName +
                    ": test file");
            stagePage(opt, *table, db, &stats, filename, readBuffer,
                    sizeof readBuffer);
        }
    }

    if (opt.debug) {
        for (const auto& [field, counts] : stats) {
            fprintf(opt.err, "%s: stats: in field:  %s\n", opt.prog,
                    field.c_str());
            fprintf(opt.err, "%s: stats: string     %8u\n", opt.prog,
                    counts.string);
            fprintf(opt.err, "%s: stats: datetime   %8u\n", opt.prog,
                    counts.dateTime);
            fprintf(opt.err, "%s: stats: bool       %8u\n", opt.prog,
                    counts.boolean);
            fprintf(opt.err, "%s: stats: number     %8u\n", opt.prog,
                    counts.number);
            fprintf(opt.err, "%s: stats: int        %8u\n", opt.prog,
                    counts.integer);
            fprintf(opt.err, "%s: stats: float      %8u\n", opt.prog,
                    counts.floating);
            fprintf(opt.err, "%s: stats: null       %8u\n", opt.prog,
                    counts.null);
        }
    }

    for (const auto& [field, counts] : stats) {
        ColumnSchema column;
        column.columnType = ColumnSchema::selectColumnType(counts);
        string typeStr;
        ColumnSchema::columnTypeToString(column.columnType, &typeStr);
        string newattr;
        decodeCamelCase(field.c_str(), &newattr);
        print(Print::debug, opt,
                string("column: ") + newattr + string(" ") + typeStr);
        //if (opt.debug)
        //    fprintf(opt.err, "debug: column: %s %s\n",
        //            newattr.c_str(), typeStr.c_str());
        column.columnName = newattr;
        column.sourceColumnName = field;
        table->columns.push_back(column);
    }
}

void stageAll(const Options& opt, Schema* schema, etymon::Postgres* db,
        const string& loadDir)
{
    print(Print::verbose, opt, "staging in target: " + opt.target);
    for (auto& table : schema->tables) {
        if (table.skip)
            continue;
        print(Print::verbose, opt, "staging table: " + table.tableName);
        stageTable(opt, &table, db, loadDir);
    }
}

