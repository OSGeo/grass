#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/dbmi.h>

/*!
 \fn const char * db_get_default_driver_name ( void )
 \brief returns pointer to default driver name
 \return returns pointer to default driver name or NULL if not set
*/
const char *
db_get_default_driver_name ( void )
{
    const char *drv;

    if ( (drv = G__getenv2("DB_DRIVER", G_VAR_MAPSET))  )
       return G_store(drv);

    return NULL;
}

/*!
 \fn const char * db_get_default_database_name ( void )
 \brief returns pointer to default database name
 \return returns pointer to default database name or NULL if not set
*/
const char *
db_get_default_database_name ( void )
{
    const char *drv;

    if ( (drv = G__getenv2("DB_DATABASE", G_VAR_MAPSET))  )
       return G_store(drv);

    return NULL;
}

/*!
 \fn const char * db_get_default_schema_name ( void )
 \brief returns pointer to default schema name
 \return returns pointer to default schema name or NULL if not set
*/
const char *
db_get_default_schema_name ( void )
{
    const char *sch;
    
    if ( (  sch = G__getenv2("DB_SCHEMA", G_VAR_MAPSET) )  )
	  return G_store(sch);

    return NULL;
}

/*!
 \fn const char * db_get_default_group_name ( void )
 \brief returns pointer to default group name
 \return returns pointer to default group name or NULL if not set
*/
const char *
db_get_default_group_name ( void )
{
    const char *gr;
    
    if ( (  gr = G__getenv2("DB_GROUP", G_VAR_MAPSET) )  )
	  return G_store(gr);

    return NULL;
}



/*!
 \fn int db_set_default_connection(void)
 \brief sets up database connection settings using GRASS default from dbmi.h
 \return returns DB_OK (TODO: DB_OK on success, DB_* error code on fail)
*/
int db_set_default_connection(void)
{
    dbConnection connection;
    char buf[GPATH_MAX];

    G_debug(1,"Creating new default DB params with db_set_default_connection()");

    /* is this really needed ? */
    db_get_connection(&connection);

    if(strcmp(DB_DEFAULT_DRIVER, "dbf") == 0 ) {
	/* Set default values and create dbf db dir */

	connection.driverName = "dbf";
	connection.databaseName = "$GISDBASE/$LOCATION_NAME/$MAPSET/dbf/";
	db_set_connection( &connection );

	sprintf ( buf, "%s/%s/dbf", G_location_path(), G_mapset());
	G__make_mapset_element ( "dbf" );
    }
    else if (strcmp(DB_DEFAULT_DRIVER, "sqlite") == 0 ) {
	/* Set default values and create dbf db dir */

	connection.driverName = "sqlite";
/*
 * TODO: Use one DB for entire mapset (LFS problems?)
 *	or per-map DBs in $MASPET/vector/mapname/sqlite.db (how to set that here?)
 *	or $MAPSET/sqlite/mapname.sql as with dbf?
 */
	connection.databaseName = "$GISDBASE/$LOCATION_NAME/$MAPSET/sqlite.db";
	db_set_connection( &connection );
    }
    else G_fatal_error("Programmer error");

    return DB_OK;
}
