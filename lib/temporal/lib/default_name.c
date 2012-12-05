/*!
  \file lib/db/dbmi_base/default_name.c
  
  \brief Temporal GIS Library (base) - default settings
  
  (C) 2012 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Soeren Gebbert 
          Code is based on the dbmi library written by 
          Joel Jones (CERL/UIUC) and Radim Blazek
*/

#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/temporal.h>
#include <grass/glocale.h>

/*!
  \brief Get default TGIS driver name

  \return pointer to default TGIS driver name
  \return NULL if not set
*/
const char *tgis_get_default_driver_name(void)
{
    const char *drv;

    if ((drv = G__getenv2("TGISDB_DRIVER", G_VAR_MAPSET)))
	return G_store(drv);

    return NULL;
}

/*!
  \brief Get default TGIS database name

  \return pointer to default TGIS database name
  \return NULL if not set
*/
const char *tgis_get_default_database_name(void)
{
    const char *drv;

    if ((drv = G__getenv2("TGISDB_DATABASE", G_VAR_MAPSET)))
	return G_store(drv);

    return NULL;
}

/*!
  \brief Sets up TGIS database connection settings using GRASS default from temporal.h

  \return returns DB_OK 
*/
int tgis_set_default_connection(void)
{
    dbConnection connection;

    G_verbose_message(_("Creating new default TGIS DB params"));

    if (strcmp(TGISDB_DEFAULT_DRIVER, "sqlite") == 0) {

	connection.driverName = "sqlite";
	connection.databaseName =
	    "$GISDBASE/$LOCATION_NAME/PERMANENT/tgis.db";
	tgis_set_connection(&connection);
    }
    else
	G_fatal_error(_("Programmer error - only SQLite driver is currently supported"));

    return DB_OK;
}
