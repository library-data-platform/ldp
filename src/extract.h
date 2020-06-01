#ifndef LDP_EXTRACT_H
#define LDP_EXTRACT_H

#include <curl/curl.h>
#include <string>
#include <unistd.h>

#include "options.h"
#include "schema.h"

class extraction_files {
public:
    string dir;
    vector<string> files;
    const options& opt;
    extraction_files(const options& options) : opt(options) {};
    ~extraction_files();
};

class Curl {
public:
    CURL* curl;
    struct curl_slist* headers;
    Curl();
    ~Curl();
};

void okapi_login(const options& o, log* lg, string* token);

bool directOverride(const options& opt, const string& sourcePath);
bool retrieveDirect(const options& opt, log* lg, const TableSchema& table,
        const string& loadDir, extraction_files* ext_files);
bool retrievePages(const Curl& c, const options& opt, log* lg,
        const string& token, const TableSchema& table, const string& loadDir,
        extraction_files* ext_files);

#endif

