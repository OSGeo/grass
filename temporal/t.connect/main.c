
/****************************************************************************
 *
 * MODULE:       t.connect
 * AUTHOR(S):    Soeren Gebbert, based on db.connect
 *
 * PURPOSE:      Prints/sets general temporal GIS database connection for current mapset.
 * COPYRIGHT:    (C) 2002-2010 by the GRASS Development Team
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
#include <grass/temporal.h>
#include <grass/glocale.h>

int main(int argc, char *argv[])
{
    dbConnection conn;
    struct Flag *print, *check_set_default, *def, *sh;
    struct Option *driver, *database;
    struct GModule *module;

    /* Initialize the GIS calls */
    G_gisinit(argv[0]);

    /* Set description */
    module = G_define_module();
    G_add_keyword(_("database"));
    G_add_keyword(_("attribute table"));
    G_add_keyword(_("connection settings"));
    module->description =
	_("Prints/sets general temporal GIS database connection for current mapset.");

    print = G_define_flag();
    print->key = 'p';
    print->description = _("Print current connection parameters and exit");
    print->guisection = _("Print");

    check_set_default = G_define_flag();
    check_set_default->key = 'c';
    check_set_default->description =
	_("Check connection parameters, set if uninitialized, and exit");
    check_set_default->guisection = _("Set");
    
    def = G_define_flag();
    def->key = 'd';
    def->label = _("Set from default settings and exit");
    def->description = _("Overwrite current settings if initialized");
    def->guisection = _("Set");
    
    sh = G_define_flag();
    sh->key = 'g';
    sh->description = _("Print current connection parameter in shell style and exit");
    sh->guisection = _("Set");

    driver = G_define_standard_option(G_OPT_DB_DRIVER);
    driver->options = "sqlite,pg";
    driver->answer = (char *) tgis_get_default_driver_name();
    driver->guisection = _("Set");

    database = G_define_standard_option(G_OPT_DB_DATABASE);
    database->answer = (char *) tgis_get_default_database_name();
    database->guisection = _("Set");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    if (print->answer) {
        if(sh->answer) {
            if (tgis_get_connection(&conn) == DB_OK) {
                fprintf(stdout, "driver=%s\n",
                        conn.driverName ? conn.driverName : "");
                fprintf(stdout, "database=%s\n",
                        conn.databaseName ? conn.databaseName : "");
            }
            else
                G_fatal_error(_("Temporal GIS database connection not defined. "
                                "Run t.connect."));

        } else {
	/* get and print connection */
            if (tgis_get_connection(&conn) == DB_OK) {
                fprintf(stdout, "driver:%s\n",
                        conn.driverName ? conn.driverName : "");
                fprintf(stdout, "database:%s\n",
                        conn.databaseName ? conn.databaseName : "");
            }
            else
                G_fatal_error(_("Temporal GIS database connection not defined. "
                                "Run t.connect."));
        }

	exit(EXIT_SUCCESS);
    }

    if (check_set_default->answer) {
	/* check connection and set to system-wide default in required */
	tgis_get_connection(&conn);

	if (!conn.driverName && !conn.databaseName) {

	    tgis_set_default_connection();
	    tgis_get_connection(&conn);

	    G_important_message(_("Default TGIS driver / database set to:\n"
				  "driver: %s\ndatabase: %s"), conn.driverName,
				conn.databaseName);
	}
	/* they must be a matched pair, so if one is set but not the other
	   then give up and let the user figure it out */
	else if (!conn.driverName) {
	    G_fatal_error(_("Default TGIS driver is not set"));
	}
	else if (!conn.databaseName) {
	    G_fatal_error(_("Default TGIS database is not set"));
	}

	/* connection either already existed or now exists */
	exit(EXIT_SUCCESS);
    }


    if (def->answer) {
	tgis_set_default_connection();
	tgis_get_connection(&conn);
	
	G_important_message(_("Default driver / database set to:\n"
			      "driver: %s\ndatabase: %s"), conn.driverName,
			    conn.databaseName);
	exit(EXIT_SUCCESS);
    }
    
    /* set connection */
    tgis_get_connection(&conn);	/* read current */

    if (driver->answer)
	conn.driverName = driver->answer;

    if (database->answer)
	conn.databaseName = database->answer;

    tgis_set_connection(&conn);

    exit(EXIT_SUCCESS);
}
