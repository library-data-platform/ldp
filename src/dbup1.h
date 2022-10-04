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
void database_upgrade_20(database_upgrade_options* opt);
void database_upgrade_21(database_upgrade_options* opt);
void database_upgrade_22(database_upgrade_options* opt);
void database_upgrade_23(database_upgrade_options* opt);
void database_upgrade_24(database_upgrade_options* opt);
void database_upgrade_25(database_upgrade_options* opt);
void database_upgrade_26(database_upgrade_options* opt);
void database_upgrade_27(database_upgrade_options* opt);
void database_upgrade_28(database_upgrade_options* opt);
void database_upgrade_29(database_upgrade_options* opt);
void database_upgrade_30(database_upgrade_options* opt);
void database_upgrade_31(database_upgrade_options* opt);
void database_upgrade_32(database_upgrade_options* opt);

void ulog_sql(const string& sql, database_upgrade_options* opt);
void ulog_commit(database_upgrade_options* opt);

#endif

