/*!
 * \file db/dbmi_client/db.c
 * 
 * \brief DBMI Library (client) - open/close driver/database connection
 *
 * (C) 1999-2008 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public
 * License (>=v2). Read the file COPYING that comes with GRASS
 * for details.
 *
 * \author Radim Blazek
 */

#include <grass/dbmi.h>
#include <grass/glocale.h>
#include "macros.h"

/*!
  \brief Open driver/database connection

  \param drvname driver name
  \param dbname database name

  \return poiner to dbDriver structure
  \return NULL on failure
 */
dbDriver *db_start_driver_open_database(const char *drvname,
					const char *dbname)
{
    dbHandle handle;
    dbDriver *driver;

    G_debug(3, "db_start_driver_open_database():\n  drvname = %s, dbname = %s",
	    drvname, dbname);

    db_init_handle(&handle);

    driver = db_start_driver(drvname);
    if (driver == NULL) {
	G_warning(_("Unable to start driver <%s>"), drvname);
	return NULL;
    }
    db_set_handle(&handle, dbname, NULL);
    if (db_open_database(driver, &handle) != DB_OK) {
	G_warning(_("Unable to open database <%s> by driver <%s>"),
		  dbname, drvname);
	db_shutdown_driver(driver);
	return NULL;
    }

    return driver;
}

/*!
  \brief Close driver/database connection

  \param driver db driver

  \return DB_OK
*/
int db_close_database_shutdown_driver(dbDriver * driver)
{
    db_close_database(driver);
    db_shutdown_driver(driver);

    return DB_OK;
}
