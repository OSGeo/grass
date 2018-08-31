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

  This function sets environmental variables as DB_DRIVER, DB_DATABASE,
  DB_SCHEMA, DB_GROUP.

  \param connection pointer to dbConnection with default settings
  
  \return DB_OK
 */
int db_set_connection(dbConnection * connection)
{
    /* TODO: add checks and return DB_* error code if needed */

    G_unsetenv2("DB_DRIVER", G_VAR_MAPSET);
    if (connection->driverName)
	G_setenv2("DB_DRIVER", connection->driverName, G_VAR_MAPSET);

    G_unsetenv2("DB_DATABASE", G_VAR_MAPSET);
    if (connection->databaseName)
	G_setenv2("DB_DATABASE", connection->databaseName, G_VAR_MAPSET);

    G_unsetenv2("DB_SCHEMA", G_VAR_MAPSET);
    if (connection->schemaName)
	G_setenv2("DB_SCHEMA", connection->schemaName, G_VAR_MAPSET);

    G_unsetenv2("DB_GROUP", G_VAR_MAPSET);
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
  \brief Get default DB connection settings for the current mapset
  
  \param[out] connection pointer to dbConnection to be modified

  \return DB_OK
  \return DB_FAILED
 */
int db_get_connection(dbConnection * connection)
{
    G_zero(connection, sizeof(dbConnection));
    
    connection->driverName = (char *)G_getenv_nofatal2("DB_DRIVER", G_VAR_MAPSET);
    connection->databaseName = (char *)G_getenv_nofatal2("DB_DATABASE", G_VAR_MAPSET);
    
    if (connection->driverName == NULL ||
        connection->databaseName == NULL)
        return DB_FAILED;
    
    connection->schemaName = (char *)G_getenv_nofatal2("DB_SCHEMA", G_VAR_MAPSET);
    connection->group = (char *)G_getenv_nofatal2("DB_GROUP", G_VAR_MAPSET);

    /* try to get user/password */
    db_get_login2(connection->driverName, connection->databaseName,
                  (const char **) &(connection->user),
                  (const char **) &(connection->password),
                  (const char **) &(connection->hostName),
                  (const char **) &(connection->port));
    
    return DB_OK;
}
