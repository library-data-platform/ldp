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
bool selectSchemaVersion(DBContext* db, string* version)
{
    string sql = "SELECT ldp_schema_version FROM ldp_system.main;";
    db->log->log(Level::trace, "", "", sql, -1);
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
        string e = "Table ldp_system.main contains no rows";
        db->log->log(Level::error, "", "", e, -1);
        throw runtime_error(e);
    }
    string ldpSchemaVersion;
    db->dbc->getData(&stmt, 1, &ldpSchemaVersion);
    if (db->dbc->fetch(&stmt)) {
        // This means there is more than one row.  Do not try to
        // recover automatically from this problem.
        string e = "Table ldp_system.main contains too many rows";
        db->log->log(Level::error, "", "", e, -1);
        throw runtime_error(e);
    }
    *version = ldpSchemaVersion;
    return true;
}

/**
 * \brief Initializes a new database with the LDP schema.
 *
 * This function assumes that the database is empty, or at least
 * contains no tables etc. that would conflict with the new schema
 * that is to be created.
 *
 * \param[in] db Database context.
 */
void initSchema(DBContext* db)
{
    // Create the first table and assume it does not exist.  If this
    // fails, do not try to recover automatically, because the
    // database may have a corrupted schema with unknown version
    // number.
    sql =
        "CREATE TABLE ldp_system.main (\n"
        "    ldp_schema_version BIGINT NOT NULL\n"
        ");";
    db->log->log(Level::trace, "", "", sql, -1);
    db->dbc->execDirect(nullptr, sql);
    sql = "INSERT INTO ldp_system.main (ldp_schema_version) VALUES (0);";
    db->log->log(Level::trace, "", "", sql, -1);
    db->dbc->execDirect(nullptr, sql);
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
    string version;
    selectSchemaVersion(db, &version);
    db->log->log(Level::trace, "", "", "ldp_schema_version: " + version, -1);

    if (version != "" && version != "0") {
        string e = "Unknown LDP schema version: " + version;
        db->log->log(Level::error, "", "", e, -1);
        throw runtime_error(e);
    }

    if (version == "") {
        db->log->log(Level::trace, "", "", "Initializing schema", -1);
        initSchema(db);
    }
}


