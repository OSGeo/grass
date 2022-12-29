
/****************************************************************************
 *
 * MODULE:       g.findetc
 * AUTHOR(S):    William Kyngesburye
 * PURPOSE:      Searches for GRASS support files
 * COPYRIGHT:    (C) 2007 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <grass/gis.h>
#include <grass/glocale.h>

int main(int argc, char *argv[])
{
    char name[200];
    char *fpath;
    struct GModule *module;
    struct Option *opt1;

    module = G_define_module();
    G_add_keyword(_("general"));
    G_add_keyword(_("map management"));
    G_add_keyword(_("scripts"));
    module->description = "Searches for GRASS support files.";

    G_gisinit(argv[0]);

    /* Define the different options */

    opt1 = G_define_option();
    opt1->key = "file";
    opt1->type = TYPE_STRING;
    opt1->required = YES;
    opt1->description = "Name of an file or directory";

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    strcpy(name, opt1->answer);

    fpath = G_find_etc(name);
    if (fpath)
	fprintf(stdout, "%s\n", fpath);

    exit(fpath ? EXIT_SUCCESS : EXIT_FAILURE);
}
