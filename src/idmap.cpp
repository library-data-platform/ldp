#include <stdexcept>

#include "../etymoncpp/include/util.h"
#include "idmap.h"

IDMap::IDMap(etymon::OdbcDbc* dbc, DBType* dbt, Log* log)
{
    this->dbc = dbc;
    this->dbt = dbt;
    this->log = log;
}

void IDMap::makeSK(const string& table, const char* id, string* sk)
{
    string lowerID = id;
    etymon::toLower(&lowerID);
    string encodedID;
    dbt->encodeStringConst(lowerID.c_str(), &encodedID);
    string selectSQL =
        "SELECT sk FROM ldpsystem.idmap\n"
        "    WHERE id = " + encodedID + ";";
    log->log(Level::detail, "", "", selectSQL, -1);
    {
        etymon::OdbcStmt stmt(dbc);
        dbc->execDirect(&stmt, selectSQL);
        if (dbc->fetch(&stmt)) {
            dbc->getData(&stmt, 1, sk);
            return;
        }
    }
    // The sk was not found; so we add it.
    string insertSQL =
        "INSERT INTO ldpsystem.idmap (id)\n"
        "    VALUES (" + encodedID + ");";
    log->log(Level::detail, "", "", insertSQL, -1);
    dbc->execDirect(nullptr, insertSQL);
    // Look up the assigned sk.
    // TODO Check if there is a Postgres-Redshift comaptible way to
    // return sk from the INSERT.
    log->log(Level::detail, "", "", selectSQL, -1);
    {
        etymon::OdbcStmt stmt(dbc);
        dbc->execDirect(&stmt, selectSQL);
        if (dbc->fetch(&stmt)) {
            dbc->getData(&stmt, 1, sk);
            return;
        }
    }
    throw runtime_error("Unable to map ID: " + string(id));
}


