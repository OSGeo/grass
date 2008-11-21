
/**********************************************************
 * MODULE:    mysql
 * AUTHOR(S): Radim Blazek (radim.blazek@gmail.com)
 * PURPOSE:   MySQL database driver
 * COPYRIGHT: (C) 2001 by the GRASS Development Team
 *            This program is free software under the 
 *            GNU General Public License (>=v2). 
 *            Read the file COPYING that comes with GRASS
 *            for details.
 **********************************************************/
#include <stdlib.h>
#include <string.h>

#include <grass/dbmi.h>
#include <grass/gis.h>
#include <grass/glocale.h>

#include "globals.h"
#include "proto.h"

int db__driver_open_database(dbHandle * handle)
{
    char *name;
    dbConnection default_connection;
    MYSQL *res;

    init_error();
    db_get_connection(&default_connection);
    name = G_store(db_get_handle_dbname(handle));

    /* if name is empty use default_connection.databaseName */
    if (strlen(name) == 0)
	name = default_connection.databaseName;

    G_debug(3, "db_driver_open_database() mysql: database definition = '%s'",
	    name);

    /* Embedded version */
    {
	char *datadir, *database;
	char *server_args[4];
	char *buf;

	if (!replace_variables(name, &datadir, &database)) {
	    append_error(_("Cannot parse MySQL embedded database name"));
	    append_error(mysql_error(connection));
	    report_error();
	    return DB_FAILED;
	}

	server_args[0] = "mesql";	/* this string is not used */
	G_asprintf(&buf, "--datadir=%s", datadir);
	server_args[1] = buf;
	/* With InnoDB it is very slow to close the database */
	server_args[2] = "--skip-innodb";	/* OK? */
	/* Without --bootstrap it complains about missing 
	 * mysql.time_zone_leap_second table */
	server_args[3] = "--bootstrap";	/* OK? */

	if (mysql_server_init(4, server_args, NULL)) {
	    append_error(_("Cannot initialize MySQL embedded server"));
	    append_error(mysql_error(connection));
	    report_error();
	    free(datadir);
	    free(database);
	    return DB_FAILED;
	}

	connection = mysql_init(NULL);
	mysql_options(connection, MYSQL_OPT_USE_EMBEDDED_CONNECTION, NULL);

	res =
	    mysql_real_connect(connection, NULL, NULL, NULL, database, 0,
			       NULL, 0);

	free(datadir);
	free(database);

	if (res == NULL) {
	    append_error(_("Cannot connect to MySQL embedded server: "));
	    append_error(mysql_error(connection));
	    report_error();
	    return DB_FAILED;
	}
    }

    return DB_OK;
}

int db__driver_close_database(void)
{
    init_error();
    mysql_close(connection);	/* this will also release connection */

    mysql_server_end();

    return DB_OK;
}
