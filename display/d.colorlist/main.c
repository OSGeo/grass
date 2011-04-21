/*
 ****************************************************************************
 *
 * MODULE:       d.colorlist
 * AUTHOR(S):    Andreas Lange - andreas.lange@rhein-main.de
 * PURPOSE:      Output a list of all available colors with a configurable
 *               separator (default is comma). 
 *               Used for scripting and tcl/tk-scripts to
 *               build a list of available options.
 * COPYRIGHT:    (C) 2000 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <string.h>
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/colors.h>

int main(int argc, char **argv)
{
    struct Option *sep;
    struct GModule *module;
    char *colorlist;
    int i;

    /* Initialize the GIS calls */
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("display"));
    G_add_keyword(_("setup"));
    module->description =
	"Output a list of all available display colors with a configurable "
	"separator (default is comma).";

    /* set up option */
    sep = G_define_option();
    sep->key = "fs";
    sep->type = TYPE_STRING;
    sep->required = NO;
    sep->description = "character for separation of list items";
    sep->answer = ",";

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    colorlist = G_store(D_COLOR_LIST);

    /* if separator is different from ",", escape this character */
    if (strcmp(sep->answer, ",") != 0 && strlen(sep->answer) > 0) {
	for (i = 0; colorlist[i] != '\0'; i++)
	    if (colorlist[i] == ',')
		colorlist[i] = (char)sep->answer[0];
    }

    fprintf(stdout, "%s\n", colorlist);
    return 0;
}
