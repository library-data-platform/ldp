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
#include "stage.h"

namespace json = rapidjson;

class PagingJSONHandler :
    public json::BaseReaderHandler<json::UTF8<>, PagingJSONHandler> {
public:
    int level = 0;
    bool foundRecord = false;
    const options& opt;
    PagingJSONHandler(const options& options) : opt(options) { }
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

bool PagingJSONHandler::StartObject()
{
    if (level == 2) {
        foundRecord = true;
        return false;
    }
    level++;
    return true;
}

bool PagingJSONHandler::EndObject(json::SizeType memberCount)
{
    level--;
    return true;
}

bool PagingJSONHandler::StartArray()
{
    level++;
    return true;
}

bool PagingJSONHandler::EndArray(json::SizeType elementCount)
{
    level--;
    return true;
}

bool PagingJSONHandler::Key(const char* str, json::SizeType length, bool copy)
{
    return true;
}

bool PagingJSONHandler::String(const char* str, json::SizeType length, bool copy)
{
    return true;
}

bool PagingJSONHandler::Int(int i)
{
    return true;
}

bool PagingJSONHandler::Uint(unsigned u)
{
    return true;
}

bool PagingJSONHandler::Int64(int64_t i)
{
    return true;
}

bool PagingJSONHandler::Uint64(uint64_t u)
{
    return true;
}

bool PagingJSONHandler::Double(double d)
{
    return true;
}

bool PagingJSONHandler::Bool(bool b)
{
    return true;

}

bool PagingJSONHandler::Null()
{
    return true;
}

bool pageIsEmpty(const options& opt, const string& filename)
{
    PagingJSONHandler handler(opt);
    json::Reader reader;
    char readBuffer[65536];
    etymon::file f(filename, "r");
    json::FileReadStream is(f.fp, readBuffer, sizeof(readBuffer));
    reader.Parse(is, handler);
    return !(handler.foundRecord);
}

