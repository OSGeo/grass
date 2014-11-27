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
    char *colorlist, *sep_str;
    int i;

    /* Initialize the GIS calls */
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("display"));
    G_add_keyword(_("settings"));
    G_add_keyword(_("colors"));
    module->description =
	_("Outputs a list of all available display colors.");

    /* set up option */
    sep = G_define_standard_option(G_OPT_F_SEP);
    sep->answer = "comma";
    
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    sep_str = G_option_to_separator(sep);
    
    colorlist = G_store(D_COLOR_LIST);

    /* if separator is different from ",", escape this character */
    for (i = 0; colorlist[i] != '\0'; i++) {
        if (colorlist[i] == ',') {
            fprintf(stdout, "%s", sep_str);
            continue;
        }
        fprintf(stdout, "%c", colorlist[i]);
    }
    fprintf(stdout, "\n");
    
    exit(EXIT_SUCCESS);
}
