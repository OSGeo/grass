#include <grass/dbmi.h>
#include "macros.h"

/*!
   \fn int db_delete_table ( const char *drvname, const char *dbname, const char *tblname )
   \brief 
   \return 
   \param 
 */
int
db_delete_table(const char *drvname, const char *dbname, const char *tblname)
{
    dbDriver *driver;
    dbHandle handle;
    dbString sql;

    G_debug(3, "db_delete_table(): driver = %s, db = %s, table = %s\n",
	    drvname, dbname, tblname);

    db_init_handle(&handle);
    db_init_string(&sql);

    /* Open driver and database */
    driver = db_start_driver(drvname);
    if (driver == NULL) {
	G_warning("Cannot open driver '%s'", drvname);
	return DB_FAILED;
    }
    db_set_handle(&handle, dbname, NULL);
    if (db_open_database(driver, &handle) != DB_OK) {
	G_warning("Cannot open database '%s'", dbname);
	db_shutdown_driver(driver);
	return DB_FAILED;
    }

    /* Delete table */
    /* TODO test if the tables exist */
    db_set_string(&sql, "drop table ");
    db_append_string(&sql, tblname);
    G_debug(3, db_get_string(&sql));

    if (db_execute_immediate(driver, &sql) != DB_OK) {
	G_warning("Cannot drop table: '%s'", db_get_string(&sql));
	db_close_database(driver);
	db_shutdown_driver(driver);
	return DB_FAILED;
    }

    db_close_database(driver);
    db_shutdown_driver(driver);

    return DB_OK;
}
