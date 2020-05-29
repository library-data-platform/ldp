#include <experimental/filesystem>
#include <sstream>
#include <stdexcept>

#include "dbtype.h"
#include "dbup1.h"
#include "init.h"
#include "log.h"
#include "util.h"

namespace fs = std::experimental::filesystem;

database_upgrade_array database_upgrades[] = {
    nullptr,  // Version 0 has no migration.
    database_upgrade_1,
    database_upgrade_2,
    database_upgrade_3,
    database_upgrade_4,
    database_upgrade_5,
    database_upgrade_6,
    database_upgrade_7,
    database_upgrade_8,
    database_upgrade_9,
    database_upgrade_10,
    database_upgrade_11
};

/* *
 * \brief Looks up the schema version number in the LDP database.
 *
 * \param[in] db Database context.
 * \param[out] version The retrieved version number.
 * \retval true The version number was retrieved.
 * \retval false The version number was not present in the database.
 */
bool select_database_version(etymon::odbc_conn* conn, int64_t* version)
{
    string sql = "SELECT ldp_schema_version FROM ldpsystem.main;";
    etymon::odbc_stmt stmt(conn);
    try {
        conn->execDirect(&stmt, sql);
    } catch (runtime_error& e) {
        // This could happen if the table does not exist.
        return false;
    }
    if (conn->fetch(&stmt) == false) {
        // This means there are no rows.  Do not try to recover
        // automatically from this problem.
        string e = "No rows could be read from table: ldpsystem.main";
        throw runtime_error(e);
    }
    string ldpSchemaVersion;
    conn->getData(&stmt, 1, &ldpSchemaVersion);
    if (conn->fetch(&stmt)) {
        // This means there is more than one row.  Do not try to
        // recover automatically from this problem.
        string e = "Too many rows in table: ldpsystem.main";
        throw runtime_error(e);
    }
    //*version = stol(ldpSchemaVersion);
    {
        stringstream stream(ldpSchemaVersion);
        stream >> *version;
    }
    return true;
}

void catalog_add_table(etymon::odbc_conn* conn, const string& table)
{
    string sql =
        "INSERT INTO ldpsystem.tables (table_name) VALUES\n"
        "    ('" + table + "');";
    conn->execDirect(nullptr, sql);
}

/* *
 * \brief Initializes a new database with the LDP schema.
 *
 * This function assumes that the database is empty, or at least
 * contains no tables etc. that would conflict with the new schema
 * that is to be created.  In case of conflicts, this function will
 * throw an exception rather than continue by making assumptions about
 * the state or version of the database schema.
 *
 * \param[in] db Database context.
 */
