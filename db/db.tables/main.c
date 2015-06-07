
/****************************************************************************
 *
 * MODULE:       db.tables
 * AUTHOR(S):    Radim Blazek <radim.blazek gmail.com> (original contributor)
 *               Glynn Clements <glynn gclements.plus.com>, Markus Neteler <neteler itc.it>, Stephan Holl
 * PURPOSE:      lists all tables for a given database
 * COPYRIGHT:    (C) 2002-2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

struct
{
    char *driver, *database;
    int s;
} parms;

/* function prototypes */
static void parse_command_line(int, char **);

int main(int argc, char **argv)
{
    dbDriver *driver;
    dbHandle handle;
    dbString *names;
    int i, count;
    int system_tables;

    parse_command_line(argc, argv);

    if (!parms.driver)
        G_fatal_error(_("Database connection not defined. Run db.connect."));
      
    driver = db_start_driver(parms.driver);
    if (driver == NULL)
	G_fatal_error(_("Unable to start driver <%s>"), parms.driver);

    db_init_handle(&handle);
    db_set_handle(&handle, parms.database, NULL);
    if (db_open_database(driver, &handle) != DB_OK)
	G_fatal_error(_("Unable to open database <%s>"), parms.database);

    system_tables = parms.s;
    if (db_list_tables(driver, &names, &count, system_tables) != DB_OK)
        G_fatal_error(_("Unable to list tables from database <%s>"), parms.database);
    
    for (i = 0; i < count; i++)
	fprintf(stdout, "%s\n", db_get_string(&names[i]));

    if (count < 1)
        G_important_message(_("No tables found"));

    db_close_database(driver);
    db_shutdown_driver(driver);

    exit(EXIT_SUCCESS);
}

static void parse_command_line(int argc, char **argv)
{
    struct Option *driver, *database;
    struct Flag *p, *s;
    struct GModule *module;
    const char *drv, *db;

    /* Initialize the GIS calls */
    G_gisinit(argv[0]);

    driver = G_define_standard_option(G_OPT_DB_DRIVER);
    driver->options = db_list_drivers();
    if ((drv = db_get_default_driver_name()))
	driver->answer = (char *) drv;

    database = G_define_standard_option(G_OPT_DB_DATABASE);
    if ((db = db_get_default_database_name()))
	database->answer = (char *) db;

    p = G_define_flag();
    p->key = 'p';
    p->description = _("Print tables and exit");

    s = G_define_flag();
    s->key = 's';
    s->description = _("System tables instead of user tables");

    /* Set description */
    module = G_define_module();
    G_add_keyword(_("database"));
    G_add_keyword(_("attribute table"));
    module->description = _("Lists all tables for a given database.");

    if (G_parser(argc, argv))
	exit(EXIT_SUCCESS);

    parms.driver = driver->answer;
    parms.database = database->answer;
    parms.s = s->answer;
}
