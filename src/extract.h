#ifndef LDP_EXTRACT_H
#define LDP_EXTRACT_H

#include <curl/curl.h>

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

class curl_wrapper {
public:
    CURL* curl;
    struct curl_slist* headers;
    curl_wrapper();
    ~curl_wrapper();
};

void okapi_login(const ldp_options& o, ldp_log* lg, string* token);

bool direct_override(const ldp_options& opt, const string& sourcePath);
bool retrieve_direct(const ldp_options& opt, ldp_log* lg,
                     const table_schema& table, const string& loadDir,
                     extraction_files* ext_files);
bool retrieve_pages(const curl_wrapper& c, const ldp_options& opt, ldp_log* lg,
                    const string& token, const table_schema& table,
                    const string& loadDir, extraction_files* ext_files);

#endif