void init_database(etymon::odbc_conn* conn, const string& ldpUser,
        const string& ldpconfigUser, int64_t thisSchemaVersion, FILE* err,
        const char* prog)
{
    fprintf(err, "%s: Initializing database\n", prog);

    DBType dbt(conn);

    // TODO This should probably be passed into the function as a parameter.
    Schema schema;
    Schema::MakeDefaultSchema(&schema);

    etymon::odbc_tx tx(conn);

    // Schema: ldpsystem

    //string sql = "CREATE SCHEMA ldpsystem;";
    //db->conn->execDirect(nullptr, sql);

    string sql = "GRANT USAGE ON SCHEMA ldpsystem TO " + ldpUser + ";";
    conn->execDirect(nullptr, sql);
    sql = "GRANT USAGE ON SCHEMA ldpsystem TO " + ldpconfigUser + ";";
    conn->execDirect(nullptr, sql);

    sql =
        "CREATE TABLE ldpsystem.main (\n"
        "    ldp_schema_version BIGINT NOT NULL\n"
        ");";
    conn->execDirect(nullptr, sql);
    sql = "INSERT INTO ldpsystem.main (ldp_schema_version) VALUES (" +
        to_string(thisSchemaVersion) + ");";
    conn->execDirect(nullptr, sql);

    sql =
        "CREATE TABLE ldpsystem.log (\n"
        "    log_time TIMESTAMP WITH TIME ZONE NOT NULL,\n"
        "    pid BIGINT NOT NULL,\n"
        "    level VARCHAR(7) NOT NULL DEFAULT '',\n"
        "    type VARCHAR(63) NOT NULL,\n"
        "    table_name VARCHAR(63) NOT NULL,\n"
        "    message VARCHAR(65535) NOT NULL,\n"
        "    elapsed_time REAL\n"
        ");";
    conn->execDirect(nullptr, sql);

    // Table: ldpsystem.tables

    sql =
        "CREATE TABLE ldpsystem.tables (\n"
        "    table_name VARCHAR(63) NOT NULL,\n"
        "    updated TIMESTAMP WITH TIME ZONE,\n"
        "    row_count BIGINT,\n"
        "    history_row_count BIGINT,\n"
        "    documentation VARCHAR(65535),\n"
        "    documentation_url VARCHAR(65535)\n"
        ");";
    conn->execDirect(nullptr, sql);

    // Add tables to the catalog.
    for (auto& table : schema.tables)
        catalog_add_table(conn, table.tableName);

    string rskeys;
    dbt.redshiftKeys("referencing_table",
            "referencing_table, referencing_column", &rskeys);
    sql =
        "CREATE TABLE ldpsystem.foreign_key_constraints (\n"
        "    referencing_table VARCHAR(63) NOT NULL,\n"
        "    referencing_column VARCHAR(63) NOT NULL,\n"
        "    referenced_table VARCHAR(63) NOT NULL,\n"
        "    referenced_column VARCHAR(63) NOT NULL,\n"
        "    constraint_name VARCHAR(63) NOT NULL,\n"
        "        PRIMARY KEY (referencing_table, referencing_column)\n"
        ")" + rskeys + ";";
    conn->execDirect(nullptr, sql);

    //sql = "GRANT SELECT ON ALL TABLES IN SCHEMA ldpsystem TO " + ldpUser + ";";
    //conn->execDirect(nullptr, sql);
    //sql = "GRANT SELECT ON ALL TABLES IN SCHEMA ldpsystem TO " + ldpconfigUser +
    //    ";";
    //conn->execDirect(nullptr, sql);


    sql = "GRANT SELECT ON ldpsystem.log TO " + ldpUser + ";";
    conn->execDirect(nullptr, sql);
    sql = "GRANT SELECT ON ldpsystem.log TO " + ldpconfigUser + ";";
    conn->execDirect(nullptr, sql);

    sql = "GRANT SELECT ON ldpsystem.main TO " + ldpUser + ";";
    conn->execDirect(nullptr, sql);
    sql = "GRANT SELECT ON ldpsystem.main TO " + ldpconfigUser + ";";
    conn->execDirect(nullptr, sql);

    sql = "GRANT SELECT ON ldpsystem.foreign_key_constraints TO " + ldpUser +
        ";";
    conn->execDirect(nullptr, sql);
    sql = "GRANT SELECT ON ldpsystem.foreign_key_constraints TO " +
        ldpconfigUser + ";";
    conn->execDirect(nullptr, sql);

    sql = "GRANT SELECT ON ldpsystem.tables TO " + ldpUser + ";";
    conn->execDirect(nullptr, sql);
    sql = "GRANT SELECT ON ldpsystem.tables TO " + ldpconfigUser + ";";
    conn->execDirect(nullptr, sql);

    // Schema: ldpconfig

    sql = "CREATE SCHEMA ldpconfig;";
    conn->execDirect(nullptr, sql);

    sql = "GRANT USAGE ON SCHEMA ldpconfig TO " + ldpUser + ";";
    conn->execDirect(nullptr, sql);
    sql = "GRANT USAGE ON SCHEMA ldpconfig TO " + ldpconfigUser + ";";
    conn->execDirect(nullptr, sql);

    sql =
        "CREATE TABLE ldpconfig.general (\n"
        "    enable_full_updates BOOLEAN NOT NULL,\n"
        "    next_full_update TIMESTAMP WITH TIME ZONE NOT NULL,\n"
        "    detect_foreign_keys BOOLEAN NOT NULL DEFAULT FALSE,\n"
        "    force_foreign_key_constraints BOOLEAN NOT NULL DEFAULT FALSE,\n"
        "    enable_foreign_key_warnings BOOLEAN NOT NULL DEFAULT FALSE,\n"
        "    disable_anonymization BOOLEAN NOT NULL DEFAULT FALSE\n"
        ");";
    conn->execDirect(nullptr, sql);
    sql = "DELETE FROM ldpconfig.general;";  // Temporary: pre-LDP-1.0
    conn->execDirect(nullptr, sql);
    sql =
        "INSERT INTO ldpconfig.general\n"
        "    (enable_full_updates, next_full_update)\n"
        "    VALUES\n"
        "    (TRUE, " + string(dbt.currentTimestamp()) + ");";
    conn->execDirect(nullptr, sql);

    sql =
        "CREATE TABLE ldpconfig.foreign_keys (\n"
        "    enable_constraint BOOLEAN NOT NULL,\n"
        "    referencing_table VARCHAR(63) NOT NULL,\n"
        "    referencing_column VARCHAR(63) NOT NULL,\n"
        "    referenced_table VARCHAR(63) NOT NULL,\n"
        "    referenced_column VARCHAR(63) NOT NULL\n"
        ");";
    conn->execDirect(nullptr, sql);

    sql = "GRANT SELECT ON ALL TABLES IN SCHEMA ldpconfig TO " + ldpUser + ";";
    conn->execDirect(nullptr, sql);
    sql = "GRANT SELECT ON ALL TABLES IN SCHEMA ldpconfig TO " + ldpconfigUser +
        ";";
    conn->execDirect(nullptr, sql);
    sql = "GRANT UPDATE ON ldpconfig.general TO " + ldpconfigUser + ";";
    conn->execDirect(nullptr, sql);

    // Schema: history

    sql = "CREATE SCHEMA history;";
    conn->execDirect(nullptr, sql);

    sql = "GRANT USAGE ON SCHEMA history TO " + ldpUser + ";";
    conn->execDirect(nullptr, sql);
    sql = "GRANT USAGE ON SCHEMA history TO " + ldpconfigUser + ";";
    conn->execDirect(nullptr, sql);

    for (auto& table : schema.tables) {
        string rskeys;
        dbt.redshiftKeys("sk", "sk, updated", &rskeys);
        string sql =
            "CREATE TABLE IF NOT EXISTS\n"
            "    history." + table.tableName + " (\n"
            "    id VARCHAR(36) NOT NULL,\n"
            "    data " + dbt.jsonType() + " NOT NULL,\n"
            "    updated TIMESTAMP WITH TIME ZONE NOT NULL,\n"
            "    tenant_id SMALLINT NOT NULL,\n"
            "    CONSTRAINT\n"
            "        history_" + table.tableName + "_pkey\n"
            "        PRIMARY KEY (id, updated)\n"
            ")" + rskeys + ";";
        conn->execDirect(nullptr, sql);

        sql =
            "GRANT SELECT ON\n"
            "    history." + table.tableName + "\n"
            "    TO " + ldpUser + ";";
        conn->execDirect(nullptr, sql);
        sql =
            "GRANT SELECT ON\n"
            "    history." + table.tableName + "\n"
            "    TO " + ldpconfigUser + ";";
        conn->execDirect(nullptr, sql);
    }

    // Schema: public

    for (auto& table : schema.tables) {
        string rskeys;
        dbt.redshiftKeys("sk", "sk", &rskeys);
        sql =
            "CREATE TABLE " + table.tableName + " (\n"
            "    sk BIGINT NOT NULL,\n"
            "    id VARCHAR(65535) NOT NULL,\n"
            "    data " + dbt.jsonType() + ",\n"
            "    tenant_id SMALLINT NOT NULL,\n"
            "    PRIMARY KEY (sk),\n"
            "    UNIQUE (id)\n"
        ")" + rskeys + ";";
        conn->execDirect(nullptr, sql);
        sql =
            "GRANT SELECT ON " + table.tableName + "\n"
            "    TO " + ldpconfigUser + ";";
        conn->execDirect(nullptr, sql);
        sql =
            "GRANT SELECT ON " + table.tableName + "\n"
            "    TO " + ldpUser + ";";
        conn->execDirect(nullptr, sql);
    }

    // Schema: local

    sql = "CREATE SCHEMA local;";
    conn->execDirect(nullptr, sql);

    sql = "GRANT CREATE, USAGE ON SCHEMA local TO " + ldpUser + ";";
    conn->execDirect(nullptr, sql);
    sql = "GRANT USAGE ON SCHEMA local TO " + ldpconfigUser + ";";
    conn->execDirect(nullptr, sql);

    tx.commit();
}

