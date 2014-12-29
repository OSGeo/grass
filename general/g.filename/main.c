
/****************************************************************************
 *
 * MODULE:       g.filename
 * AUTHOR(S):    Michael Shapiro CERL (original contributor)
 *               Markus Neteler <neteler itc.it>,
 *               Bernhard Reiter <bernhard intevation.de>, 
 *               Glynn Clements <glynn gclements.plus.com>,
 *               Hamish Bowman <hamish_b yahoo.com>,
 *               Jan-Oliver Wagner <jan intevation.de>
 * PURPOSE:      Prints GRASS data base file names
 * COPYRIGHT:    (C) 1999-2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>

int main(int argc, char *argv[])
{
    char path[1024];
    const char *element;
    const char *mapset;
    const char *name;
    struct GModule *module;
    struct Option *opt1;
    struct Option *opt2;
    struct Option *opt3;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("general"));
    G_add_keyword(_("map management"));
    G_add_keyword(_("scripts"));
    module->description = _("Prints GRASS data base file names.");

    /* Define the different options */

    opt1 = G_define_option();
    opt1->key = "element";
    opt1->type = TYPE_STRING;
    opt1->required = YES;
    opt1->description = _("Name of an element");

    opt3 = G_define_option();
    opt3->key = "file";
    opt3->type = TYPE_STRING;
    opt3->required = YES;
    opt3->description = _("Name of a database file");

    opt2 = G_define_option();
    opt2->key = "mapset";
    opt2->type = TYPE_STRING;
    opt2->required = NO;
    opt2->description = _("Name of a mapset (default: current)");


    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    element = opt1->answer;
    name = opt3->answer;

    if (opt2->answer)
	mapset = opt2->answer;
    else
	mapset = G_mapset();

    if (strcmp(mapset, ".") == 0 || strcmp(mapset, "") == 0)
	mapset = G_mapset();

    G_make_mapset_element(element);
    G_file_name(path, element, name, mapset);

    fprintf(stdout, "file='%s'\n", path);
    exit(EXIT_SUCCESS);
}
