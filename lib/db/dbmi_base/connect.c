/*!
  \file lib/db/dbmi_base/connect.c
  
  \brief DBMI Library (base) - connect to DB
  
  (C) 1999-2009, 2011 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Joel Jones (CERL/UIUC), Radim Blazek
  \author Doxygenized by Martin Landa <landa.martin gmail.com> (2011)
*/

#include <grass/gis.h>
#include <grass/dbmi.h>

/*!
  \brief Set default DB connection settings

  This function sets enviromental variables as DB_DRIVER, DB_DATABASE,
  DB_SCHEMA, DB_GROUP.

  \param connection pointer to dbConnection with default settings
  
  \return DB_OK
 */
int db_set_connection(dbConnection * connection)
{
    /* TODO: add checks and return DB_* error code if needed */

    if (connection->driverName)
	G_setenv2("DB_DRIVER", connection->driverName, G_VAR_MAPSET);

    if (connection->databaseName)
	G_setenv2("DB_DATABASE", connection->databaseName, G_VAR_MAPSET);

    if (connection->schemaName)
	G_setenv2("DB_SCHEMA", connection->schemaName, G_VAR_MAPSET);

    if (connection->group)
	G_setenv2("DB_GROUP", connection->group, G_VAR_MAPSET);

    /* below commented due to new mechanism:
       if ( connection->hostName )
       G_setenv("DB_HOST", connection->hostName);

       if ( connection->location )
       G_setenv("DB_LOCATION", connection->location);

       if ( connection->user )
       G_setenv("DB_USER", connection->user);

       if ( connection->password )
       G_setenv("DB_PASSWORD", connection->password);
     */

    return DB_OK;
}

/*!
  \brief Get default DB connection settings
  
  \param[out] connection pointer to dbConnection to be modified

  \return DB_OK
 */
int db_get_connection(dbConnection * connection)
{
  /* TODO: add checks and return DB_* error code if needed */

    connection->driverName = G__getenv2("DB_DRIVER", G_VAR_MAPSET);
    connection->databaseName = G__getenv2("DB_DATABASE", G_VAR_MAPSET);
    connection->schemaName = G__getenv2("DB_SCHEMA", G_VAR_MAPSET);
    connection->group = G__getenv2("DB_GROUP", G_VAR_MAPSET);

    /* below commented due to new mechanism:
       connection->hostName = G__getenv("DB_HOST");
       connection->location = G__getenv("DB_LOCATION");
       connection->user = G__getenv("DB_USER");
       connection->password = G__getenv("DB_PASSWORD");
     */
    
    return DB_OK;
}
