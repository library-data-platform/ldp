#include <stdexcept>

#include "init.h"
#include "log.h"

static void selectSchemaVersion(DBContext* db, string* version)
{
    string sql = "SELECT ldp_schema_version FROM ldp_system.main;";
    db->log->log(Level::trace, "", "", sql, -1);
    etymon::OdbcStmt stmt(db->dbc);
    try {
        db->dbc->execDirect(&stmt, sql);
    } catch (runtime_error& e) {
        *version = "";
        return;
    }
    if (db->dbc->fetch(&stmt) == false) {
        *version = "";
        return;
    }
    string ldpSchemaVersion;
    db->dbc->getData(&stmt, 1, &ldpSchemaVersion);
    if (db->dbc->fetch(&stmt)) {
        string e = "Table ldp_system.main contains too many rows";
        db->log->log(Level::error, "", "", e, -1);
        throw runtime_error(e);
    }
    *version = ldpSchemaVersion;
}

void initSchema(DBContext* db)
{
    string sql =
        "CREATE TABLE ldp_system.main (\n"
        "    ldp_schema_version BIGINT NOT NULL\n"
        ");";
    db->log->log(Level::trace, "", "", sql, -1);
    db->dbc->execDirect(nullptr, sql);
    sql = "INSERT INTO ldp_system.main (ldp_schema_version) VALUES (0);";
    db->log->log(Level::trace, "", "", sql, -1);
    db->dbc->execDirect(nullptr, sql);
}

void init(DBContext* db)
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


