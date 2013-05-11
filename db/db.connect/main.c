
/****************************************************************************
 *
 * MODULE:       db.connect
 * AUTHOR(S):    Radim Blazek <radim.blazek gmail.com> (original contributor)
 *               Alex Shevlakov <sixote yahoo.com>, 
 *               Glynn Clements <glynn gclements.plus.com>,
 *               Markus Neteler <neteler itc.it>,
 *               Hamish Bowman <hamish_b yahoo com>
 *               Martin Landa <landa.martin gmail.com> ('d' flag)
 * PURPOSE:      set parameters for connection to database
 * COPYRIGHT:    (C) 2002-2013 by the GRASS Development Team
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
#include <grass/glocale.h>

/* database for DBF can be written with variables:
 *   database=$GISDBASE/$LOCATION_NAME/$MAPSET/dbf
 */

int main(int argc, char *argv[])
{
    dbConnection conn;
    struct Flag *print, *shell, *check_set_default, *def;

    /*    struct Option *driver, *database, *user, *password, *keycol; */
    struct Option *driver, *database, *schema, *group;
    struct GModule *module;

    /* Initialize the GIS calls */
    G_gisinit(argv[0]);

    /* Set description */
    module = G_define_module();
    G_add_keyword(_("database"));
    G_add_keyword(_("attribute table"));
    G_add_keyword(_("connection settings"));
    module->description =
	_("Prints/sets general DB connection for current mapset.");

    print = G_define_flag();
    print->key = 'p';
    print->description = _("Print current connection parameters and exit");
    print->guisection = _("Print");

    shell = G_define_flag();
    shell->key = 'g';
    shell->description = _("Print current connection parameters using shell style and exit");
    shell->guisection = _("Print");

    check_set_default = G_define_flag();
    check_set_default->key = 'c';
    check_set_default->description =
	_("Check connection parameters, set if uninitialized, and exit");
    check_set_default->guisection = _("Set");
    
    def = G_define_flag();
    def->key = 'd';
    def->label = _("Set from default settings and exit");
    def->description = _("Overwrite current settings if already initialized");
    def->guisection = _("Set");

    driver = G_define_standard_option(G_OPT_DB_DRIVER);
    driver->options = db_list_drivers();
    driver->answer = (char *) db_get_default_driver_name();
    driver->guisection = _("Set");

    database = G_define_standard_option(G_OPT_DB_DATABASE);
    database->answer = (char *) db_get_default_database_name();
    database->guisection = _("Set");

    schema = G_define_standard_option(G_OPT_DB_SCHEMA);
    schema->answer = (char *) db_get_default_schema_name();
    schema->guisection = _("Set");

    group = G_define_option();
    group->key = "group";
    group->type = TYPE_STRING;
    group->required = NO;
    group->multiple = NO;
    group->answer = (char*) db_get_default_group_name();
    group->description = _("Default group of database users to which "
			   "select privilege is granted");
    group->guisection = _("Set");

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

    if (print->answer || shell->answer) {
	/* get and print connection in shell style */
	if (db_get_connection(&conn) == DB_OK) {
            if (shell->answer) {
                fprintf(stdout, "driver=%s\n",
                        conn.driverName ? conn.driverName : "");
                fprintf(stdout, "database=%s\n",
                        conn.databaseName ? conn.databaseName : "");
                fprintf(stdout, "schema=%s\n",
                        conn.schemaName ? conn.schemaName : "");
                fprintf(stdout, "group=%s\n", conn.group ? conn.group : "");
            }
            else {
                fprintf(stdout, "driver: %s\n",
                        conn.driverName ? conn.driverName : "");
                fprintf(stdout, "database: %s\n",
                        conn.databaseName ? conn.databaseName : "");
                fprintf(stdout, "schema: %s\n",
                        conn.schemaName ? conn.schemaName : "");
                fprintf(stdout, "group: %s\n", conn.group ? conn.group : "");
            }
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

	    G_important_message(_("Default driver / database set to:\n"
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


    if (def->answer) {
	db_set_default_connection();
	db_get_connection(&conn);
	
	G_important_message(_("Default driver / database set to:\n"
			      "driver: %s\ndatabase: %s"), conn.driverName,
			    conn.databaseName);
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
