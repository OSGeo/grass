#ifndef DBDRIVER_H
#define	DBDRIVER_H

#include <grass/dbstubs.h>

#define	init_dbdriver() do{\
db_driver_create_table = db__driver_create_table;\
db_driver_close_cursor = db__driver_close_cursor;\
db_driver_open_database = db__driver_open_database;\
db_driver_close_database = db__driver_close_database;\
db_driver_create_database = db__driver_create_database;\
db_driver_delete_database = db__driver_delete_database;\
db_driver_describe_table = db__driver_describe_table;\
db_driver_init = db__driver_init;\
db_driver_finish = db__driver_finish;\
db_driver_execute_immediate = db__driver_execute_immediate;\
db_driver_begin_transaction = db__driver_begin_transaction;\
db_driver_commit_transaction = db__driver_commit_transaction;\
db_driver_fetch = db__driver_fetch;\
db_driver_get_num_rows = db__driver_get_num_rows;\
db_driver_create_index = db__driver_create_index;\
db_driver_list_databases = db__driver_list_databases;\
db_driver_list_tables = db__driver_list_tables;\
db_driver_grant_on_table = db__driver_grant_on_table;\
db_driver_open_select_cursor = db__driver_open_select_cursor;\
db_driver_drop_table = db__driver_drop_table;\
}while(0)

#endif