void upgrade_database(etymon::odbc_conn* conn, const string& ldpUser,
        const string& ldpconfigUser, int64_t version,
        int64_t this_schema_version, const string& datadir, FILE* err,
        const char* prog)
{
    if (version < 0 || version > this_schema_version)
        throw runtime_error(
                "Unknown LDP database version: " + to_string(version));

    fs::path log_path = fs::path(datadir) / "log";
    fs::create_directories(log_path);
    fs::path ulog_path = log_path / "database_upgrade.sql";
    etymon::File ulogFile(ulog_path, "a");

    bool upgraded = false;
    for (int64_t v = version + 1; v <= this_schema_version; v++) {
        if (!upgraded) {
            fprintf(err, "%s: ", prog);
            print_banner_line(err, '=', 74);
            fprintf(err,
                    "%s: Upgrading database: "
                    "Do not interrupt the upgrade process\n",
                    prog);
            fprintf(err, "%s: ", prog);
            print_banner_line(err, '=', 74);
        }
        fprintf(err, "%s: Upgrading: %s\n", prog, to_string(v).c_str());
        print_banner_line(ulogFile.file, '-', 79);
        fprintf(ulogFile.file, "-- Upgrading: %s\n", to_string(v).c_str());
        print_banner_line(ulogFile.file, '-', 79);
        database_upgrade_options opt;
        opt.ulog = ulogFile.file;
        opt.conn = conn;
        opt.ldp_user = ldpUser;
        opt.ldpconfig_user = ldpconfigUser;
        opt.datadir = datadir;
        database_upgrades[v](&opt);
        upgraded = true;
        print_banner_line(ulogFile.file, '-', 79);
        fprintf(ulogFile.file, "-- Completed upgrade: %s\n",
                to_string(v).c_str());
        print_banner_line(ulogFile.file, '-', 79);
        fputc('\n', ulogFile.file);
    }

    if (upgraded) {
        fprintf(err, "%s: ", prog);
        print_banner_line(err, '=', 74);
        fprintf(err, "%s: Database upgrade completed\n", prog);
        fprintf(err, "%s: ", prog);
        print_banner_line(err, '=', 74);
        Log lg(conn, Level::info, false, prog);
        lg.log(Level::info, "", "", "Upgraded to database version: " +
                to_string(this_schema_version), -1);
    } else {
        fprintf(err, "%s: Database version is up to date\n", prog);
    }
}

