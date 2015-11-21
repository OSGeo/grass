#ifndef DBDRIVER_H
#define	DBDRIVER_H

#include <grass/dbstubs.h>

#define	init_dbdriver() do{\
db_driver_create_table = db__driver_create_table;\
db_driver_close_cursor = db__driver_close_cursor;\
db_driver_open_database = db__driver_open_database;\
db_driver_close_database = db__driver_close_database;\
db_driver_describe_table = db__driver_describe_table;\
db_driver_init = db__driver_init;\
db_driver_finish = db__driver_finish;\
db_driver_execute_immediate = db__driver_execute_immediate;\
db_driver_fetch = db__driver_fetch;\
db_driver_get_num_rows = db__driver_get_num_rows;\
db_driver_list_tables = db__driver_list_tables;\
db_driver_open_select_cursor = db__driver_open_select_cursor;\
}while(0)

#endif
