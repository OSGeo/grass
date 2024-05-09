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
    struct Flag *flag_create;

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

    flag_create = G_define_flag();
    flag_create->key = 'c';
    flag_create->label = _("Create element directory in the current mapset");
    flag_create->description =
        _("If element directory for database file does not exist, create it");

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
<<<<<<< HEAD

    /* Create element directory if requested and in current mapset. */
    if (flag_create) {
        if (strcmp(mapset, G_mapset()) != 0) {
            G_fatal_error("Cannot create <%s> (flag -%c):"
                          " <%s> is not the current mapset (%s)",
                          element, flag_create->key, mapset, G_mapset());
        }
        G_make_mapset_object_group(element);
<<<<<<< HEAD
    }

<<<<<<< HEAD
=======
    /* Create element directory if requested and in current mapset. */
    if (flag_create) {
        if (strcmp(mapset, G_mapset()) != 0) {
            G_fatal_error("Cannot create <%s> (flag -%c):"
                          " <%s> is not the current mapset (%s)",
                          element, flag_create->key, mapset, G_mapset());
        }
        G_make_mapset_element(element);
=======
>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))
    }

>>>>>>> 392c6e8e0b (pygrass: Add update parameters method to Module (#1712))
=======

    /* Create element directory if requested and in current mapset. */
    if (flag_create) {
        if (strcmp(mapset, G_mapset()) != 0) {
            G_fatal_error("Cannot create <%s> (flag -%c):"
                          " <%s> is not the current mapset (%s)",
                          element, flag_create->key, mapset, G_mapset());
        }
        G_make_mapset_object_group(element);
    }

>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
    G_file_name(path, element, name, mapset);

    fprintf(stdout, "file='%s'\n", path);
    exit(EXIT_SUCCESS);
}
