#include <cassert>
#include <cstring>
#include <ctype.h>
#include <curl/curl.h>
#include <iostream>
#include <stdio.h>

#include "../etymoncpp/include/util.h"
#include "extract.h"
#include "paging.h"
#include "timer.h"
#include "util.h"

ExtractionFiles::~ExtractionFiles()
{
    if (!savetemps) {
        for (const auto& f : files)
            unlink(f.c_str());
        if (dir != "") {
            print(Print::debug, opt, string("removing directory: ") + dir);
            int r = rmdir(dir.c_str());
            if (r == -1)
                print(Print::warning, opt,
                        string("unable to remove temporary directory: ") +
                        dir + string(": ") + string(strerror(errno)) );
        }
    } else {
        print(Print::info, opt, string("directory not removed: ") + dir);
    }
}

class Curl {
public:
    CURL* curl;
    struct curl_slist* headers;
    Curl();
    ~Curl();
};

Curl::Curl()
{
    curl = curl_easy_init();
    headers = NULL;
}

Curl::~Curl()
{
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
}

void encodeLogin(const string& okapiUser, const string& okapiPassword,
        string* login)
{
    *login += string("{");
    *login += "\"username\":\"" ;
    *login += okapiUser + "\",";
    *login += "\"password\":\"" ;
    *login += okapiPassword + "\"";
    *login += "}";
}

size_t write_callback(char* buffer, size_t size, size_t nitems,
		       void* userdata)
{
    *((string*) userdata) = buffer;
    return size * nitems;
}

size_t header_callback(char* buffer, size_t size, size_t nitems,
		       void* userdata)
{
    if (!strncasecmp(buffer, "x-okapi-token:", 14)) {
        *((string*) userdata) = buffer;
    }
    return size * nitems;
}

/**
 * \brief Performs the equivalent of a login to Okapi.
 *
 * \param[in] o Options specifying okapiURL, okapiUser, okapiPassword,
 * and okapiTenant.
 * \param[out] token The authentication token received from Okapi.
 */
void okapiLogin(const Options& opt, string* token)
{
    Timer timer(opt);

    string login;
    encodeLogin(opt.okapiUser, opt.okapiPassword, &login);

    string path = opt.okapiURL;
    etymon::join(&path, "/authn/login");

    print(Print::debug, opt, string("retrieving: ") + path);

    string tenantHeader = "X-Okapi-Tenant: ";
    tenantHeader += opt.okapiTenant;
    string bodyData;
    string tokenData;

    Curl c;
    if (c.curl) {
        c.headers = curl_slist_append(c.headers, tenantHeader.c_str());
        c.headers = curl_slist_append(c.headers,
                "Content-Type: application/json");
        c.headers = curl_slist_append(c.headers,
                "Accept: "
                "application/json,text/plain");
        curl_easy_setopt(c.curl, CURLOPT_URL, path.c_str());
        curl_easy_setopt(c.curl, CURLOPT_POSTFIELDS, login.c_str());
        curl_easy_setopt(c.curl, CURLOPT_POSTFIELDSIZE, login.size());
        curl_easy_setopt(c.curl, CURLOPT_HTTPHEADER, c.headers);
        curl_easy_setopt(c.curl, CURLOPT_WRITEDATA, &bodyData);
        curl_easy_setopt(c.curl, CURLOPT_WRITEFUNCTION,
                write_callback);
        curl_easy_setopt(c.curl, CURLOPT_HEADERDATA, &tokenData);
        curl_easy_setopt(c.curl, CURLOPT_HEADERFUNCTION,
                header_callback);

        CURLcode code = curl_easy_perform(c.curl);

        if (code) {
            throw runtime_error(string("logging in to Okapi: ") +
                    curl_easy_strerror(code));
        }

        long response_code = 0;
        curl_easy_getinfo(c.curl, CURLINFO_RESPONSE_CODE, &response_code);
        if (response_code != 201) {
            string err = "logging in to okapi: server response:\n";
            err += bodyData;
            throw runtime_error(err);
        }
    }
    // Trim token string.
    // Trim white space.
    etymon::trim(&tokenData);
    // Move past header name.
    const char* t = tokenData.c_str();
    while (*t != '\0' && !isspace(*t))
        t++;
    while (*t != '\0' && isspace(*t))
        t++;
    *token = t;

    // TODO enable times
    // Temporarily disabled until timing added for staging, merging, etc.
    //if (opt.verbose)
    //    timer.print("login time");
}

