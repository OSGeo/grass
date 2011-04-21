
/****************************************************************************
 *
 * MODULE:       r.what
 * AUTHOR(S):    Michael Shapiro, CERL (original contributor)
 *               Markus Neteler <neteler itc.it>,Brad Douglas <rez touchofmadness.com>,
 *               Huidae Cho <grass4u gmail.com>, Glynn Clements <glynn gclements.plus.com>,
 *               Hamish Bowman <hamish_b yahoo.com>, Soeren Gebbert <soeren.gebbert gmx.de>
 * PURPOSE:      
 * COPYRIGHT:    (C) 1999-2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

static const char *fmt;

static int do_value(const char *buf, RASTER_MAP_TYPE type,
		    struct Colors *colors)
{
    CELL ival;
    DCELL fval;
    int red, grn, blu;

    switch (type) {
    case CELL_TYPE:
	if (sscanf(buf, "%d", &ival) != 1) {
	    fprintf(stdout, "*: *\n");
	    return 0;
	}
	if (!Rast_get_c_color(&ival, &red, &grn, &blu, colors)) {
	    fprintf(stdout, "%d: *\n", ival);
	    return 0;
	}
	fprintf(stdout, "%d: ", ival);
	fprintf(stdout, fmt, red, grn, blu);
	fprintf(stdout, "\n");
	return 1;

    case FCELL_TYPE:
    case DCELL_TYPE:
	if (sscanf(buf, "%lf", &fval) != 1) {
	    fprintf(stdout, "*: *\n");
	    return 0;
	}
	if (!Rast_get_d_color(&fval, &red, &grn, &blu, colors)) {
	    fprintf(stdout, "%.15g: *\n", fval);
	    return 0;
	}
	fprintf(stdout, "%.15g: ", fval);
	fprintf(stdout, fmt, red, grn, blu);
	fprintf(stdout, "\n");
	return 1;
    default:
	G_fatal_error("Invalid map type %d", type);
	return 0;
    }
}

int main(int argc, char **argv)
{
    struct GModule *module;
    struct
    {
	struct Option *input, *value, *format;
    } opt;
    struct
    {
	struct Flag *i;
    } flag;
    const char *name;
    struct Colors colors;
    RASTER_MAP_TYPE type;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("querying"));
    G_add_keyword(_("color table"));
    module->description = _("Queries colors for a raster map layer.");

    opt.input = G_define_option();
    opt.input->key = "input";
    opt.input->type = TYPE_STRING;
    opt.input->required = YES;
    opt.input->multiple = NO;
    opt.input->gisprompt = "old,cell,raster";
    opt.input->description = _("Name of existing raster map to query colors");

    opt.value = G_define_option();
    opt.value->key = "value";
    opt.value->type = TYPE_DOUBLE;
    opt.value->required = NO;
    opt.value->multiple = YES;
    opt.value->description = _("Values to query colors for");

    opt.format = G_define_option();
    opt.format->key = "format";
    opt.format->type = TYPE_STRING;
    opt.format->required = NO;
    opt.format->answer = "%d:%d:%d";
    opt.format->description = _("Output format (printf-style)");

    flag.i = G_define_flag();
    flag.i->key = 'i';
    flag.i->description = _("Read values from stdin");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    if (!opt.value->answer && !flag.i->answer)
	G_fatal_error(_("Either \"-i\" or \"value=\" must be given"));

    name = opt.input->answer;

    type = Rast_map_type(name, "");
    if (type < 0)
	G_fatal_error("Unable to determine type of input map %s", name);

    if (Rast_read_colors(name, "", &colors) < 0)
	G_fatal_error("Unable to read colors for input map %s", name);

    fmt = opt.format->answer;

    if (flag.i->answer) {
	for (;;) {
	    char buf[64];

	    if (!fgets(buf, sizeof(buf), stdin))
		break;

	    do_value(buf, type, &colors);
	}
    }
    else if (opt.value->answer) {
	const char *ans;
	int i;

	for (i = 0; ans = opt.value->answers[i], ans; i++)
	    do_value(ans, type, &colors);
    }

    return EXIT_SUCCESS;
}
