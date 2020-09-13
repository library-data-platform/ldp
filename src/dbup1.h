#ifndef LDP_DBUP1_H
#define LDP_DBUP1_H

#include "init.h"

void database_upgrade_1(database_upgrade_options* opt);
void database_upgrade_2(database_upgrade_options* opt);
void database_upgrade_3(database_upgrade_options* opt);
void database_upgrade_4(database_upgrade_options* opt);
void database_upgrade_5(database_upgrade_options* opt);
void database_upgrade_6(database_upgrade_options* opt);
void database_upgrade_7(database_upgrade_options* opt);
void database_upgrade_8(database_upgrade_options* opt);
void database_upgrade_9(database_upgrade_options* opt);
void database_upgrade_10(database_upgrade_options* opt);
void database_upgrade_11(database_upgrade_options* opt);
void database_upgrade_12(database_upgrade_options* opt);
void database_upgrade_13(database_upgrade_options* opt);
void database_upgrade_14(database_upgrade_options* opt);
void database_upgrade_15(database_upgrade_options* opt);
void database_upgrade_16(database_upgrade_options* opt);
void database_upgrade_17(database_upgrade_options* opt);
void database_upgrade_18(database_upgrade_options* opt);
void database_upgrade_19(database_upgrade_options* opt);

void ulog_sql(const string& sql, database_upgrade_options* opt);
void ulog_commit(database_upgrade_options* opt);

#endif

