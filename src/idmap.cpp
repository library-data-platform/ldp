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

void IDMap::makeIID(const char* id, string* iid)
{
    string encodedID;
    dbt->encodeStringConst(id, &encodedID);
    string selectSQL =
        "SELECT iid FROM ldpsystem.idmap WHERE id = " + encodedID + ";";
    log->log(Level::detail, "", "", selectSQL, -1);
    {
        etymon::OdbcStmt stmt(dbc);
        dbc->execDirect(&stmt, selectSQL);
        if (dbc->fetch(&stmt)) {
            dbc->getData(&stmt, 1, iid);
            return;
        }
    }
    // The iid was not found.  So we shall add it.
    string insertSQL =
        "INSERT INTO ldpsystem.idmap (id) VALUES (" + encodedID + ");";
    log->log(Level::detail, "", "", insertSQL, -1);
    dbc->execDirect(nullptr, insertSQL);
    log->log(Level::detail, "", "", selectSQL, -1);
    {
        etymon::OdbcStmt stmt(dbc);
        dbc->execDirect(&stmt, selectSQL);
        if (dbc->fetch(&stmt)) {
            dbc->getData(&stmt, 1, iid);
            return;
        }
    }
    throw runtime_error("Unable to map ID: " + string(id));
}


