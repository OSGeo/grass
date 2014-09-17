
/****************************************************************************
 *
 * MODULE:       v.colors.out
 *
 * AUTHOR(S):    Modified r.colors.out by Martin Landa <landa.martin gmail.com>
 *
 * PURPOSE:      Allows export of the color table for a vector map.
 *
 * COPYRIGHT:    (C) 2011 by the GRASS Development Team
 *
 *               This program is free software under the GNU General
 *               Public License (>=v2). Read the file COPYING that
 *               comes with GRASS for details.
 *
 ***************************************************************************/

#include <stdlib.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/vector.h>
#include <grass/glocale.h>

#include "local_proto.h"

int main(int argc, char **argv)
{    
    struct GModule *module;
    struct
    {
	struct Option *map, *field, *file, *col;
    } opt;
    struct
    {
	struct Flag *p;
    } flag;
    struct Colors cat_colors, *colors;
    
    int ret;
    int min, max;
    const char *file, *name, *layer, *column;
    FILE *fp;
    
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("color table"));
    G_add_keyword(_("export"));
    module->description =
	_("Exports the color table associated with a vector map.");
   
    opt.map = G_define_standard_option(G_OPT_V_MAP);
    
    opt.field = G_define_standard_option(G_OPT_V_FIELD);
    
    opt.file = G_define_standard_option(G_OPT_F_OUTPUT);
    opt.file->key = "rules";
    opt.file->label = _("Path to output rules file");
    opt.file->description = _("If not given write to standard output");
    opt.file->required = NO;

    opt.col = G_define_standard_option(G_OPT_DB_COLUMN);
    opt.col->label = _("Name of attribute (numeric) column to which refer color rules");
    opt.col->description = _("If not given, color rules refer to categories");
    opt.col->guisection = _("Settings");

    flag.p = G_define_flag();
    flag.p->key = 'p';
    flag.p->description = _("Output values as percentages");
    flag.p->guisection = _("Settings");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    name = opt.map->answer;
    layer = opt.field->answer;
    file = opt.file->answer;
    column = opt.col->answer;
    
    ret = Vect_read_colors(name, "", &cat_colors);
    if (ret < 0)
	G_fatal_error(_("Unable to read color table for vector map <%s>"),
		      opt.map->answer);
    if (ret == 0) {
	G_warning(_("No color table defined for vector map <%s>"),
		  opt.map->answer);
	exit(EXIT_SUCCESS);
    }

    min = max = -1;
    if (flag.p->answer) {
	scan_cats(name, layer, &min, &max);
    }
    
    if (!file || strcmp(file, "-") == 0)
	fp = stdout;
    else {
	fp = fopen(file, "w");
	if (!fp)
	    G_fatal_error(_("Unable to open output file <%s>"), file);
    }

    if (column)
	colors = make_colors(name, layer, column, &cat_colors);
    else
	colors = &cat_colors;
    
    Rast_print_colors(colors, (DCELL) min, (DCELL) max, fp,
		      flag.p->answer ? 1 : 0);
    
    exit(EXIT_SUCCESS);
}
