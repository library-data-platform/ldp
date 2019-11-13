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
    bool savetemps = false;
    const Options& opt;
    ExtractionFiles(const Options& options) : opt(options) {};
    ~ExtractionFiles();
};

void okapiLogin(const Options& o, string* token);

void extract(const Options& o, const Schema& spec, const string& token,
        const string& loadDir, ExtractionFiles* extractionFiles);

#endif

