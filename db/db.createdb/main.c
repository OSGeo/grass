
/****************************************************************************
 *
 * MODULE:       db.createdb
 * AUTHOR(S):    Radim Blazek <radim.blazek gmail.com> (original contributor)
 *               Glynn Clements <glynn gclements.plus.com>, Markus Neteler <neteler itc.it>
 * PURPOSE:      create a new empty database
 * COPYRIGHT:    (C) 2002-2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <grass/dbmi.h>
#include <grass/gis.h>
#include <grass/glocale.h>


struct
{
    char *driver, *database;
} parms;


/* function prototypes */
static void parse_command_line(int, char **);


int main(int argc, char **argv)
{
    dbDriver *driver;
    dbHandle handle;
    int stat;

    parse_command_line(argc, argv);

    driver = db_start_driver(parms.driver);
    if (driver == NULL)
	G_fatal_error(_("Unable to start driver <%s>"), parms.driver);

    db_init_handle(&handle);
    db_set_handle(&handle, parms.database, NULL);
    stat = db_create_database(driver, &handle);
    db_shutdown_driver(driver);

    exit(stat == DB_OK ? EXIT_SUCCESS : EXIT_FAILURE);
}


static void parse_command_line(int argc, char **argv)
{
    struct Option *driver, *database;
    struct GModule *module;

    /* Initialize the GIS calls */
    G_gisinit(argv[0]);

    driver = G_define_standard_option(G_OPT_DB_DRIVER);
    driver->options = db_list_drivers();
    driver->required = YES;
    driver->answer = (char *) db_get_default_driver_name();
    
    database = G_define_standard_option(G_OPT_DB_DATABASE);
    database->required = YES;

    /* Set description */
    module = G_define_module();
    G_add_keyword(_("database"));
    G_add_keyword(_("attribute table"));
    G_add_keyword(_("SQL"));
    module->description = _("Creates an empty database.");
    
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    parms.driver = driver->answer;
    parms.database = database->answer;
}
