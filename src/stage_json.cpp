#include <algorithm>
#include <cstdio>
#include <map>
#include <memory>
#include <regex>

#include "../etymoncpp/include/postgres.h"
#include "../etymoncpp/include/util.h"
#include "anonymize.h"
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
#include "stage_json.h"
#include "util.h"

//using namespace std;
namespace json = rapidjson;

constexpr json::ParseFlag pflags = json::kParseTrailingCommasFlag;

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

// Collect statistics and anonimize data
void processJSONRecord(json::Document* root, json::Value* node,
        bool collectStats, bool anonymizeTable, const string& path,
        unsigned int depth, map<string,Counts>* stats)
{
    switch (node->GetType()) {
        case json::kNullType:
            if (collectStats && depth == 1)
                (*stats)[path.c_str() + 1].null++;
            break;
        case json::kTrueType:
        case json::kFalseType:
            if (anonymizeTable && possiblePersonalData(path.c_str())) {
                json::Pointer(path.c_str()).Set(*root, false);
            }
            if (collectStats && depth == 1)
                (*stats)[path.c_str() + 1].boolean++;
            break;
        case json::kNumberType:
            if (anonymizeTable && possiblePersonalData(path.c_str())) {
                json::Pointer(path.c_str()).Set(*root, 0);
            }
            if (collectStats && depth == 1) {
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
            if (collectStats && depth == 1) {
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
                    processJSONRecord(root, i, collectStats, anonymizeTable,
                            newpath, depth + 1, stats);
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
                processJSONRecord(root, &(i->value), collectStats,
                        anonymizeTable, newpath, depth + 1, stats);
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
class JSONHandler :
    public json::BaseReaderHandler<json::UTF8<>, JSONHandler> {
public:
    int pass;
    const Options& opt;
    int level = 0;
    bool active = false;
    string record;
    const TableSchema& tableSchema;
    // Collection of statistics
    map<string,Counts>* stats;
    // Loading to database
    etymon::Postgres* db;
    size_t recordCount = 0;
    string insertBuffer;
    JSONHandler(int pass, const Options& options, const TableSchema& table,
            etymon::Postgres* database, map<string,Counts>* statistics) :
        pass(pass), opt(options), tableSchema(table), stats(statistics),
        db(database) { }
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

static void beginInserts(const string& table, string* buffer)
{
    string loadingTable;
    loadingTableName(table, &loadingTable);
    *buffer = "INSERT INTO " + loadingTable + " VALUES ";
}

static void endInserts(const Options& opt, string* buffer, etymon::Postgres* db)
{
    *buffer += ";\n";
    printSQL(Print::debug, opt, *buffer);
    { etymon::PostgresResult result(db, *buffer); }
    buffer->clear();
}

static void writeTuple(const Options& opt, const TableSchema& table,
        const json::Document& doc, const json::StringBuffer& jsondata,
        size_t* recordCount, string* insertBuffer)
{
    if (*recordCount > 0)
        *insertBuffer += ',';
    *insertBuffer += '(';

    const char* id = doc["id"].GetString();
    string idenc;
    opt.dbtype.encodeStringConst(id, &idenc);
    *insertBuffer += idenc;
    *insertBuffer += ",";

    string s;
    for (const auto& column : table.columns) {
        if (column.columnName == "id")
            continue;
        const char* sourceColumnName = column.sourceColumnName.c_str();
        if (doc.HasMember(sourceColumnName) == false) {
            *insertBuffer += "NULL,";
            continue;
        }
        const json::Value& jsonValue = doc[sourceColumnName];
        if (jsonValue.IsNull()) {
            *insertBuffer += "NULL,";
            continue;
        }
        switch (column.columnType) {
        case ColumnType::bigint:
            *insertBuffer += to_string(jsonValue.GetInt());
            break;
        case ColumnType::boolean:
            *insertBuffer += ( jsonValue.GetBool() ?  "TRUE" : "FALSE" );
            break;
        case ColumnType::numeric:
            *insertBuffer += to_string(jsonValue.GetDouble());
            break;
        case ColumnType::timestamptz:
        case ColumnType::varchar:
            opt.dbtype.encodeStringConst(jsonValue.GetString(), &s);
            // Check if varchar exceeds maximum string length (65535).
            if (s.length() >= 65535) {
                print(Print::warning, opt,
                        "string length exceeds database limit: " +
                        table.sourcePath + ": " + id + ": " +
                        column.columnName);
                s = "NULL";
            }
            *insertBuffer += s;
            break;
        }
        *insertBuffer += ",";
    }

    string data;
    opt.dbtype.encodeStringConst(jsondata.GetString(), &data);
    // Check if JSON object exceeds maximum string length (65535).
    if (data.length() >= 65535) {
        print(Print::warning, opt,
                "json object size exceeds database limit: " +
                table.sourcePath + ": " + id);
        data = "NULL";
    }
    *insertBuffer += data;
    *insertBuffer += ",1)";
    (*recordCount)++;
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
        bool collectStats = (pass == 1);
        bool anonymizeTable;
        if (pass == 1) {
            anonymizeTable = false;
        } else {
            anonymizeTable =
                (strcmp(tableSchema.tableName.c_str(), "users") == 0);
        }
        // Collect statistics and anonimize data
        processJSONRecord(&doc, &doc, collectStats, anonymizeTable, path, 0,
                stats);

        if (pass == 2) {

            // Normalize JSON format
            json::StringBuffer jsondata;
            json::PrettyWriter<json::StringBuffer> writer(jsondata);
            doc.Accept(writer);

            //if (opt.debug)
            //    fprintf(opt.err, "Record ready for staging:\n%s\n",
            //            jsondata.GetString());

            if (insertBuffer.length() > 16500000) {
                endInserts(opt, &insertBuffer, db);
                beginInserts(tableSchema.tableName, &insertBuffer);
                recordCount = 0;
            }

            writeTuple(opt, tableSchema, doc, jsondata, &recordCount,
                    &insertBuffer);

        }

        free(buffer);

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
            if (pass == 2)
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

static void stagePage(const Options& opt, int pass,
        const TableSchema& tableSchema, etymon::Postgres* db,
        map<string,Counts>* stats, const string& filename, char* readBuffer,
        size_t readBufferSize)
{
    json::Reader reader;
    etymon::File f(filename, "r");
    json::FileReadStream is(f.file, readBuffer, readBufferSize);
    JSONHandler handler(pass, opt, tableSchema, db, stats);
    reader.Parse(is, handler);
}

static void composeDataFilePath(const string& loadDir,
        const TableSchema& table, const string& suffix, string* path)
{
    *path = loadDir;
    etymon::join(path, table.tableName);
    *path += suffix;
}

static void createLoadingTable(const Options& opt, const TableSchema& table,
        etymon::Postgres* db)
{
    string loadingTable;
    loadingTableName(table.tableName, &loadingTable);

    string sql = "CREATE TABLE ";
    sql += loadingTable;
    sql += " (\n"
        "    id VARCHAR(65535) NOT NULL,\n";
    string columnType;
    for (const auto& column : table.columns) {
        if (column.columnName != "id") {
            sql += "    \"";
            sql += column.columnName;
            sql += "\" ";
            ColumnSchema::columnTypeToString(column.columnType, &columnType);
            sql += columnType;
            sql += ",\n";
        }
    }
    sql += "    data ";
    sql += opt.dbtype.jsonType();
    sql += ",\n"
        "    tenant_id SMALLINT NOT NULL,\n"
        "    PRIMARY KEY (tenant_id, id)\n"
        ");";
    printSQL(Print::debug, opt, sql);
    { etymon::PostgresResult result(db, sql); }

    // Add comment on table.
    if (table.moduleName != "mod-agreements") {
        sql = "COMMENT ON TABLE " + loadingTable + " IS '";
        sql += table.sourcePath;
        sql += " in ";
        sql += table.moduleName;
        sql += ": ";
        sql += "https://dev.folio.org/reference/api/#";
        sql += table.moduleName;
        sql += "';";
        printSQL(Print::debug, opt, sql);
        { etymon::PostgresResult result(db, sql); }
    }

}

static void stageTable(const Options& opt, TableSchema* table,
        etymon::Postgres* db, const string& loadDir)
{
    size_t pageCount = readPageCount(loadDir, table->tableName);

    print(Print::debug, opt, "staging: " + table->tableName +
            ": page count: " + to_string(pageCount));

    // TODO remove this and create the load table from merge.cpp after
    // pass 1
    //createStagingTable(opt, table->tableName, db);

    map<string,Counts> stats;
    char readBuffer[65536];

    for (int pass = 1; pass <= 2; pass++) {

        print(Print::debug, opt, "staging: " + table->tableName +
                (pass == 1 ?  ": analyze" : ": load"));

        for (size_t page = 0; page < pageCount; page++) {
            string path;
            composeDataFilePath(loadDir, *table,
                    "_" + to_string(page) + ".json", &path);
            print(Print::debug, opt, "staging: " + table->tableName +
                    (pass == 1 ?  ": analyze" : ": load") + ": page: " +
                    to_string(page));
            stagePage(opt, pass, *table, db, &stats, path, readBuffer,
                    sizeof readBuffer);
        }

        if (opt.loadFromDir != "") {
            string path;
            composeDataFilePath(loadDir, *table, "_test.json", &path);
            if (etymon::fileExists(path)) {
                print(Print::debug, opt, "staging: " + table->tableName +
                        (pass == 1 ?  ": analyze" : ": load") +
                        ": test file");
                stagePage(opt, pass, *table, db, &stats, path, readBuffer,
                        sizeof readBuffer);
            }
        }

        if (pass == 1 && opt.debug) {
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

        if (pass == 1) {
            for (const auto& [field, counts] : stats) {
                ColumnSchema column;
                column.columnType = ColumnSchema::selectColumnType(counts);
                string typeStr;
                ColumnSchema::columnTypeToString(column.columnType, &typeStr);
                string newattr;
                decodeCamelCase(field.c_str(), &newattr);
                print(Print::debug, opt,
                        string("column: ") + newattr + string(" ") + typeStr);
                column.columnName = newattr;
                column.sourceColumnName = field;
                table->columns.push_back(column);
            }
            createLoadingTable(opt, *table, db);
        }

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

