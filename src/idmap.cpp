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

void IDMap::makeSK(const string& table, const char* id, string* sk,
        string* storedTable)
{
    string lowerID = id;
    etymon::toLower(&lowerID);
    string encodedID;
    dbt->encodeStringConst(lowerID.c_str(), &encodedID);
    string selectSQL =
        "SELECT sk, table_name FROM ldpsystem.idmap\n"
        "    WHERE id = " + encodedID + ";";
    log->log(Level::detail, "", "", selectSQL, -1);
    bool found = false;
    string foundTable;
    {
        etymon::OdbcStmt stmt(dbc);
        dbc->execDirect(&stmt, selectSQL);
        if (dbc->fetch(&stmt)) {
            found = true;
            dbc->getData(&stmt, 1, sk);
            dbc->getData(&stmt, 2, &foundTable);
            if (storedTable != nullptr)
                *storedTable = foundTable;
        }
    }
    if (found) {
        if (foundTable == "" && table != "") {
            string sql =
                "UPDATE ldpsystem.idmap\n"
                "    SET table_name = '" + table + "'\n"
                "    WHERE sk = " + *sk + ";";
            log->logDetail(sql);
            dbc->execDirect(nullptr, sql);
        }
        return;
    }
    // The sk was not found; so we add it.
    string insertSQL =
        "INSERT INTO ldpsystem.idmap (id, table_name)\n"
        "    VALUES (" + encodedID + ", '" + table + "');";
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


