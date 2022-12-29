
/****************************************************************************
 *
 * MODULE:       db.drivers
 * AUTHOR(S):    Radim Blazek <radim.blazek gmail.com> (original contributor)
 *               Glynn Clements <glynn gclements.plus.com>, Markus Neteler <neteler itc.it>, Stephan Holl
 * PURPOSE:      lists all database drivers
 * COPYRIGHT:    (C) 2002-2006, 2012 by the GRASS Development Team
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
    int f;
} parms;

/* function prototypes */
static void parse_command_line(int, char **);


int main(int argc, char **argv)
{
    dbDbmscap *list, *p;

    parse_command_line(argc, argv);

    list = db_read_dbmscap();
    if (list == NULL) {
	G_fatal_error(_("Unable to read dbmscap file"));
    }

    for (p = list; p; p = p->next) {
	fprintf(stdout, "%s", p->driverName);
	if (parms.f)
	    fprintf(stdout, ":%s", p->comment);
	fprintf(stdout, "\n");
    }

    exit(EXIT_SUCCESS);
}


static void parse_command_line(int argc, char **argv)
{
    struct Flag *full, *print;
    struct GModule *module;

    /* Initialize the GIS calls */
    G_gisinit(argv[0]);

    full = G_define_flag();
    full->key = 'f';
    full->description = _("Full output");

    print = G_define_flag();
    print->key = 'p';
    print->description = _("Print drivers and exit");

    /* Set description */
    module = G_define_module();
    G_add_keyword(_("database"));
    G_add_keyword(_("connection settings"));
    module->description = _("Lists all database drivers.");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    parms.f = full->answer;
}
