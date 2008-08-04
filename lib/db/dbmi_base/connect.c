#include <grass/gis.h>
#include <grass/dbmi.h>

/*!
   \fn int db_set_connection (dbConnection *connection )
   \brief set default db connection settings
   \return DB_OK
   \param dbConnection
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
   \fn int db_get_connection (dbConnection *connection )
   \brief get default db connection settings
   \return DB_OK
   \param dbConnection
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
