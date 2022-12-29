
/****************************************************************************
 *
 * MODULE:       r.colors.out
 *
 * AUTHOR(S):    Glynn Clements
 *
 * PURPOSE:      Allows export of the color table for a raster map.
 *
 * COPYRIGHT:    (C) 2008, 2010-2011 Glynn Clements and the GRASS Development Team
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
#include <grass/glocale.h>

/* Run in raster mode */
int main(int argc, char **argv)
{    
    struct GModule *module;
    struct
    {
	struct Option *map, *file;
    } opt;
    struct
    {
	struct Flag *p;
    } flag;

    const char *file;
    FILE * fp;
    struct Colors colors;
    struct FPRange range;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("color table"));
    G_add_keyword(_("export"));
    module->description =
	_("Exports the color table associated with a raster map.");
   
    opt.map = G_define_standard_option(G_OPT_R_MAP);
    
    opt.file = G_define_standard_option(G_OPT_F_OUTPUT);
    opt.file->key = "rules";
    opt.file->label = _("Path to output rules file");
    opt.file->description = _("If not given write to standard output");
    opt.file->required = NO;

    flag.p = G_define_flag();
    flag.p->key = 'p';
    flag.p->description = _("Output values as percentages");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    file = opt.file->answer;
    
    if (Rast_read_colors(opt.map->answer, "", &colors) < 0)
        G_fatal_error(_("Unable to read color table for raster map <%s>"),
		      opt.map->answer);

    Rast_read_fp_range(opt.map->answer, "", &range);
    
    if (!file || strcmp(file, "-") == 0)
	fp = stdout;
    else {
	fp = fopen(file, "w");
	if (!fp)
	    G_fatal_error(_("Unable to open output file <%s>"), file);
    }
    
    Rast_print_colors(&colors, range.min, range.max, fp,
		      flag.p->answer ? 1 : 0);
    
    exit(EXIT_SUCCESS);
}
