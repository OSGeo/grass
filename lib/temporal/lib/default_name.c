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
    char default_connection[2048];
    char use_current_mapset = 0;

    /* Check GRASS environmental variable if mapset specific
     * temporal databases should be created 
     * */

    if(G__getenv2("TGIS_USE_CURRENT_MAPSET", G_VAR_MAPSET)) {
        use_current_mapset = 1;
    } else {
        if(G__getenv2("TGIS_USE_CURRENT_MAPSET", G_VAR_GISRC))
            use_current_mapset = 1;
    }

    if(!use_current_mapset) {
        G_snprintf(default_connection, 2048, "%s/%s/%s", G_gisdbase(), G_location(),
                   TGISDB_DEFAULT_SQLITE_PATH);
    } else {
        G_snprintf(default_connection, 2048, "%s/%s/%s/tgis/sqlite.db", G_gisdbase(), G_location(),
                   G_mapset());
    }

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
