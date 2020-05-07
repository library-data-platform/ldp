#include <stdexcept>

#include "../etymoncpp/include/util.h"
#include "idmap.h"

IDMap::IDMap(etymon::OdbcEnv* odbc, const string& dataSourceName, Log* log)
{
    dbc = new etymon::OdbcDbc(odbc, dataSourceName);
    dbt = new DBType(dbc);
    this->log = log;
}

IDMap::~IDMap()
{
    delete dbt;
    delete dbc;
}

void IDMap::makeSK(const char* id, string* sk)
{
    string lowerID = id;
    etymon::toLower(&lowerID);
    string encodedID;
    dbt->encodeStringConst(lowerID.c_str(), &encodedID);
    string selectSQL =
        "SELECT sk FROM ldpsystem.idmap WHERE id = " + encodedID + ";";
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
        "INSERT INTO ldpsystem.idmap (id) VALUES (" + encodedID + ");";
    log->log(Level::detail, "", "", insertSQL, -1);
    dbc->execDirect(nullptr, insertSQL);
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


