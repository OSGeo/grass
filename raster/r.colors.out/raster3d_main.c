
/****************************************************************************
 *
 * MODULE:       r.colors.out
 *
 * AUTHOR(S):    Glynn Clements
 *
 * PURPOSE:      Allows export of the color table for a raster map layer.
 *
 * COPYRIGHT:    (C) 2008, 2010 Glynn Clements and the GRASS Development Team
 *
 *               This program is free software under the GNU General
 *               Public License (>=v2). Read the file COPYING that
 *               comes with GRASS for details.
 *
 ***************************************************************************/

#include "local_proto.h"

/* Run in raster3d mode */
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
    struct Colors colors;
    struct FPRange range;
    
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster3d"));
    G_add_keyword(_("color table"));
    G_add_keyword(_("export"));
    module->description =
    _("Exports the color table associated with a raster3d map layer.");

    opt.map = G_define_standard_option(G_OPT_R3_MAP);
    opt.file = G_define_standard_option(G_OPT_F_OUTPUT);
    opt.file->key = "rules";
    opt.file->label = _("Path to output rules file");
    opt.file->description = _("\"-\" to write to stdout");
    opt.file->answer = "-";
    
    flag.p = G_define_flag();
    flag.p->key = 'p';
    flag.p->description = _("Output values as percentages");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    if (Rast3d_read_colors(opt.map->answer, "", &colors) < 0)
        G_fatal_error(_("Unable to read color table for raster3d map <%s>"), opt.map->answer);
    Rast3d_read_range(opt.map->answer, "", &range);

    write_colors(&colors, &range, opt.file->answer, flag.p->answer ? 1 : 0);

    exit(EXIT_SUCCESS);
}
