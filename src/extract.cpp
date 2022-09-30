#include <cassert>
#include <cstring>
#include <ctype.h>
#include <fstream>
#include <iostream>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <unistd.h>

#include "../etymoncpp/include/postgres.h"
#include "../etymoncpp/include/util.h"
#include "extract.h"
#include "paging.h"
#include "stage.h"
#include "timer.h"
#include "util.h"

extraction_files::~extraction_files()
{
    if (!opt.savetemps) {
        for (const auto& f : files) {
            unlink(f.c_str());
        }
        if (dir != "") {
            int r = rmdir(dir.c_str());
            if (r == -1) {
                log->write(log_level::warning, "", "", string("unable to remove temporary directory: ") + dir + string(": ") + string(strerror(errno)), -1);
            }
        }
    } else {
        if (dir != "") {
            log->write(log_level::warning, "", "", string("directory not removed: ") + dir, -1);
        }
    }
}

curl_wrapper::curl_wrapper()
{
    curl = curl_easy_init();
    headers = NULL;
}

curl_wrapper::~curl_wrapper()
{
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
}

// struct curl_data {
//     char trace_ascii; /* 1 or 0 */ 
// };

// static void curl_dump(const char *text, FILE *stream, unsigned char *ptr, size_t size, char nohex)
// {
//     size_t i;
//     size_t c;

//     unsigned int width = 0x10;

//     if(nohex)
// 	/* without the hex output, we can fit more on screen */ 
// 	width = 0x40;

//     fprintf(stream, "%s, %10.10lu bytes (0x%8.8lx)\n",
// 	    text, (unsigned long)size, (unsigned long)size);

//     for(i = 0; i<size; i += width) {

// 	fprintf(stream, "%4.4lx: ", (unsigned long)i);

// 	if(!nohex) {
// 	    /* hex not disabled, show it */ 
// 	    for(c = 0; c < width; c++)
// 		if(i + c < size)
// 		    fprintf(stream, "%02x ", ptr[i + c]);
// 		else
// 		    fputs("   ", stream);
// 	}

// 	for(c = 0; (c < width) && (i + c < size); c++) {
// 	    /* check for 0D0A; if found, skip past and start a new line of output */ 
// 	    if(nohex && (i + c + 1 < size) && ptr[i + c] == 0x0D &&
// 	       ptr[i + c + 1] == 0x0A) {
// 		i += (c + 2 - width);
// 		break;
// 	    }
// 	    fprintf(stream, "%c",
// 		    (ptr[i + c] >= 0x20) && (ptr[i + c]<0x80)?ptr[i + c]:'.');
// 	    /* check again for 0D0A, to avoid an extra \n if it's at width */ 
// 	    if(nohex && (i + c + 2 < size) && ptr[i + c + 1] == 0x0D &&
// 	       ptr[i + c + 2] == 0x0A) {
// 		i += (c + 3 - width);
// 		break;
// 	    }
// 	}
// 	fputc('\n', stream); /* newline */ 
//     }
//     fflush(stream);
// }

// static int curl_trace(CURL *handle, curl_infotype type, char *data, size_t size, void *userp)
// {
//     struct curl_data *config = (struct curl_data *)userp;
//     const char *text;
//     (void)handle; /* prevent compiler warning */ 

//     switch(type) {
//     case CURLINFO_TEXT:
//         fprintf(stderr, "== Info: %s", data);
//         /* FALLTHROUGH */ 
//     default: /* in case a new one is introduced to shock us */ 
//         return 0;

//     case CURLINFO_HEADER_OUT:
//         text = "=> Send header";
//         break;
//     case CURLINFO_DATA_OUT:
//         text = "=> Send data";
//         break;
//     case CURLINFO_SSL_DATA_OUT:
//         text = "=> Send SSL data";
//         break;
//     case CURLINFO_HEADER_IN:
//         text = "<= Recv header";
//         break;
//     case CURLINFO_DATA_IN:
//         text = "<= Recv data";
//         break;
//     case CURLINFO_SSL_DATA_IN:
//         text = "<= Recv SSL data";
//         break;
//     }