/* *
 * \brief Initializes or upgrades an LDP database if needed.
 *
 * This function checks if the database has been previously
 * initialized with an LDP schema.  If not, it creates the schema.  If
 * the schema exists but its version number is not current, this
 * function attempts to upgrade the schema to the current version.
 *
 * If an unexpected state in the database is detected, this function
 * will throw an exception rather than continue by making assumptions
 * about the state or version of the database schema.  However, it
 * does not perform any thorough validation of the database, and it
 * assumes that the schema has not been altered or corrupted.
 *
 * \param[in] odbc ODBC environment.
 * \param[in] db Database context.
 */
void init_upgrade(etymon::odbc_env* odbc, const string& dbname,
        const string& ldpUser, const string& ldpconfigUser,
        const string& datadir, FILE* err, const char* prog)
{
    int64_t this_schema_version = 11;

    etymon::odbc_conn conn(odbc, dbname);

    int64_t version;
    bool version_found = select_database_version(&conn, &version);

    if (version_found)
        // Schema is present: check if it needs to be upgraded.
        upgrade_database(&conn, ldpUser, ldpconfigUser, version,
                this_schema_version, datadir, err, prog);
    else
        // Schema is not present: create it.
        init_database(&conn, ldpUser, ldpconfigUser, this_schema_version, err,
                prog);
}

