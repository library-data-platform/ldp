#ifndef LDP_EXTRACT_H
#define LDP_EXTRACT_H

#include <curl/curl.h>
#include <string>
#include <unistd.h>

#include "options.h"
#include "schema.h"

class ExtractionFiles {
public:
    string dir;
    vector<string> files;
    const options& opt;
    ExtractionFiles(const options& options) : opt(options) {};
    ~ExtractionFiles();
};

class Curl {
public:
    CURL* curl;
    struct curl_slist* headers;
    Curl();
    ~Curl();
};

void okapiLogin(const options& o, log* lg, string* token);

bool directOverride(const options& opt, const string& sourcePath);
bool retrieveDirect(const options& opt, log* lg, const TableSchema& table,
        const string& loadDir, ExtractionFiles* extractionFiles);
bool retrievePages(const Curl& c, const options& opt, log* lg,
        const string& token, const TableSchema& table, const string& loadDir,
        ExtractionFiles* extractionFiles);

//void extract(const options& o, Schema* spec, const string& token,
//        const string& loadDir, ExtractionFiles* extractionFiles);

#endif

