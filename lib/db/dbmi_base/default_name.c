/*!
  \file lib/db/dbmi_base/default_name.c
  
  \brief DBMI Library (base) - default settings
  
  (C) 1999-2010 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Joel Jones (CERL/UIUC)
  \author Upgraded to GRASS 5.7 by Radim Blazek
*/

#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

/*!
  \brief Get driver name from current DB connection settings

  \return pointer to driver name
  \return NULL if not set
*/
const char *db_get_default_driver_name(void)
{
    const char *drv;

    if ((drv = G_getenv_nofatal2("DB_DRIVER", G_VAR_MAPSET)))
	return G_store(drv);

    return NULL;
}

/*!
  \brief Get database name from current DB connection settings

  \return pointer to database name
  \return NULL if not set
*/
const char *db_get_default_database_name(void)
{
    const char *drv;

    if ((drv = G_getenv_nofatal2("DB_DATABASE", G_VAR_MAPSET)))
	return G_store(drv);

    return NULL;
}

/*!
  \brief Get schema name from current DB connection settings
  
  \return pointer to schema name
  \return NULL if not set
*/
const char *db_get_default_schema_name(void)
{
    const char *sch;

    if ((sch = G_getenv_nofatal2("DB_SCHEMA", G_VAR_MAPSET)))
	return G_store(sch);

    return NULL;
}

/*!
  \brief Get group name from current DB connection settings
  
  \return pointer to group name
  \return NULL if not set
*/
const char *db_get_default_group_name(void)
{
    const char *gr;

    if ((gr = G_getenv_nofatal2("DB_GROUP", G_VAR_MAPSET)))
	return G_store(gr);

    return NULL;
}

/*!
  \brief Sets up database connection settings using GRASS default from dbmi.h

  This function ignores current DB connection settings and uses GRASS 
  default settings instead.

  \todo DB_OK on success, DB_* error code on fail

  \return returns DB_OK 
*/
int db_set_default_connection(void)
{
    dbConnection connection;
    char buf[GPATH_MAX];

    G_debug(1,
	    "Creating new default DB params with db_set_default_connection()");

    /* do not use default DB connection settings for the current mapset */
    G_zero(&connection, sizeof(dbConnection));

    if (strcmp(DB_DEFAULT_DRIVER, "dbf") == 0) {
	/* Set default values and create dbf db dir */

	connection.driverName = "dbf";
	connection.databaseName = "$GISDBASE/$LOCATION_NAME/$MAPSET/dbf/";
	db_set_connection(&connection);

	sprintf(buf, "%s/%s/dbf", G_location_path(), G_mapset());
	G_make_mapset_element("dbf");
    }
    else if (strcmp(DB_DEFAULT_DRIVER, "sqlite") == 0) {
	/* Set default values and create sqlite db dir */

	connection.driverName = "sqlite";
	/*
	 * TODO: Use one DB for entire mapset (LFS problems?)
	 *      or per-map DBs in $MASPET/vector/mapname/sqlite.db (how to set that here?)
	 *      or $MAPSET/sqlite/mapname.sql as with dbf?
	 */
	 
	 /* http://www.sqlite.org/lockingv3.html
	  * When SQLite creates a journal file on Unix, it opens the 
	  * directory that contains that file and calls fsync() on the 
	  * directory, in an effort to push the directory information to disk.
	  * 
	  * -> have sqlite.db in a separate directory
	  */
	connection.databaseName =
	    "$GISDBASE/$LOCATION_NAME/$MAPSET/sqlite/sqlite.db";
	G_make_mapset_element("sqlite");
	db_set_connection(&connection);
    }
    else
	G_fatal_error(_("Programmer error"));

    return DB_OK;
}
