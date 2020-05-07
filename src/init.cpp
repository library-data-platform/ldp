#include <stdexcept>

#include "init.h"
#include "log.h"

/**
 * \brief Looks up the schema version number in the LDP database.
 *
 * \param[in] db Database context.
 * \param[out] version The retrieved version number.
 * \retval true The version number was retrieved.
 * \retval false The version number was not present in the database.
 */
bool selectSchemaVersion(DBContext* db, int64_t* version)
{
    string sql = "SELECT ldp_schema_version FROM ldpsystem.main;";
    db->log->log(Level::detail, "", "", sql, -1);
    etymon::OdbcStmt stmt(db->dbc);
    try {
        db->dbc->execDirect(&stmt, sql);
    } catch (runtime_error& e) {
        // This could happen if the table does not exist.
        return false;
    }
    if (db->dbc->fetch(&stmt) == false) {
        // This means there are no rows.  Do not try to recover
        // automatically from this problem.
        string e = "No rows could be read from table: ldpsystem.main";
        db->log->log(Level::error, "", "", e, -1);
        throw runtime_error(e);
    }
    string ldpSchemaVersion;
    db->dbc->getData(&stmt, 1, &ldpSchemaVersion);
    if (db->dbc->fetch(&stmt)) {
        // This means there is more than one row.  Do not try to
        // recover automatically from this problem.
        string e = "Too many rows in table: ldpsystem.main";
        db->log->log(Level::error, "", "", e, -1);
        throw runtime_error(e);
    }
    *version = stol(ldpSchemaVersion);
    return true;
}

/**
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
void initSchema(DBContext* db)
{
    // Create the schema.  Beginning around the time of LDP 1.0, "IF
    // NOT EXISTS" will no longer be used because we will want to
    // throw an exception in such cases.

    string sql = "CREATE SCHEMA IF NOT EXISTS ldpsystem;";
    db->log->log(Level::detail, "", "", sql, -1);
    db->dbc->execDirect(nullptr, sql);

    sql =
        "CREATE TABLE IF NOT EXISTS ldpsystem.main (\n"
        "    ldp_schema_version BIGINT NOT NULL\n"
        ");";
    db->log->log(Level::detail, "", "", sql, -1);
    db->dbc->execDirect(nullptr, sql);
    sql = "DELETE FROM ldpsystem.main;";  // Temporary: pre-LDP-1.0
    db->log->log(Level::detail, "", "", sql, -1);
    db->dbc->execDirect(nullptr, sql);
    sql = "INSERT INTO ldpsystem.main (ldp_schema_version) VALUES (0);";
    db->log->log(Level::detail, "", "", sql, -1);
    db->dbc->execDirect(nullptr, sql);

    sql =
        "CREATE TABLE IF NOT EXISTS ldpsystem.log (\n"
        "    log_time TIMESTAMPTZ NOT NULL,\n"
        "    pid BIGINT NOT NULL,\n"
        "    level VARCHAR(6) NOT NULL,\n"
        "    type VARCHAR(63) NOT NULL,\n"
        "    table_name VARCHAR(63) NOT NULL,\n"
        "    message VARCHAR(65535) NOT NULL,\n"
        "    elapsed_time REAL\n"
        ");";
    db->dbc->execDirect(nullptr, sql);
    db->log->log(Level::detail, "", "", sql, -1);

    sql = "CREATE SCHEMA IF NOT EXISTS ldpconfig;";
    db->log->log(Level::detail, "", "", sql, -1);
    db->dbc->execDirect(nullptr, sql);

    sql =
        "CREATE TABLE IF NOT EXISTS ldpconfig.general (\n"
        "    full_update_enabled BOOLEAN NOT NULL,\n"
        "    next_full_update TIMESTAMPTZ NOT NULL\n"
        ");";
    db->dbc->execDirect(nullptr, sql);
    db->log->log(Level::detail, "", "", sql, -1);
    sql = "DELETE FROM ldpconfig.general;";  // Temporary: pre-LDP-1.0
    db->log->log(Level::detail, "", "", sql, -1);
    db->dbc->execDirect(nullptr, sql);
    sql =
        "INSERT INTO ldpconfig.general\n"
        "    (full_update_enabled, next_full_update)\n"
        "    VALUES\n"
        "    (TRUE, " + string(db->dbt->currentTimestamp()) + ");";
    db->log->log(Level::detail, "", "", sql, -1);
    db->dbc->execDirect(nullptr, sql);

    sql = "CREATE SCHEMA IF NOT EXISTS history;";
    db->log->log(Level::detail, "", "", sql, -1);
    db->dbc->execDirect(nullptr, sql);

    sql = "CREATE SCHEMA IF NOT EXISTS local;";
    db->log->log(Level::detail, "", "", sql, -1);
    db->dbc->execDirect(nullptr, sql);
}

void bootstrapSchema(DBContext* db)
{
    string rskeys;
    db->dbt->redshiftKeys("sk", "sk", &rskeys);
    string autoInc;
    db->dbt->autoIncrementType(1, false, "", &autoInc);
    string sql =
        "CREATE TABLE IF NOT EXISTS ldpsystem.idmap (\n"
        "    sk " + autoInc + ",\n"
        "    id VARCHAR(65535),\n"
        "        PRIMARY KEY (sk),\n"
        "        UNIQUE (id)\n"
        ")" + rskeys + ";";
    db->dbc->execDirect(nullptr, sql);
    db->log->log(Level::detail, "", "", sql, -1);
}

/**
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
 * \param[in] db Database context.
 */
void initUpgrade(DBContext* db)
{
    db->log->log(Level::trace, "", "", "Initializing database", -1);

    int64_t version;
    bool versionFound = selectSchemaVersion(db, &version);
    db->log->log(Level::detail, "", "", "ldp_schema_version: " +
            (versionFound ? to_string(version) : "(not found)"),
            -1);

    if (versionFound && version != 0) {
        string e = "Unknown LDP schema version: " + to_string(version);
        db->log->log(Level::error, "", "", e, -1);
        throw runtime_error(e);
    }

    if (!versionFound) {
        db->log->log(Level::detail, "", "", "Creating schema", -1);
        {
            etymon::OdbcTx tx(db->dbc);
            initSchema(db);
            tx.commit();
            fprintf(stderr, "ldp: Logging enabled in table: ldpsystem.log\n");
        }
    }

    bootstrapSchema(db);
}

