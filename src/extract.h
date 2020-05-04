#ifndef LDP_EXTRACT_H
#define LDP_EXTRACT_H

#include <string>
#include <unistd.h>

#include "options.h"
#include "schema.h"

class ExtractionFiles {
public:
    string dir;
    vector<string> files;
    const Options& opt;
    ExtractionFiles(const Options& options) : opt(options) {};
    ~ExtractionFiles();
};

class Curl {
public:
    CURL* curl;
    struct curl_slist* headers;
    Curl();
    ~Curl();
};

void okapiLogin(const Options& o, Log* log, string* token);

bool directOverride(const Options& opt, const string& sourcePath);
bool retrieveDirect(const Options& opt, Log* log, const TableSchema& table,
        const string& loadDir, ExtractionFiles* extractionFiles);
bool retrievePages(const Curl& c, const Options& opt, Log* log,
        const string& token, const TableSchema& table, const string& loadDir,
        ExtractionFiles* extractionFiles);

//void extract(const Options& o, Schema* spec, const string& token,
//        const string& loadDir, ExtractionFiles* extractionFiles);

#endif

