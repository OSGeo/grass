/*!
 * \file db/dbmi_client/delete_tab.c
 * 
 * \brief DBMI Library (client) - delete table
 *
 * (C) 1999-2008 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public
 * License (>=v2). Read the file COPYING that comes with GRASS
 * for details.
 *
 * \author Joel Jones (CERL/UIUC), Radim Blazek
 */

#include <grass/dbmi.h>
#include <grass/glocale.h>
#include "macros.h"

/*!
  \brief Delete table

  \param drv driver name
  \param dbname database name
  \param tblname table name

  \return DB_OK on success
  \return DB_FAILED on failure
 */
int db_delete_table(const char *drvname, const char *dbname, const char *tblname)
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
	G_warning(_("Unable to open driver <%s>"), drvname);
	return DB_FAILED;
    }
    db_set_handle(&handle, dbname, NULL);
    if (db_open_database(driver, &handle) != DB_OK) {
	G_warning(_("Unable to open database <%s> by driver <%s>"),
		  dbname, drvname);
	db_shutdown_driver(driver);
	return DB_FAILED;
    }

    /* Delete table */
    /* TODO test if the tables exist */
    db_set_string(&sql, "drop table ");
    db_append_string(&sql, tblname);
    G_debug(3, db_get_string(&sql));

    if (db_execute_immediate(driver, &sql) != DB_OK) {
	G_warning(_("Unable to drop table: '%s'"),
		  db_get_string(&sql));
	db_close_database(driver);
	db_shutdown_driver(driver);
	return DB_FAILED;
    }

    db_close_database(driver);
    db_shutdown_driver(driver);

    return DB_OK;
}
