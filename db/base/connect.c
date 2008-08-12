
/****************************************************************************
 *
 * MODULE:       db.connect
 * AUTHOR(S):    Radim Blazek <radim.blazek gmail.com> (original contributor)
 *               Alex Shevlakov <sixote yahoo.com>, 
 *               Glynn Clements <glynn gclements.plus.com>,
 *               Markus Neteler <neteler itc.it>,
 *               Hamish Bowman <hamish_b yahoo com>
 * PURPOSE:      set parameters for connection to database
 * COPYRIGHT:    (C) 2002-2008 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/codes.h>
#include <grass/glocale.h>

/* database for DBF can be written with variables:
 *   database=$GISDBASE/$LOCATION_NAME/$MAPSET/dbf
 */

int main(int argc, char *argv[])
{
    dbConnection conn;
    struct Flag *print, *check_set_default;

    /*    struct Option *driver, *database, *user, *password, *keycol; */
    struct Option *driver, *database, *schema, *group;
    struct GModule *module;


    /* Initialize the GIS calls */
    G_gisinit(argv[0]);

    /* Set description */
    module = G_define_module();
    module->keywords = _("database, attribute table");
    module->description =
	_("Prints/sets general DB connection for current mapset and exits.");


    print = G_define_flag();
    print->key = 'p';
    print->description = _("Print current connection parameters and exit");

    check_set_default = G_define_flag();
    check_set_default->key = 'c';
    check_set_default->description =
	_("Check connection parameters, set if uninitialized, and exit");

    driver = G_define_standard_option(G_OPT_DRIVER);
    driver->options = db_list_drivers();
    driver->answer = (char *) db_get_default_driver_name();

    database = G_define_standard_option(G_OPT_DATABASE);
    database->answer = (char *) db_get_default_database_name();

    schema = G_define_option();
    schema->key = "schema";
    schema->type = TYPE_STRING;
    schema->required = NO;
    schema->multiple = NO;
    schema->answer = (char *) db_get_default_schema_name();
    schema->label = _("Database schema");
    schema->description = _("Do not use this option if schemas "
			    "are not supported by driver/database server");

    group = G_define_option();
    group->key = "group";
    group->type = TYPE_STRING;
    group->required = NO;
    group->multiple = NO;
    group->answer = (char*) db_get_default_group_name();
    group->description = _("Default group of database users to which "
			   "select privilege is granted");

    /* commented due to new mechanism:
       user = G_define_option() ;
       user->key        = "user" ;
       user->type       = TYPE_STRING ;
       user->required   = NO  ;
       user->multiple   = NO ;
       user->description= "User:" ;    

       password = G_define_option() ;
       password->key        = "password" ;
       password->type       = TYPE_STRING ;
       password->required   = NO  ;
       password->multiple   = NO ;
       password->description= "Password:" ;
     */

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);


    if (print->answer) {
	/* get and print connection */
	if (db_get_connection(&conn) == DB_OK) {
	    fprintf(stdout, "driver:%s\n",
		    conn.driverName ? conn.driverName : "");
	    fprintf(stdout, "database:%s\n",
		    conn.databaseName ? conn.databaseName : "");
	    fprintf(stdout, "schema:%s\n",
		    conn.schemaName ? conn.schemaName : "");
	    fprintf(stdout, "group:%s\n", conn.group ? conn.group : "");
	}
	else
	    G_fatal_error(_("Database connection not defined. "
			    "Run db.connect."));

	exit(EXIT_SUCCESS);
    }


    if (check_set_default->answer) {
	/* check connection and set to system-wide default in required */
	/*
	 * TODO: improve db_{get,set}_connection() to not return DB_OK on error
	 *       (thus currently there is no point in checking for that here)
	 */
	db_get_connection(&conn);

	if (!conn.driverName && !conn.databaseName) {

	    db_set_default_connection();
	    db_get_connection(&conn);

	    G_message(_("Default driver / database set to:\n"
			"driver: %s\ndatabase: %s"), conn.driverName,
		      conn.databaseName);
	}
	/* they must be a matched pair, so if one is set but not the other
	   then give up and let the user figure it out */
	else if (!conn.driverName) {
	    G_fatal_error(_("Default driver is not set"));
	}
	else if (!conn.databaseName) {
	    G_fatal_error(_("Default database is not set"));
	}

	/* connection either already existed or now exists */
	exit(EXIT_SUCCESS);
    }


    /* set connection */
    db_get_connection(&conn);	/* read current */

    if (driver->answer)
	conn.driverName = driver->answer;

    if (database->answer)
	conn.databaseName = database->answer;

    if (schema->answer)
	conn.schemaName = schema->answer;

    if (group->answer)
	conn.group = group->answer;

    db_set_connection(&conn);


    exit(EXIT_SUCCESS);
}
