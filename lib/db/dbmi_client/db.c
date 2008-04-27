#include <grass/dbmi.h>
#include "macros.h"

/*!
 \fn dbDriver * db_start_driver_open_database ( char *drvname, char *dbname )
 \brief 
 \return 
 \param 
*/
dbDriver *
db_start_driver_open_database ( char *drvname, char *dbname )
{
    dbHandle handle;
    dbDriver *driver;

    G_debug ( 3, "db_start_driver_open_database():\n  drvname = %s, dbname = %s", drvname, dbname );

    db_init_handle (&handle);

    driver = db_start_driver(drvname);
    if ( driver == NULL) {
	G_warning ( "Cannot open driver '%s'", drvname);
	return NULL;
    }
    db_set_handle (&handle, dbname, NULL);
    if (db_open_database(driver, &handle) != DB_OK) {
	G_warning ( "Cannot open database '%s'", dbname);
	db_shutdown_driver(driver);
	return NULL;
    }

    return driver;
}

/*!
 \fn int db_close_database_shutdown_driver (dbDriver *driver )
 \brief 
 \return 
 \param 
*/
int
db_close_database_shutdown_driver ( dbDriver *driver )
{
    db_close_database(driver);
    db_shutdown_driver(driver);

    return DB_OK;
}

