/****************************************************************************
 *
 * MODULE:       r.colors.out
 *
 * AUTHOR(S):    Glynn Clements
 *
 * PURPOSE:      Allows export of the color table for a raster map.
 *
 * COPYRIGHT:    (C) 2008, 2010-2011 Glynn Clements and the GRASS Development
 *               Team
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
#include <grass/parson.h>

#include "local_proto.h"

/* Run in raster mode */
int main(int argc, char **argv)
{
    struct GModule *module;
    struct {
        struct Option *map, *file, *format, *color_format;
    } opt;
    struct {
        struct Flag *p;
    } flag;

    const char *file;
    FILE *fp;
    struct Colors colors;
    struct FPRange range;

    enum ColorFormat clr_frmt;

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

    opt.format = G_define_standard_option(G_OPT_F_FORMAT);
    opt.format->guisection = _("Print");

    opt.color_format = G_define_option();
    opt.color_format->key = "color_format";
    opt.color_format->type = TYPE_STRING;
    opt.color_format->key_desc = "name";
    opt.color_format->required = YES;
    opt.color_format->multiple = NO;
    opt.color_format->answer = "xterm";
    opt.color_format->options = "rgb,hex,hsv,xterm";
    opt.color_format->label = _("Color format");
    opt.color_format->description = _("Color format output for raster values.");
    char *desc = NULL;
    G_asprintf(&desc, "rgb;%s;hex;%s;hsv;%s;xterm;%s",
               _("output color in RGB format"), _("output color in HEX format"),
               _("output color in HSV format"),
               _("output color in XTERM format"));
    opt.color_format->descriptions = desc;
    opt.color_format->guisection = _("Color");

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

    if (strcmp(opt.format->answer, "json") == 0) {
        if (strcmp(opt.color_format->answer, "rgb") == 0) {
            clr_frmt = RGB;
        }
        else if (strcmp(opt.color_format->answer, "hex") == 0) {
            clr_frmt = HEX;
        }
        else if (strcmp(opt.color_format->answer, "hsv") == 0) {
            clr_frmt = HSV;
        }
        else {
            clr_frmt = XTERM;
        }
        print_json_colors(&colors, range.min, range.max, fp,
                          flag.p->answer ? 1 : 0, clr_frmt);
    }
    else {
        Rast_print_colors(&colors, range.min, range.max, fp,
                          flag.p->answer ? 1 : 0);
    }

    exit(EXIT_SUCCESS);
}
