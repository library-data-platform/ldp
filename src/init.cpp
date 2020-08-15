#include <experimental/filesystem>
#include <sstream>
#include <stdexcept>

#include "dbtype.h"
#include "dbup1.h"
#include "init.h"
#include "initutil.h"
#include "log.h"
#include "schema.h"
#include "util.h"

namespace fs = std::experimental::filesystem;

static int64_t ldp_latest_database_version = 18;

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
    database_upgrade_11,
    database_upgrade_12,
    database_upgrade_13,
    database_upgrade_14,
    database_upgrade_15,
    database_upgrade_16,
    database_upgrade_17,
    database_upgrade_18
};

int64_t latest_database_version()
{
    return ldp_latest_database_version;
}

/* *
 * \brief Looks up the schema version number in the LDP database.
 *
 * \param[in] db Database context.
 * \param[out] version The retrieved version number.
 * \retval true The version number was retrieved.
 * \retval false The version number was not present in the database.
 */
bool select_database_version_ldpsystem(etymon::odbc_conn* conn,
                                       int64_t* version)
{
    string sql = "SELECT ldp_schema_version FROM ldpsystem.main;";
    etymon::odbc_stmt stmt(conn);
    try {
        conn->exec_direct(&stmt, sql);
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
    string ldp_schema_version;
    conn->get_data(&stmt, 1, &ldp_schema_version);
    if (conn->fetch(&stmt)) {
        // This means there is more than one row.  Do not try to
        // recover automatically from this problem.
        string e = "Too many rows in table: ldpsystem.main";
        throw runtime_error(e);
    }
    //*version = stol(ldp_schema_version);
    {
        stringstream stream(ldp_schema_version);
        stream >> *version;
    }
    return true;
}

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
    string sql = "SELECT database_version FROM dbsystem.main;";
    etymon::odbc_stmt stmt(conn);
    try {
        conn->exec_direct(&stmt, sql);
    } catch (runtime_error& e) {
        // This could happen if the table does not exist.
        return select_database_version_ldpsystem(conn, version);
    }
    if (conn->fetch(&stmt) == false) {
        // This means there are no rows.  Do not try to recover
        // automatically from this problem.
        string e = "No rows could be read from table: dbsystem.main";
        throw runtime_error(e);
    }
    string database_version;
    conn->get_data(&stmt, 1, &database_version);
    if (conn->fetch(&stmt)) {
        // This means there is more than one row.  Do not try to
        // recover automatically from this problem.
        string e = "Too many rows in table: dbsystem.main";
        throw runtime_error(e);
    }
    //*version = stol(database_version);
    {
        stringstream stream(database_version);
        stream >> *version;
    }
    return true;
}

