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

  \param drvname driver name
  \param dbname database name
  \param tblname table name

  \return DB_OK on success
  \return DB_FAILED on failure
 */
int db_delete_table(const char *drvname, const char *dbname, const char *tblname)
{
    dbDriver *driver;
    dbString sql;

    G_debug(3, "db_delete_table(): driver = %s, db = %s, table = %s\n",
	    drvname, dbname, tblname);

    /* Open driver and database */
    driver = db_start_driver_open_database(drvname, dbname);
    if (driver == NULL) {
	G_warning(_("Unable open database <%s> by driver <%s>"), dbname,
		  drvname);
	return DB_FAILED;
    }

    /* Delete table */
    /* TODO test if the tables exist */
    db_init_string(&sql);
    db_set_string(&sql, "drop table ");
    db_append_string(&sql, tblname);
    G_debug(3, "%s", db_get_string(&sql));

    if (db_execute_immediate(driver, &sql) != DB_OK) {
	G_warning(_("Unable to drop table: '%s'"),
		  db_get_string(&sql));
	db_close_database_shutdown_driver(driver);
	return DB_FAILED;
    }

    db_close_database_shutdown_driver(driver);

    return DB_OK;
}
