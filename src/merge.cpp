#include "merge.h"
#include "names.h"
#include "util.h"

static void mergeTable(const Options& opt, const TableSchema& table,
        etymon::Postgres* db)
{
    string stagingTable;
    stagingTableName(table.tableName, &stagingTable);
    string loadingTable;
    loadingTableName(table.tableName, &loadingTable);

    string sql = "CREATE TABLE ";
    sql += loadingTable;
    sql += " (\n"
        "    id VARCHAR(65535) NOT NULL PRIMARY KEY,\n";
    string columnType;
    for (const auto& column : table.columns) {
        if (column.columnName != "id") {
            sql += "    \"";
            sql += column.columnName;
            sql += "\" ";
            ColumnSchema::columnTypeToString(column.columnType, &columnType);
            sql += columnType;
            sql += ",\n";
        }
    }
    sql += "    data ";
    sql += opt.dbtype.jsonType();
    sql += "\n"
        ");\n";
    printSQL(Print::debug, opt, sql);
    { etymon::PostgresResult result(db, sql); }

    // Add comment on table.
    if (table.moduleName != "mod-agreements") {
        sql = "COMMENT ON TABLE " + loadingTable + " IS '";
        sql += table.sourcePath;
        sql += " in ";
        sql += table.moduleName;
        sql += ": ";
        sql += "https://dev.folio.org/reference/api/#";
        sql += table.moduleName;
        sql += "';";
        printSQL(Print::debug, opt, sql);
        { etymon::PostgresResult result(db, sql); }
    }

    sql = "INSERT INTO " + loadingTable + "\n"
        "SELECT id,\n";
    string exp;
    string val;
    for (const auto& column : table.columns) {
        if (column.columnName != "id") {
            ColumnSchema::columnTypeToString(column.columnType, &columnType);
            val = "json_extract_path_text(data, '";
            val += column.sourceColumnName;
            val += "')";
            switch (column.columnType) {
            case ColumnType::varchar:
                exp = val;
                break;
            case ColumnType::boolean:
                exp = "CASE ";
                exp += val;
                exp += "\n"
                    "            WHEN 'true' THEN TRUE "
                    "WHEN 'false' THEN FALSE "
                    "ELSE NULL "
                    "END";
                break;
            default:
                exp = "NULLIF(";
                exp += val;
                exp += ", '')::";
                exp += columnType;
            }
            sql += "       ";
            sql += exp;
            sql += "\n"
                "                AS \"";
            sql += column.columnName;
            sql += "\",\n";
        }
    }
    sql += "       data\n"
        "    FROM ";
    sql += stagingTable;
    sql += ";\n";
    printSQL(Print::debug, opt, sql);
    { etymon::PostgresResult result(db, sql); }

    // Update history tables.

    // TODO merge history table.

}

static void dropTable(const Options& opt, const TableSchema& table,
        etymon::Postgres* db)
{
    string sql = "DROP TABLE IF EXISTS " + table.tableName + ";";
    printSQL(Print::debug, opt, sql);
    { etymon::PostgresResult result(db, sql); }
}

static void placeTable(const Options& opt, const TableSchema& table,
        etymon::Postgres* db)
{
    string loadingTable;
    loadingTableName(table.tableName, &loadingTable);
    string sql = "ALTER TABLE " + loadingTable +
        " RENAME TO " + table.tableName + ";";
    printSQL(Print::debug, opt, sql);
    { etymon::PostgresResult result(db, sql); }
}

void mergeAll(const Options& opt, Schema* schema, etymon::Postgres* db)
{
    print(Print::verbose, opt, "merging");
    for (auto& table : schema->tables) {
        if (table.skip)
            continue;
        print(Print::verbose, opt, "merging table: " + table.tableName);
        mergeTable(opt, table, db);
    }
    // Table-level exclusive locks begin here.
    print(Print::verbose, opt, "replacing tables");
    for (auto& table : schema->tables)
        dropTable(opt, table, db);
    for (auto& table : schema->tables) {
        if (table.skip)
            continue;
        placeTable(opt, table, db);
    }
}