void validate_database_version(int64_t database_version)
{
    int64_t latest_version = latest_database_version();
    if (database_version < 0 || database_version > latest_version)
        throw runtime_error(
                "Unknown LDP database version: " + to_string(database_version));
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
static void init_database_all(etymon::odbc_conn* conn, const string& ldp_user,
        const string& ldpconfig_user, int64_t this_schema_version, FILE* err,
        const char* prog)
{
    fprintf(err, "%s: Initializing database\n", prog);

    dbtype dbt(conn);

    // TODO This should probably be passed into the function as a parameter.
    ldp_schema schema;
    ldp_schema::make_default_schema(&schema);

    etymon::odbc_tx tx(conn);

    // Schema: dbsystem

    string sql = "CREATE SCHEMA dbsystem;";
    conn->exec(sql);
    
    sql = "GRANT USAGE ON SCHEMA dbsystem TO " + ldp_user + ";";
    conn->exec(sql);
    sql = "GRANT USAGE ON SCHEMA dbsystem TO " + ldpconfig_user + ";";
    conn->exec(sql);

    sql =
        "CREATE TABLE dbsystem.main (\n"
        "    database_version BIGINT NOT NULL\n"
        ");";
    conn->exec(sql);
    sql = "INSERT INTO dbsystem.main (database_version) VALUES (" +
        to_string(this_schema_version) + ");";
    conn->exec(sql);

    sql =
        "CREATE TABLE dbsystem.log (\n"
        "    log_time TIMESTAMP WITH TIME ZONE NOT NULL,\n"
        "    pid BIGINT NOT NULL,\n"
        "    level VARCHAR(7) NOT NULL DEFAULT '',\n"
        "    type VARCHAR(63) NOT NULL,\n"
        "    table_name VARCHAR(63) NOT NULL,\n"
        "    message VARCHAR(65535) NOT NULL,\n"
        "    elapsed_time REAL\n"
        ");";
    conn->exec(sql);

    // Table: dbsystem.tables

    sql =
        "CREATE TABLE dbsystem.tables (\n"
        "    table_name VARCHAR(63) NOT NULL,\n"
        "    updated TIMESTAMP WITH TIME ZONE,\n"
        "    row_count BIGINT,\n"
        "    history_row_count BIGINT,\n"
        "    documentation VARCHAR(65535),\n"
        "    documentation_url VARCHAR(65535)\n"
        ");";
    conn->exec(sql);
    // Add tables to the catalog.
    for (auto& table : schema.tables) {
        add_table_to_catalog_sql(conn, table.name, &sql);
        conn->exec(sql);
    }

    string rskeys;
    dbt.redshift_keys("referencing_table",
            "referencing_table, referencing_column", &rskeys);
    sql =
        "CREATE TABLE dbsystem.suggested_foreign_keys (\n"
        "    enable_constraint BOOLEAN NOT NULL,\n"
        "    referencing_table VARCHAR(63) NOT NULL,\n"
        "    referencing_column VARCHAR(63) NOT NULL,\n"
        "    referenced_table VARCHAR(63) NOT NULL,\n"
        "    referenced_column VARCHAR(63) NOT NULL\n"
        ")" + rskeys + ";";
    conn->exec(sql);

    dbt.redshift_keys("referencing_table",
            "referencing_table, referencing_column", &rskeys);
    sql =
        "CREATE TABLE dbsystem.foreign_key_constraints (\n"
        "    referencing_table VARCHAR(63) NOT NULL,\n"
        "    referencing_column VARCHAR(63) NOT NULL,\n"
        "    referenced_table VARCHAR(63) NOT NULL,\n"
        "    referenced_column VARCHAR(63) NOT NULL,\n"
        "    constraint_name VARCHAR(63) NOT NULL,\n"
        "        PRIMARY KEY (referencing_table, referencing_column)\n"
        ")" + rskeys + ";";
    conn->exec(sql);

    //sql = "GRANT SELECT ON ALL TABLES IN SCHEMA dbsystem TO " + ldp_user + ";";
    //conn->exec(sql);
    //sql = "GRANT SELECT ON ALL TABLES IN SCHEMA dbsystem TO " +
    //ldpconfig_user +
    //    ";";
    //conn->exec(sql);


    sql = "GRANT SELECT ON dbsystem.log TO " + ldp_user + ";";
    conn->exec(sql);
    sql = "GRANT SELECT ON dbsystem.log TO " + ldpconfig_user + ";";
    conn->exec(sql);

    sql = "GRANT SELECT ON dbsystem.main TO " + ldp_user + ";";
    conn->exec(sql);
    sql = "GRANT SELECT ON dbsystem.main TO " + ldpconfig_user + ";";
    conn->exec(sql);

    sql = "GRANT SELECT ON dbsystem.foreign_key_constraints TO " + ldp_user +
        ";";
    conn->exec(sql);
    sql = "GRANT SELECT ON dbsystem.foreign_key_constraints TO " +
        ldpconfig_user + ";";
    conn->exec(sql);

    sql = "GRANT SELECT ON dbsystem.tables TO " + ldp_user + ";";
    conn->exec(sql);
    sql = "GRANT SELECT ON dbsystem.tables TO " + ldpconfig_user + ";";
    conn->exec(sql);

    // Schema: dbconfig

    sql = "CREATE SCHEMA dbconfig;";
    conn->exec(sql);

    sql = "GRANT USAGE ON SCHEMA dbconfig TO " + ldp_user + ";";
    conn->exec(sql);
    sql = "GRANT USAGE ON SCHEMA dbconfig TO " + ldpconfig_user + ";";
    conn->exec(sql);

    sql =
        "CREATE TABLE dbconfig.general (\n"
        "    update_all_tables BOOLEAN NOT NULL DEFAULT FALSE,\n"
        "    enable_full_updates BOOLEAN NOT NULL,\n"
        "    next_full_update TIMESTAMP WITH TIME ZONE NOT NULL,\n"
        "    detect_foreign_keys BOOLEAN NOT NULL DEFAULT FALSE,\n"
        "    enable_foreign_key_warnings BOOLEAN NOT NULL DEFAULT FALSE,\n"
        "    force_foreign_key_constraints BOOLEAN NOT NULL DEFAULT FALSE,\n"
        "    disable_anonymization BOOLEAN NOT NULL DEFAULT FALSE\n"
        ");";
    conn->exec(sql);
    sql =
        "INSERT INTO dbconfig.general\n"
        "    (enable_full_updates, next_full_update)\n"
        "    VALUES\n"
        "    (TRUE, " + string(dbt.current_timestamp()) + ");";
    conn->exec(sql);

    // dbconfig.update_tables
    sql =
        "CREATE TABLE dbconfig.update_tables (\n"
        "    enable_update BOOLEAN NOT NULL,\n"
        "    table_name VARCHAR(63) NOT NULL,\n"
        "    tenant_id SMALLINT NOT NULL\n"
        ");";
    conn->exec(sql);

    sql =
        "CREATE TABLE dbconfig.foreign_keys (\n"
        "    enable_constraint BOOLEAN NOT NULL,\n"
        "    referencing_table VARCHAR(63) NOT NULL,\n"
        "    referencing_column VARCHAR(63) NOT NULL,\n"
        "    referenced_table VARCHAR(63) NOT NULL,\n"
        "    referenced_column VARCHAR(63) NOT NULL\n"
        ");";
    conn->exec(sql);

    sql = "GRANT SELECT ON ALL TABLES IN SCHEMA dbconfig TO " + ldp_user + ";";
    conn->exec(sql);
    sql = "GRANT SELECT ON ALL TABLES IN SCHEMA dbconfig TO " + ldpconfig_user +
        ";";
    conn->exec(sql);
    sql = "GRANT UPDATE ON dbconfig.general TO " + ldpconfig_user + ";";
    conn->exec(sql);

    // Schema: history

    sql = "CREATE SCHEMA history;";
    conn->exec(sql);

    sql = "GRANT USAGE ON SCHEMA history TO " + ldp_user + ";";
    conn->exec(sql);
    sql = "GRANT USAGE ON SCHEMA history TO " + ldpconfig_user + ";";
    conn->exec(sql);

    for (auto& table : schema.tables) {
        create_history_table_sql(table.name, conn, dbt, &sql);
        conn->exec(sql);
        grant_select_on_table_sql("history." + table.name, ldp_user,
                                  conn, &sql);
        conn->exec(sql);
        grant_select_on_table_sql("history." + table.name, ldpconfig_user,
                                  conn, &sql);
        conn->exec(sql);
    }

    // Schema: public

    for (auto& table : schema.tables) {
        create_main_table_sql(table.name, conn, dbt, &sql);
        conn->exec(sql);
        grant_select_on_table_sql(table.name, ldp_user, conn, &sql);
        conn->exec(sql);
        grant_select_on_table_sql(table.name, ldpconfig_user, conn, &sql);
        conn->exec(sql);
    }

    // Schema: local

    sql = "CREATE SCHEMA local;";
    conn->exec(sql);

    sql = "GRANT CREATE, USAGE ON SCHEMA local TO " + ldp_user + ";";
    conn->exec(sql);
    sql = "GRANT USAGE ON SCHEMA local TO " + ldpconfig_user + ";";
    conn->exec(sql);

    tx.commit();

    fprintf(err, "%s: Database initialization completed\n", prog);
}

static void upgrade_database_all(etymon::odbc_conn* conn, const string& ldp_user,
        const string& ldpconfig_user, int64_t version,
        int64_t latest_version, const string& datadir, FILE* err,
        const char* prog, bool quiet)
{
    fs::path ulog_dir = fs::path(datadir) / "database_upgrade";
    fs::create_directories(ulog_dir);

    bool upgraded = false;
    for (int64_t v = version + 1; v <= latest_version; v++) {
        string ulog_filename = "upgrade_" + to_string(v) + ".sql";
        fs::path ulog_path = ulog_dir / ulog_filename;
        etymon::file ulog_file(ulog_path, "a");
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
        print_banner_line(ulog_file.fp, '-', 79);
        fprintf(ulog_file.fp, "-- Upgrading: %s\n", to_string(v).c_str());
        print_banner_line(ulog_file.fp, '-', 79);
        database_upgrade_options opt;
        opt.ulog = ulog_file.fp;
        opt.conn = conn;
        opt.ldp_user = ldp_user;
        opt.ldpconfig_user = ldpconfig_user;
        opt.datadir = datadir;
        database_upgrades[v](&opt);
        print_banner_line(ulog_file.fp, '-', 79);
        fprintf(ulog_file.fp, "-- Completed upgrade: %s\n",
                to_string(v).c_str());
        print_banner_line(ulog_file.fp, '-', 79);
        fputc('\n', ulog_file.fp);
        upgraded = true;
    }

    if (upgraded) {
        fprintf(err, "%s: ", prog);
        print_banner_line(err, '=', 74);
        fprintf(err, "%s: Database upgrade completed\n", prog);
        fprintf(err, "%s: ", prog);
        print_banner_line(err, '=', 74);
        ldp_log lg(conn, log_level::info, false, quiet, prog);
        lg.write(log_level::info, "server", "", "Upgraded to database version: " +
                to_string(latest_version), -1);
    } else {
        if (!quiet)
            fprintf(err, "%s: Database version is up to date\n", prog);
    }
}

void init_database(etymon::odbc_env* odbc, const string& dbname,
        const string& ldp_user, const string& ldpconfig_user, FILE* err,
        const char* prog)
{
    int64_t latest_version = latest_database_version();

    etymon::odbc_conn conn(odbc, dbname);

    int64_t database_version;
    bool version_found = select_database_version(&conn, &database_version);
    if (version_found) {
        validate_database_version(database_version);
        throw runtime_error("Database may have been previously initialized");
    } else {
        init_database_all(&conn, ldp_user, ldpconfig_user, latest_version, err,
                prog);
    }
}

void upgrade_database(etymon::odbc_env* odbc, const string& dbname,
        const string& ldp_user, const string& ldpconfig_user,
        const string& datadir, FILE* err, const char* prog, bool quiet)
{
    int64_t latest_version = latest_database_version();

    etymon::odbc_conn conn(odbc, dbname);

    int64_t database_version;
    bool version_found = select_database_version(&conn, &database_version);
    if (version_found) {
        // Database appears to be initialized: check if it should be upgraded.
        validate_database_version(database_version);
        if (database_version < latest_version) {
            upgrade_database_all(&conn, ldp_user, ldpconfig_user,
                    database_version, latest_version, datadir, err, prog,
                    quiet);
        } else {
            if (!quiet)
                fprintf(err, "%s: Database version is up to date\n", prog);
        }
    } else {
        throw runtime_error("Database has not been initialized");
    }
}

void validate_database_latest_version(etymon::odbc_env* odbc,
        const string& dbname)
{
    int64_t latest_version = latest_database_version();

    etymon::odbc_conn conn(odbc, dbname);

    int64_t database_version;
    bool version_found = select_database_version(&conn, &database_version);
    if (version_found) {
        // Database appears to be initialized: confirm version up to date.
        validate_database_version(database_version);
        if (database_version != latest_version) {
            throw runtime_error(
                    "Database should be upgraded using the "
                    "upgrade-database command");
        }
    } else {
        throw runtime_error("Database has not been initialized");
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