static bool retrieve(const Curl& c, const Options& opt, const string& token,
        const TableSchema& table, const string& loadDir,
        ExtractionFiles* extractionFiles, size_t page)
{
    Timer timer(opt);

    string path = opt.okapiURL;
    etymon::join(&path, table.sourcePath);

    path += "?offset=" + to_string(page * opt.pageSize) +
        "&limit=" + to_string(opt.pageSize) +
        "&query=cql.allRecords%3d1%20sortby%20id";

    //path += "?offset=0&limit=1000&query=id==*%20sortby%20id";
    //if (table.sourcePath.find("/erm/") == 0)
    //    path += "?stats=true&offset=0&max=100";

    string output = loadDir;
    etymon::join(&output, table.tableName);
    output += "_" + to_string(page) + ".json";

    {
        etymon::File f(output, "wb");
        extractionFiles->files.push_back(output);

        // testing
        //curl_easy_setopt(c.curl, CURLOPT_TIMEOUT, 100000);
        //curl_easy_setopt(c.curl, CURLOPT_CONNECTTIMEOUT, 100000);

        curl_easy_setopt(c.curl, CURLOPT_URL, path.c_str());
        curl_easy_setopt(c.curl, CURLOPT_WRITEDATA, f.file);

        print(Print::debug, opt, string("retrieving: ") + path);

        CURLcode code = curl_easy_perform(c.curl);

        if (code) {
            throw runtime_error(string("error retrieving data from Okapi: ") +
                    curl_easy_strerror(code));
        }
    }

    long response_code = 0;
    curl_easy_getinfo(c.curl, CURLINFO_RESPONSE_CODE, &response_code);
    if (response_code != 200) {
        string err = string("error retrieving data from Okapi: ") +
            to_string(response_code);
            //string(": server response in file: " + output);
        // TODO read and print server response, before tmp files cleaned up
        throw runtime_error(err);
    }

    bool records = pageEmpty(opt, output);

    // TODO enable times
    // Temporarily disabled until timing added for staging, merging, etc.
    //if (opt.verbose)
    //    timer.print("extraction time");

    return records;
}

static void retrievePages(const Curl& c, const Options& opt,
        const string& token, const TableSchema& table, const string& loadDir,
        ExtractionFiles* extractionFiles)
{
    size_t page = 0;
    while (true) {
        print(Print::debug, opt, "page: " + to_string(page));
        bool records = retrieve(c, opt, token, table, loadDir,
                extractionFiles, page);
        if (!records) {
            string countFile = loadDir;
            etymon::join(&countFile, table.tableName);
            countFile += "_count.txt";
            etymon::File f(countFile, "w");
            extractionFiles->files.push_back(countFile);
            string pageStr = to_string(page) + "\n";
            fputs(pageStr.c_str(), f.file);
            break;
        }
        page++;
    }
}

void extract(const Options& opt, const Schema& schema, const string& token,
        const string& loadDir, ExtractionFiles* extractionFiles)
{
    Curl c;
    if (c.curl) {

        string tenantHeader = "X-Okapi-Tenant: ";
        tenantHeader + opt.okapiTenant;

        string tokenHeader = "X-Okapi-Token: ";
        tokenHeader += token;

        c.headers = curl_slist_append(c.headers, tenantHeader.c_str());
        c.headers = curl_slist_append(c.headers, tokenHeader.c_str());
        c.headers = curl_slist_append(c.headers,
                "Accept: "
                "application/json,text/plain");
        curl_easy_setopt(c.curl, CURLOPT_HTTPHEADER, c.headers);

        fprintf(opt.err, "%s: extracting data from source: %s\n", opt.prog,
                opt.source.c_str());
        for (auto table : schema.tables) {
            if (opt.verbose)
                fprintf(opt.err, "%s: extracting: %s\n", opt.prog,
                        table.sourcePath.c_str());
            retrievePages(c, opt, token, table, loadDir, extractionFiles);
        }
    }
}

