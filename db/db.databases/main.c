
/****************************************************************************
 *
 * MODULE:       db.databases
 * AUTHOR(S):    Radim Blazek <radim.blazek gmail.com> (original contributor)
 *               Glynn Clements <glynn gclements.plus.com>, Markus Neteler <neteler itc.it>
 * PURPOSE:      lists all databases for a given driver
 * COPYRIGHT:    (C) 2002-2006, 2012 by the GRASS Development Team
 *
 *               This program is free software under the GNU General
 *               Public License (>=v2). Read the file COPYING that
 *               comes with GRASS for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <grass/dbmi.h>
#include <grass/gis.h>
#include <grass/glocale.h>

struct
{
    char *driver;
    char *project;
} parms;

/* function prototypes */
static void parse_command_line(int, char **);

int main(int argc, char **argv)
{
    dbDriver *driver;
    dbHandle *handles;
    dbString projects;
    int nlocs = 0;
    int count, i;

    db_init_string(&projects);
    parse_command_line(argc, argv);

    if (parms.project) {
	db_set_string(&projects, parms.project);
	nlocs = 1;
    }

    driver = db_start_driver(parms.driver);
    if (driver == NULL)
	G_fatal_error(_("Unable to start driver <%s>"), parms.driver);

    if (db_list_databases(driver, &projects,
                          nlocs, &handles, &count) != DB_OK) {
        db_shutdown_driver(driver);
        G_fatal_error(_("Unable to list databases. "
                        "Try to define correct connection settings by db.login."));
    }
    db_shutdown_driver(driver);

    for (i = 0; i < count; i++) {
	fprintf(stdout, "%s\n", db_get_handle_dbname(&handles[i]));
    }

    if (count < 1)
        G_important_message(_("No databases found"));
                            
    exit(EXIT_SUCCESS);
}


static void parse_command_line(int argc, char **argv)
{
    struct Option *driver, *project;
    struct GModule *module;

    /* Initialize the GIS calls */
    G_gisinit(argv[0]);

    driver = G_define_standard_option(G_OPT_DB_DRIVER);
    driver->options = db_list_drivers();
    driver->answer = (char *) db_get_default_driver_name();
    driver->guisection = _("Connection");
    
    project = G_define_option();
    project->key = "project";
    project->type = TYPE_STRING;
    project->required = NO;
    /* project->multiple = YES; ? */
    project->label = _("Project");
    project->description = _("Path for SQLite driver, or connection string "
                              "for PostgreSQL driver");
    project->key_desc = "name";
    project->guisection = _("Connection");

    /* Set description */
    module = G_define_module();
    G_add_keyword(_("database"));
    G_add_keyword(_("attribute table"));
    G_add_keyword(_("SQL"));
    module->description =
	_("Lists all databases for a given driver and project.");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    parms.driver = driver->answer;
    parms.project = project->answer ? project->answer : "";
}
