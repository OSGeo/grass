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
*/
const char *tgis_get_default_driver_name(void)
{
    return TGISDB_DEFAULT_DRIVER;
}

/*!
  \brief Get default TGIS database name for the sqlite connection

  The default name is $GISDBASE/$LOCATION_NAME/PERMANENT/tgis/sqlite.db

  \return pointer to default TGIS database name
*/
char *tgis_get_default_database_name(void)
{
    int n;
    const char *name = NULL, *value = NULL;
    char *location = NULL, *gisbase = NULL;
    char default_connection[2048];

    for (n = 0; (name = G__env_name(n)); n++) {
        value = (char *)G__getenv(name);
        if (value) {
            if (G_strcasecmp("GISDBASE", name) == 0)
                gisbase = G_store(value);
            if (G_strcasecmp("LOCATION_NAME", name) == 0)
                location = G_store(value);
        }
    }

    G_snprintf(default_connection, 2048, "%s/%s/%s", gisbase, location,
               TGISDB_DEFAULT_SQLITE_PATH);

    if(location)
        G_free(location);
    if(gisbase)
        G_free(gisbase);

    return G_store(default_connection);
}

/*!
  \brief Sets up TGIS database connection settings using GRASS default

  \return returns DB_OK 
*/
int tgis_set_default_connection(void)
{
    dbConnection connection;
    char db_name[2048];
    char *tmp = tgis_get_default_database_name();

    G_snprintf(db_name, 2048, "%s", tmp);
    G_free(tmp);

    if (strcmp(TGISDB_DEFAULT_DRIVER, "sqlite") == 0) {
	connection.driverName = "sqlite";
        connection.databaseName = db_name;
	tgis_set_connection(&connection);
    }
    else
	G_fatal_error(_("Programmer error - only SQLite driver is currently supported"));

    
    return DB_OK;
}
