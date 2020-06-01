#include <cassert>
#include <cstring>
#include <ctype.h>
#include <iostream>
#include <stdio.h>

#include "../etymoncpp/include/postgres.h"
#include "../etymoncpp/include/util.h"
#include "extract.h"
#include "paging.h"
#include "timer.h"
#include "util.h"

extraction_files::~extraction_files()
{
    if (!opt.savetemps) {
        for (const auto& f : files)
            unlink(f.c_str());
        if (dir != "") {
            int r = rmdir(dir.c_str());
            if (r == -1)
                print(Print::warning, opt,
                        string("unable to remove temporary directory: ") +
                        dir + string(": ") + string(strerror(errno)) );
        }
    } else {
        if (dir != "") {
            print(Print::verbose, opt, string("directory not removed: ") + dir);
        }
    }
}

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

/* *
 * \brief Performs the equivalent of a login to Okapi.
 *
 * \param[in] opt Options specifying okapiURL, okapiUser,
 * okapiPassword, and okapiTenant.
 * \param[out] token The authentication token received from Okapi.
 */
void okapi_login(const options& opt, log* lg, string* token)
{
    //timer t(opt);

    string login;
    encodeLogin(opt.okapi_user, opt.okapi_password, &login);

    string path = opt.okapi_url;
    etymon::join(&path, "/authn/login");

    lg->write(level::detail, "", "", "Retrieving: " + path, -1);

    string tenantHeader = "X-Okapi-Tenant: ";
    tenantHeader += opt.okapi_tenant;
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
            throw runtime_error(string("logging in to okapi: ") +
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
    //    t.print("login time");
}

enum class PageStatus {
    interfaceNotAvailable,
    pageEmpty,
    containsRecords
};

static PageStatus retrieve(const Curl& c, const options& opt, log* lg,
        const string& token, const TableSchema& table, const string& loadDir,
        extraction_files* ext_files, size_t page)
{
    // TODO move timing code to calling function and re-enable
    //timer t(opt);

    string path = opt.okapi_url;
    etymon::join(&path, table.sourcePath);

    //path += "?offset=" + to_string(page * opt.pageSize) +
    //    "&limit=" + to_string(opt.pageSize) +
    //    "&query=cql.allRecords%3d1%20sortby%20id";

    string query = "?offset=" + to_string(page * opt.page_size) +
        "&limit=" + to_string(opt.page_size) +
        "&query=cql.allRecords%3d1%20sortby%20id";
    path += query;

    //path += "?offset=0&limit=1000&query=id==*%20sortby%20id";
    //if (table.sourcePath.find("/erm/") == 0)
    //    path += "?stats=true&offset=0&max=100";

    string output = loadDir;
    etymon::join(&output, table.tableName);
    output += "_" + to_string(page) + ".json";

    {
        etymon::file f(output, "wb");
        ext_files->files.push_back(output);

        // testing
        //curl_easy_setopt(c.curl, CURLOPT_TIMEOUT, 100000);
        //curl_easy_setopt(c.curl, CURLOPT_CONNECTTIMEOUT, 100000);

        curl_easy_setopt(c.curl, CURLOPT_URL, path.c_str());
        curl_easy_setopt(c.curl, CURLOPT_WRITEDATA, f.fp);

        lg->write(level::detail, "", "",
                "Retrieving from:\n"
                "    Path: " + table.sourcePath + "\n"
                "    Query: " + query, -1);

        CURLcode code = curl_easy_perform(c.curl);

        if (code) {
            throw runtime_error(string("error retrieving data from okapi: ") +
                    curl_easy_strerror(code));
        }
    }

    long response_code = 0;
    curl_easy_getinfo(c.curl, CURLINFO_RESPONSE_CODE, &response_code);
    if (response_code == 403 || response_code == 404 || response_code == 500) {
        return PageStatus::interfaceNotAvailable;
    }
    if (response_code != 200) {
        string err = string("error retrieving data from okapi: ") +
            to_string(response_code);
            //string(": server response in file: " + output);
        // TODO read and print server response, before tmp files cleaned up
        throw runtime_error(err);
    }

    bool empty = pageIsEmpty(opt, output);
    return empty ? PageStatus::pageEmpty : PageStatus::containsRecords;

    // TODO move timing code to calling function and re-enable
    // Temporarily disabled until timing added for staging, merging, etc.
    //if (opt.verbose)
    //    t.print("extraction time");
}

static void writeCountFile(const string& loadDir, const string& tableName,
        extraction_files* ext_files, size_t page) {
    string countFile = loadDir;
    etymon::join(&countFile, tableName);
    countFile += "_count.txt";
    etymon::file f(countFile, "w");
    ext_files->files.push_back(countFile);
    string pageStr = to_string(page) + "\n";
    fputs(pageStr.c_str(), f.fp);
}

bool retrievePages(const Curl& c, const options& opt, log* lg,
        const string& token, const TableSchema& table, const string& loadDir,
        extraction_files* ext_files)
{
    size_t page = 0;
    while (true) {
        lg->write(level::detail, "", "",
                "Extracting page: " + to_string(page), -1);
        PageStatus status = retrieve(c, opt, lg, token, table, loadDir,
                ext_files, page);
        switch (status) {
        case PageStatus::interfaceNotAvailable:
            lg->write(level::trace, "", "",
                    "Interface not available: " + table.sourcePath, -1);
            return false;
        case PageStatus::pageEmpty:
            writeCountFile(loadDir, table.tableName, ext_files, page);
            return true;
        case PageStatus::containsRecords:
            break;
        }
        page++;
    }
}

bool directOverride(const options& opt, const string& tableName)
{
    for (auto& t : opt.direct.table_names) {
        if (t == tableName)
            return true;
    }
    return false;
}

bool retrieveDirect(const options& opt, log* lg, const TableSchema& table,
        const string& loadDir, extraction_files* ext_files)
{
    lg->write(level::trace, "", "",
            "Direct from database: " + table.sourcePath, -1);
    if (table.directSourceTable == "") {
        lg->write(level::warning, "", "",
                "Direct source table undefined: " + table.sourcePath, -1);
        return false;
    }

    // Select jsonb from table.directSourceTable and write to JSON file.
    etymon::Postgres db(opt.direct.database_host, opt.direct.database_port,
            opt.direct.database_user, opt.direct.database_password,
            opt.direct.database_name, "require");
    string sql = "SELECT jsonb FROM " +
        opt.okapi_tenant + "_" + table.directSourceTable + ";";
    lg->write(level::detail, "", "", sql, -1);

    if (PQsendQuery(db.conn, sql.c_str()) == 0) {
        string err = PQerrorMessage(db.conn);
        throw runtime_error(err);
    }
    if (PQsetSingleRowMode(db.conn) == 0)
        throw runtime_error("unable to set single-row mode in database query");

    string output = loadDir;
    etymon::join(&output, table.tableName + "_0.json");
    etymon::file f(output, "w");
    ext_files->files.push_back(output);

    fprintf(f.fp, "{\n  \"a\": [\n");

    int row = 0;
    while (true) {
        etymon::PostgresResultAsync res(&db);
        if (res.result == nullptr ||
                PQresultStatus(res.result) != PGRES_SINGLE_TUPLE)
            break;
        const char* j = PQgetvalue(res.result, 0, 0);
        //if (j == nullptr)
        //    break;
        if (row > 0)
            fprintf(f.fp, ",\n");
        fprintf(f.fp, "%s\n", j);
        row++;
    }
    if (row == 0)
        return false;

    fprintf(f.fp, "\n  ]\n}\n");

    // Write 1 to count file.
    writeCountFile(loadDir, table.tableName, ext_files, 1);

    return true;
}

