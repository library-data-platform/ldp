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
    const ldp_options& opt;
    extraction_files(const ldp_options& options) : opt(options) {};
    ~extraction_files();
};

class Curl {
public:
    CURL* curl;
    struct curl_slist* headers;
    Curl();
    ~Curl();
};

void okapi_login(const ldp_options& o, ldp_log* lg, string* token);

bool directOverride(const ldp_options& opt, const string& sourcePath);
bool retrieveDirect(const ldp_options& opt, ldp_log* lg, const TableSchema& table,
        const string& loadDir, extraction_files* ext_files);
bool retrievePages(const Curl& c, const ldp_options& opt, ldp_log* lg,
        const string& token, const TableSchema& table, const string& loadDir,
        extraction_files* ext_files);

#endif

