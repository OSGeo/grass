/*!
  \file lib/temporal/lib/connect.c
  
  \brief Temporal GIS Library - connect to TGIS DB
  
  (C) 2012 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Soeren Gebbert 
          Code is based on the dbmi library written by 
          Joel Jones (CERL/UIUC) and Radim Blazek
*/
#include <grass/temporal.h>

/*!
 * \brief Get TGIS driver name
 * 
 * \return pointer to TGIS driver name
 * \return NULL if not set
 */
char *tgis_get_driver_name(void)
{
    const char *drv;
    
    if ((drv = G__getenv2("TGISDB_DRIVER", G_VAR_MAPSET)))
        return G_store(drv);
    
    return NULL;
}

/*!
 * \brief Get TGIS database name
 * 
 * \return pointer to TGIS database name
 * \return NULL if not set
 */
char *tgis_get_database_name(void)
{
    const char *drv;
    
    if ((drv = G__getenv2("TGISDB_DATABASE", G_VAR_MAPSET)))
        return G_store(drv);
    
    return NULL;
}

/*!
  \brief Set Temporal GIS DB connection settings

  This function sets environmental variables as TGISDB_DRIVER, TGISDB_DATABASE.

  \param connection pointer to dbConnection with default settings
  
  \return DB_OK
 */
int tgis_set_connection(dbConnection * connection)
{
    if (connection->driverName)
	G_setenv2("TGISDB_DRIVER", connection->driverName, G_VAR_MAPSET);

    if (connection->databaseName)
	G_setenv2("TGISDB_DATABASE", connection->databaseName, G_VAR_MAPSET);

    return DB_OK;
}

/*!
  \brief Get Temporal GIS DB connection settings
  
  \param[out] connection pointer to dbConnection to be modified

  \return DB_OK
 */
int tgis_get_connection(dbConnection * connection)
{
    G_zero(connection, sizeof(dbConnection));
    
    connection->driverName = (char *)G__getenv2("TGISDB_DRIVER", G_VAR_MAPSET);
    connection->databaseName = (char *)G__getenv2("TGISDB_DATABASE", G_VAR_MAPSET);

    return DB_OK;
}
