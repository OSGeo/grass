
/*****************************************************************************
* just a test for debugging purpose, imitating dbf driver -a.sh.
*****************************************************************************/

#include <stdlib.h>
#include <grass/dbmi.h>
#include <grass/gis.h>
#include "globals.h"

int main(int argc, char *argv[])
{

    dbCursor *cursor;
    int stat;
    dbToken token;
    dbString *select;
    int mode = 0;
    dbDriver *driver;
    dbHandle handle;

    driver = db_start_driver("dbf");
    if (driver == NULL)
	exit(EXIT_FAILURE);

    db_init_handle(&handle);
    db_set_handle(&handle, "dbf_catalog", NULL);

    if (db__test_database_open()) {
	db_error("Multiple open databases not allowed");
	return DB_OK;
    }

    /* call the procedure */
    stat = db_driver_open_database(&handle);

    /* send the return code */
    if (stat != DB_OK) {
	db_free_handle(&handle);
	return DB_OK;
    }

    /* record the open in the driver state */
    db__mark_database_open(db_get_handle_dbname(&handle),
			   db_get_handle_dbpath(&handle));
    /* DO NOT free the handle since we saved the pointers to the name,path */

    select = (dbString *) G_malloc(1024);

    db_init_string(select);

    db_set_string(select,
		  "select id, quality, flow from river where (flow = 10) or (flow = 20) or (flow = 30) or (flow = 5) or (flow = 7)");

    /* create a cursor */
    cursor = (dbCursor *) db_malloc(sizeof(dbCursor));
    if (cursor == NULL)
	return db_get_error_code();
    token = db_new_token((dbAddress) cursor);
    if (token < 0)
	return db_get_error_code();
    db_init_cursor(cursor);
    cursor->driver = driver;

    G_debug(3, "sql is %s", *select);
    G_debug(3, "driver is %s", cursor->driver);

    /* call the procedure */
    stat = db_driver_open_select_cursor(select, cursor, mode);
    db_free_string(select);

    /* mark this as a readonly cursor */
    db_set_cursor_type_readonly(cursor);

    /* add this cursor to the cursors managed by the driver state */
    db__add_cursor_to_driver_state(cursor);
    G_debug(3, "db_d_close_database()");

    /* see if a database is open */
    if (!db__test_database_open()) {
	db_error("no database is open");
	G_debug(3, "db_d_close_database(): would sent DB_FAILURE");
	return DB_OK;
    };
    /* make sure all cursors are closed */
    db__close_all_cursors();

    /* call the procedure */
    stat = db_driver_close_database();
    G_debug(3, "db_d_close_database(): would have stat = %d", stat);

    /* send the return code */
    if (stat != DB_OK) {
	G_debug(3, "db_d_close_database(): would sent DB_FAILURE");
	return DB_OK;
    }
    G_debug(3, "db_d_close_database(): would sent DB_OK");

    /* clear the driver state */
    db__mark_database_closed();
    db__init_driver_state();


    G_debug(3, "main(): ok");
    return DB_OK;

}
