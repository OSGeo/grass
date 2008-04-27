/**
 * \file db.c
 *
 * \brief Low Level SQLite databse driver.
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Radim Blazek
 *
 * \date 2005-2007
 */

#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/dbmi.h>
#include "globals.h"
#include "proto.h"


/**
 * \fn int db__driver_open_database (dbHandle *handle)
 *
 * \brief Open SQLite database.
 *
 * \param[in,out] handle database handle
 * \return int DB_FAILED on error; DB_OK on success
 */

int db__driver_open_database (dbHandle *handle)

{
    char *name, name2[2000];
    
    G_debug(3, "\ndb_driver_open_database()" );

    init_error();
    name = db_get_handle_dbname(handle);

    /* if name is empty use connection.databaseName */
    if (strlen(name) == 0) 
    {
        dbConnection connection;
        db_get_connection(&connection);
	name = connection.databaseName;
    }

    G_debug(3, "name = '%s'", name );

    /* parse variables in db.name if present */
    if ( strchr(name,'$') )
    {
        char **tokens;
        int no_tokens, n;

	tokens = G_tokenize (name, "/");
	no_tokens=G_number_of_tokens(tokens);

        name2[0] = '\0';
	for (n = 0; n < no_tokens ; n++)
	{
	   if ( n > 0 )
	   	strcat ( name2, "/" );

	   G_debug (3, "tokens[%d] = %s", n, tokens[n] );
	   if ( tokens[n][0] == '$' )
	   {
	       G_strchg(tokens[n],'$', ' ' );
	       G_chop(tokens[n]);
	       strcat(name2, G__getenv(tokens[n]) );
	       G_debug (3, "   -> %s", G__getenv(tokens[n]) );
	   }
	   else
	   {
	       strcat (name2, tokens[n]);
	   }
	 }
	 G_free_tokens ( tokens );
    }
    else
    {
	strcpy ( name2, name );
    }

    G_debug(2, "name2 = '%s'", name2 );

    if ( sqlite3_open(name2,&sqlite) != SQLITE_OK ) {
	append_error ( "Cannot open database: " );
        append_error ((char *) sqlite3_errmsg (sqlite));
	report_error ();
	return DB_FAILED;
    }

    return DB_OK;
}


/**
 * \fn int db__driver_close_database (void)
 *
 * \brief Close SQLite database.
 *
 * \return int always returns DB_OK
 */

int db__driver_close_database (void)
{
    G_debug(3, "db_close_database()" );

    init_error();
    sqlite3_close ( sqlite );

    return DB_OK;
}