//     curl_dump(text, stderr, (unsigned char *)data, size, config->trace_ascii);
//     return 0;
// }

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
void okapi_login(const ldp_options& opt, const data_source& source,
                 ldp_log* lg, string* token)
{
    //timer t(opt);

    string login;
    encodeLogin(source.okapi_user, source.okapi_password, &login);

    string auth = "/authn/login";
    string path = source.okapi_url;
    etymon::join(&path, auth);

    lg->write(log_level::trace, "", "", "authenticating with okapi", -1);

    string tenantHeader = "X-Okapi-Tenant: ";
    tenantHeader += source.okapi_tenant;
    string bodyData;
    string tokenData;

    curl_wrapper c;
    // struct curl_data curl_config;
    if (c.curl) {

        if (opt.lg_level == log_level::detail) {
            // curl_easy_setopt(c.curl, CURLOPT_DEBUGFUNCTION, curl_trace);
            // curl_easy_setopt(c.curl, CURLOPT_DEBUGDATA, &curl_config);
            curl_easy_setopt(c.curl, CURLOPT_VERBOSE, 1L);
        }

        c.headers = curl_slist_append(c.headers, tenantHeader.c_str());
        c.headers = curl_slist_append(c.headers,
                "Content-Type: application/json");
        c.headers = curl_slist_append(c.headers,
                "Accept: "
                "application/json,text/plain");
        curl_easy_setopt(c.curl, CURLOPT_TIMEOUT, opt.okapi_timeout);
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

static PageStatus retrieve(const curl_wrapper& c, const ldp_options& opt,
                           const data_source& source,
                           ldp_log* lg, const string& token,
                           const table_schema& table, const string& loadDir,
                           extraction_files* ext_files, size_t page,
                           long* http_code)
{
    *http_code = 0;

    // TODO move timing code to calling function and re-enable
    //timer t(opt);

    string path = source.okapi_url;
    etymon::join(&path, table.source_spec);

    //path += "?offset=" + to_string(page * opt.pageSize) +
    //    "&limit=" + to_string(opt.pageSize) +
    //    "&query=cql.allRecords%3d1%20sortby%20id";

    string query = "?offset=" + to_string(page * opt.page_size) +
        "&limit=" + to_string(opt.page_size) +
        "&query=cql.allRecords%3d1%20sortby%20id";
    path += query;

    //path += "?offset=0&limit=1000&query=id==*%20sortby%20id";
    //if (table.source_spec.find("/erm/") == 0)
    //    path += "?stats=true&offset=0&max=100";

    string output = loadDir;
    etymon::join(&output, table.name);
    output += "_" + source.source_name;
    output += "_" + to_string(page) + ".json";

    {
        etymon::file f(output, "wb");
        ext_files->files.push_back(output);

        curl_easy_setopt(c.curl, CURLOPT_TIMEOUT, opt.okapi_timeout);

        CURLcode cc = curl_easy_setopt(c.curl, CURLOPT_URL, path.c_str());
        if (cc != CURLE_OK)
            throw runtime_error(string("Error extracting data: ") +
                                curl_easy_strerror(cc));
        cc = curl_easy_setopt(c.curl, CURLOPT_WRITEDATA, f.fp);
        if (cc != CURLE_OK)
            throw runtime_error(string("Error extracting data: ") +
                                curl_easy_strerror(cc));

        lg->write(log_level::detail, "", "", "reading: " + path, -1);

        cc = curl_easy_perform(c.curl);
        if (cc != CURLE_OK)
            throw runtime_error(string("Error extracting data: ") +
                                curl_easy_strerror(cc));
    }

    long response_code = 0;
    curl_easy_getinfo(c.curl, CURLINFO_RESPONSE_CODE, &response_code);
    *http_code = response_code;
    lg->write(log_level::detail, "", "",
              "Response code: " + table.module_name + ": " +
              table.source_spec + ": " + to_string(response_code), -1);
    if (response_code == 403 || response_code == 404 || response_code == 500) {
        return PageStatus::interfaceNotAvailable;
    }
    if (response_code != 200) {
        stringstream s;
	std::ifstream f(output);
	if (f.is_open())
	    s << f.rdbuf() << endl;
        string err = string("Error extracting data: ") +
                to_string(response_code) + ":\n" + s.str();
        // TODO read and print server response, before tmp files cleaned up
        throw runtime_error(err);
    }

    bool empty = page_is_empty(opt, output);
    return empty ? PageStatus::pageEmpty : PageStatus::containsRecords;

    // TODO move timing code to calling function and re-enable
    // Temporarily disabled until timing added for staging, merging, etc.
    //if (opt.verbose)
    //    t.print("extraction time");
}

static void writeCountFile(const data_source& source, const string& loadDir,
                           const string& tableName,
                           extraction_files* ext_files, size_t page) {
    string count_file = loadDir;
    etymon::join(&count_file, tableName);
    count_file += "_" + source.source_name;
    count_file += "_count.txt";
    etymon::file f(count_file, "w");
    ext_files->files.push_back(count_file);
    string pageStr = to_string(page) + "\n";
    fputs(pageStr.c_str(), f.fp);
}

bool retrieve_pages(const curl_wrapper& c, const ldp_options& opt,
                    const data_source& source, ldp_log* lg,
                    const string& token, const table_schema& table,
                    const string& loadDir, extraction_files* ext_files)
{
    // struct curl_data curl_config;
    if (opt.lg_level == log_level::detail) {
	// curl_easy_setopt(c.curl, CURLOPT_DEBUGFUNCTION, curl_trace);
	// curl_easy_setopt(c.curl, CURLOPT_DEBUGDATA, &curl_config);
	curl_easy_setopt(c.curl, CURLOPT_VERBOSE, 1L);
    }

    size_t page = 0;
    while (true) {
        lg->write(log_level::detail, "", "", "reading: page: " + to_string(page), -1);
        long http_code = 0;
        PageStatus status = retrieve(c, opt, source, lg, token, table, loadDir,
                                     ext_files, page, &http_code);
        switch (status) {
        case PageStatus::interfaceNotAvailable:
            lg->write(log_level::warning, "", "",
                      "Interface not available for extracting data:\n"
                      "    Table: " + table.name + "\n"
                      "    Module: " + table.module_name + "\n"
                      "    Interface: " + table.source_spec + "\n"
                      "    HTTP response: " + to_string(http_code) + "\n"
                      "    Action: Table not updated", -1);
            return false;
        case PageStatus::pageEmpty:
            writeCountFile(source, loadDir, table.name, ext_files, page);
            return true;
        case PageStatus::containsRecords:
            break;
        }
        page++;
    }
}

bool direct_override(const data_source& source, const string& tableName)
{
    for (auto& t : source.direct.table_names) {
        if (t == tableName)
            return true;
    }
    return false;
}

void pq_get_value_json_string(const PGresult *res, int row_number, int column_number, string* j)
{
    if (PQgetisnull(res, row_number, column_number)) {
        *j = "null";
    } else {
        string s;
        encode_json(PQgetvalue(res, row_number, column_number), &s);
        *j = string("\"") + s + "\"";
    }
}

void pq_get_value_json_number(const PGresult *res, int row_number, int column_number, string* j)
{
    if (PQgetisnull(res, row_number, column_number)) {
        *j = "null";
    } else {
        *j = PQgetvalue(res, row_number, column_number);
    }
}

void pq_get_value_json_boolean(const PGresult *res, int row_number, int column_number, string* j)
{
    if (PQgetisnull(res, row_number, column_number)) {
        *j = "null";
    } else {
        string s = PQgetvalue(res, row_number, column_number);
        if (s == "t") {
            *j = "true";
        } else {
            *j = "false";
        }
    }
}

void pq_get_value_json_object(const PGresult *res, int row_number, int column_number, string* j)
{
    if (PQgetisnull(res, row_number, column_number)) {
        *j = "null";
    } else {
        *j = PQgetvalue(res, row_number, column_number);
    }
}

void retrieve_direct_rmb(const PGresult *res, string* j)
{
    pq_get_value_json_object(res, 0, 1, j);
}

void retrieve_direct_srs_marc(const PGresult *res, string source_spec, string* j)
{
    string id, jsonb, description;
    pq_get_value_json_string(res, 0, 0, &id);
    pq_get_value_json_object(res, 0, 1, &jsonb);
    pq_get_value_json_string(res, 0, 2, &description);
    etymon::trim(&jsonb);
    if (jsonb.size() == 0 || jsonb[0] != '{') {
        throw runtime_error("expected '{' in JSON data: " + source_spec);
    }
    *j = "{\"id\": " + id + "," +
        "\"description\": " + description + "," +
        jsonb.substr(1, jsonb.size() - 1);
}

void retrieve_direct_srs_error(const PGresult *res, string source_spec, string* j)
{
    string id, jsonb;
    pq_get_value_json_string(res, 0, 0, &id);
    pq_get_value_json_object(res, 0, 1, &jsonb);
    etymon::trim(&jsonb);
    if (jsonb.size() == 0 || jsonb[0] != '{') {
        throw runtime_error("expected '{' in JSON data: " + source_spec);
    }
    *j = "{\"id\": " + id + "," + jsonb.substr(1, jsonb.size() - 1);
}

void retrieve_direct_srs_records(const PGresult *res, string* j)
{
    string id, snapshot_id, matched_id, generation, record_type, external_id, state, leader_record_status, order, suppress_discovery, created_by_user_id, created_date, updated_by_user_id, updated_date, external_hrid;
    pq_get_value_json_string(res, 0, 0, &id);
    pq_get_value_json_string(res, 0, 1, &snapshot_id);
    pq_get_value_json_string(res, 0, 2, &matched_id);
    pq_get_value_json_number(res, 0, 3, &generation);
    pq_get_value_json_string(res, 0, 4, &record_type);
    pq_get_value_json_string(res, 0, 5, &external_id);
    pq_get_value_json_string(res, 0, 6, &state);
    pq_get_value_json_string(res, 0, 7, &leader_record_status);
    pq_get_value_json_number(res, 0, 8, &order);
    pq_get_value_json_boolean(res, 0, 9, &suppress_discovery);
    pq_get_value_json_string(res, 0, 10, &created_by_user_id);
    pq_get_value_json_string(res, 0, 11, &created_date);
    pq_get_value_json_string(res, 0, 12, &updated_by_user_id);
    pq_get_value_json_string(res, 0, 13, &updated_date);
    pq_get_value_json_string(res, 0, 14, &external_hrid);
    *j = string("") +
        "  {\n" +
        "    \"id\": " + id + ",\n" +
        "    \"snapshotId\": " + snapshot_id + ",\n" +
        "    \"matchedId\": " + matched_id + ",\n" +
        "    \"generation\": " + generation + ",\n" +
        "    \"recordType\": " + record_type + ",\n" +
        "    \"externalId\": " + external_id + ",\n" +
        "    \"state\": " + state + ",\n" +
        "    \"leaderRecordStatus\": " + leader_record_status + ",\n" +
        "    \"order\": " + order + ",\n" +
        "    \"suppressDiscovery\": " + suppress_discovery + ",\n" +
        "    \"createdByUserId\": " + created_by_user_id + ",\n" +
        "    \"createdDate\": " + created_date + ",\n" +
        "    \"updatedByUserId\": " + updated_by_user_id + ",\n" +
        "    \"updatedDate\": " + updated_date + ",\n" +
        "    \"externalHrid\": " + external_hrid + "\n" +
        "  }";
}

void retrieve_direct_notes(const PGresult *res, string* j)
{
    string id, title, content, indexed_content, domain, type_id, pop_up_on_user, pop_up_on_check_out, created_by, created_date, updated_by, updated_date;
    pq_get_value_json_string(res, 0, 0, &id);
    pq_get_value_json_string(res, 0, 1, &title);
    pq_get_value_json_string(res, 0, 2, &content);
    pq_get_value_json_string(res, 0, 3, &indexed_content);
    pq_get_value_json_string(res, 0, 4, &domain);
    pq_get_value_json_string(res, 0, 5, &type_id);
    pq_get_value_json_boolean(res, 0, 6, &pop_up_on_user);
    pq_get_value_json_boolean(res, 0, 7, &pop_up_on_check_out);
    pq_get_value_json_string(res, 0, 8, &created_by);
    pq_get_value_json_string(res, 0, 9, &created_date);
    pq_get_value_json_string(res, 0, 10, &updated_by);
    pq_get_value_json_string(res, 0, 11, &updated_date);
    *j = string("") +
        "  {\n" +
        "    \"id\": " + id + ",\n" +
        "    \"title\": " + title + ",\n" +
        "    \"content\": " + content + ",\n" +
        "    \"indexedContent\": " + indexed_content + ",\n" +
        "    \"domain\": " + domain + ",\n" +
        "    \"typeId\": " + type_id + ",\n" +
        "    \"popUpOnUser\": " + pop_up_on_user + ",\n" +
        "    \"popUpOnCheckOut\": " + pop_up_on_check_out + ",\n" +
        "    \"createdBy\": " + created_by + ",\n" +
        "    \"createdDate\": " + created_date + ",\n" +
        "    \"updatedBy\": " + updated_by + ",\n" +
        "    \"updatedDate\": " + updated_date + "\n" +
        "  }";
}

bool try_retrieve_direct(const data_source& source, ldp_log* lg,
                     const table_schema& table, const string& loadDir,
                     extraction_files* ext_files, bool direct_extraction_no_ssl, const char* instance)
{
    if (table.direct_source_table == "") {
        lg->write(log_level::warning, "", "", "direct source table undefined: " + table.source_spec, -1);
        return false;
    }

    string attr = "'' AS id, jsonb";
    if (table.source_type == data_source_type::srs_marc_records) {
        attr = "id, content";
    }
    if (table.source_type == data_source_type::srs_error_records) {
        attr = "id, content, description";
    }
    if (table.source_type == data_source_type::srs_records) {
        attr = string("id, snapshot_id, matched_id, generation, record_type, ") + instance + "_id, state, leader_record_status, \"order\", suppress_discovery, created_by_user_id, created_date, updated_by_user_id, updated_date, " + instance + "_hrid";
    }
    if (table.source_type == data_source_type::notes) {
        attr = "id,title,content,indexed_content,domain,type_id,pop_up_on_user,pop_up_on_user,created_by,created_date,updated_by,updated_date";
    }

    // Select from table.direct_source_table and write to JSON file.
    etymon::pgconn_info dbinfo;
    dbinfo.dbhost = source.direct.database_host;
    dbinfo.dbport = source.direct.database_port;
    dbinfo.dbuser = source.direct.database_user;
    dbinfo.dbpasswd = source.direct.database_password;
    dbinfo.dbname = source.direct.database_name;
    dbinfo.dbsslmode = direct_extraction_no_ssl ? "disable" : "require";
    etymon::pgconn db(dbinfo);
    string sql = "SELECT " + attr + " FROM " + source.okapi_tenant + "_" + table.direct_source_table + ";";
    lg->write(log_level::detail, "", "", sql, -1);

    if (PQsendQuery(db.conn, sql.c_str()) == 0) {
        string err = PQerrorMessage(db.conn);
        throw runtime_error(err);
    }
    if (PQsetSingleRowMode(db.conn) == 0) {
        throw runtime_error("unable to set single-row mode in database query");
    }

    string output = loadDir;
    etymon::join(&output, table.name);
    output += "_" + source.source_name;
    output += "_0.json";
    etymon::file f(output, "w");
    ext_files->files.push_back(output);

    fprintf(f.fp, "{\n  \"a\": [\n");

    int row = 0;
    while (true) {
        etymon::pgconn_result_async res(&db);
        if (res.result == nullptr || PQresultStatus(res.result) != PGRES_SINGLE_TUPLE) {
            break;
        }
        string j;
        switch (table.source_type) {
        case data_source_type::direct_only:
        case data_source_type::rmb:
            retrieve_direct_rmb(res.result, &j);
            break;
        case data_source_type::srs_marc_records:
            retrieve_direct_srs_marc(res.result, table.source_spec, &j);
            break;
        case data_source_type::srs_error_records:
            retrieve_direct_srs_marc(res.result, table.source_spec, &j);
            break;
        case data_source_type::srs_records:
            retrieve_direct_srs_records(res.result, &j);
            break;
        case data_source_type::notes:
            retrieve_direct_notes(res.result, &j);
            break;
        default:
            throw runtime_error("internal error: unknown value for data_source_type");
        }
        if (row > 0) {
            fprintf(f.fp, ",\n");
        }
        fprintf(f.fp, "%s\n", j.data());
        row++;
    }
    lg->trace(to_string(row) + " rows written to temp file " + output);
    if (row == 0) {
        return false;
    }

    fprintf(f.fp, "\n  ]\n}\n");

    // Write 1 to count file.
    writeCountFile(source, loadDir, table.name, ext_files, 1);

    return true;
}

bool retrieve_direct(const data_source& source, ldp_log* lg,
                     table_schema* table, const string& loadDir,
                     extraction_files* ext_files, bool direct_extraction_no_ssl) {
    lg->write(log_level::trace, "", "", "direct from database: " + table->source_spec, -1);

    if (table->source_type == data_source_type::notes) {
        try {
            return try_retrieve_direct(source, lg, *table, loadDir, ext_files, direct_extraction_no_ssl, "");
        } catch (runtime_error& e) {}
        lg->write(log_level::info, "", "", "notes: falling back to note_data for compatibility", -1);
        table->source_type = data_source_type::rmb;
        table->direct_source_table = "mod_notes.note_data";
        return try_retrieve_direct(source, lg, *table, loadDir, ext_files, direct_extraction_no_ssl, "");
    }

    if (table->source_type == data_source_type::srs_records) {
        try {
            return try_retrieve_direct(source, lg, *table, loadDir, ext_files, direct_extraction_no_ssl, "external");
        } catch (runtime_error& e) {}
        lg->write(log_level::info, "", "", "srs_records: falling back to instance_id for compatibility", -1);
        return try_retrieve_direct(source, lg, *table, loadDir, ext_files, direct_extraction_no_ssl, "instance");
    }

    return try_retrieve_direct(source, lg, *table, loadDir, ext_files, direct_extraction_no_ssl, "");
}
