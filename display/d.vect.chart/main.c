
/****************************************************************************
 *
 * MODULE:       d.vect.chart
 *
 * AUTHOR(S):    Radim Blazek
 *
 * PURPOSE:      Display charts
 *
 * COPYRIGHT:    (C) 2001-2008 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/display.h>
#include <grass/vector.h>
#include <grass/colors.h>
#include <grass/symbol.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>
#include "global.h"

int main(int argc, char **argv)
{
    char *p;
    int y_center;
    int i, j, ret, type, field, ctype, ncols;
    COLOR ocolor, *colors;
    int r, g, b;
    int size;
    double scale, *max_reference;
    struct GModule *module;
    struct Option *map_opt;
    struct Option *type_opt, *ctype_opt;
    struct Option *size_opt, *scale_opt, *max_reference_opt;
    struct Option *field_opt;
    struct Option *ocolor_opt, *colors_opt;
    struct Option *columns_opt, *sizecol_opt;
    struct Flag *y_center_flag, *legend_flag;

    /*   struct Flag *horizontal_bar_flag; */
    struct Map_info Map;
    char **tokens;
    int ntokens;		/* number of tokens */

    COLOR defcols[] = { {0, 0, 0, 255},	/* blue */
    {0, 0, 255, 255},		/* cyan */
    {0, 0, 255, 0},		/* green */
    {0, 255, 255, 0},		/* yellow */
    {0, 255, 0, 0},		/* red */
    {0, 255, 0, 255},		/* magenta */
    {-1, 0, 0, 0}		/* END */
    };

    module = G_define_module();
    G_add_keyword(_("display"));
    G_add_keyword(_("cartography"));
    G_add_keyword(_("chart maps"));
    module->description =
	_("Displays charts of vector data in the active frame "
	  "on the graphics monitor.");

    map_opt = G_define_standard_option(G_OPT_V_MAP);

    type_opt = G_define_standard_option(G_OPT_V_TYPE);
    type_opt->answer = "point,line,boundary,centroid";

    field_opt = G_define_standard_option(G_OPT_V_FIELD);

    ctype_opt = G_define_option();
    ctype_opt->key = "ctype";
    ctype_opt->type = TYPE_STRING;
    ctype_opt->required = NO;
    ctype_opt->multiple = NO;
    ctype_opt->answer = "pie";
    ctype_opt->options = "pie,bar";
    ctype_opt->description = _("Chart type");
    ctype_opt->guisection = _("Chart properties");

    columns_opt = G_define_option();
    columns_opt->key = "columns";
    columns_opt->type = TYPE_STRING;
    columns_opt->required = YES;
    columns_opt->multiple = YES;
    columns_opt->description = _("Attribute columns containing data");

    sizecol_opt = G_define_option();
    sizecol_opt->key = "sizecol";
    sizecol_opt->type = TYPE_STRING;
    sizecol_opt->required = NO;
    sizecol_opt->description = _("Column used for pie chart size");
    sizecol_opt->guisection = _("Chart properties");

    size_opt = G_define_option();
    size_opt->key = "size";
    size_opt->type = TYPE_INTEGER;
    size_opt->answer = "40";
    size_opt->description =
	_("Size of chart (diameter for pie, total width for bar)");
    size_opt->guisection = _("Chart properties");

    scale_opt = G_define_option();
    scale_opt->key = "scale";
    scale_opt->type = TYPE_DOUBLE;
    scale_opt->answer = "1";
    scale_opt->description = _("Scale for size (to get size in pixels)");
    scale_opt->guisection = _("Chart properties");

    ocolor_opt = G_define_option();
    ocolor_opt->key = "ocolor";
    ocolor_opt->type = TYPE_STRING;
    ocolor_opt->answer = DEFAULT_FG_COLOR;
    ocolor_opt->description = _("Outline color");
    ocolor_opt->gisprompt = "old_color,color,color";
    ocolor_opt->guisection = _("Chart properties");

    colors_opt = G_define_option();
    colors_opt->key = "colors";
    colors_opt->type = TYPE_STRING;
    colors_opt->required = NO;
    colors_opt->multiple = YES;
    colors_opt->description = _("Colors used to fill charts");
    colors_opt->gisprompt = "old_color,color,color";
    colors_opt->guisection = _("Chart properties");

    y_center_flag = G_define_flag();
    y_center_flag->key = 'c';
    y_center_flag->description =
	_("Center the bar chart around a data point");
    y_center_flag->guisection = _("Chart properties");

    max_reference_opt = G_define_option();
    max_reference_opt->key = "max_ref";
    max_reference_opt->type = TYPE_DOUBLE;
    max_reference_opt->required = NO;
    max_reference_opt->multiple = YES;
    max_reference_opt->description =
	_("Maximum value used for bar plot reference");

    legend_flag = G_define_flag();
    legend_flag->key = 'l';
    legend_flag->description =
	_("Create legend information and send to stdout");

    /*
       horizontal_bar_flag = G_define_flag();
       horizontal_bar_flag->key = 'h';
       horizontal_bar_flag->description = _("Create a horizontal bar chart from left to right");
     */

    G_gisinit(argv[0]);

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    /* Center the barchart around the y coordinate?  */
    if (y_center_flag->answer)
	y_center = 1;		/* center the bar graphs around the y_coord of a point */
    else
	y_center = 0;		/* do not center the bar graphs around the y_coord of a point */


    /* Read options */
    type = Vect_option_to_types(type_opt);
    field = atoi(field_opt->answer);

    /* Outline color */
    ret = G_str_to_color(ocolor_opt->answer, &r, &g, &b);
    if (ret == 1) {
	ocolor.none = 0;
	ocolor.r = r;
	ocolor.g = g;
	ocolor.b = b;
    }
    else if (ret == 2) {	/* none */
	ocolor.none = 1;
    }

    /* Count input columns */
    p = columns_opt->answer;
    ncols = 1;
    while ((p = strchr(p, ',')) != NULL) {
	ncols++;
	p++;
    }
    G_debug(3, "ncols = %d", ncols);


    /* Fill colors */
    colors = (COLOR *) G_malloc(ncols * sizeof(COLOR));

    /* Fill max_reference values */
    max_reference = (double *)G_malloc(ncols * sizeof(double));

    /* default colors */
    j = 0;
    for (i = 0; i < ncols; i++) {
	if (defcols[j].none == -1)
	    j = 0;
	colors[i].none = 0;
	colors[i].r = defcols[j].r;
	colors[i].g = defcols[j].g;
	colors[i].b = defcols[j].b;
	j++;
    }
    /* user colors */
    if (colors_opt->answers != NULL) {
	for (i = 0; i < ncols; i++) {
	    if (colors_opt->answers[i] == NULL)
		break;

	    ret = G_str_to_color(colors_opt->answers[i], &r, &g, &b);
	    if (ret == 1) {
		colors[i].none = 0;
		colors[i].r = r;
		colors[i].g = g;
		colors[i].b = b;
	    }
	    else if (ret == 2) {	/* none */
		colors[i].none = 1;
	    }
	}
    }

    if (legend_flag->answer) {
	tokens = G_tokenize(columns_opt->answer, ",");
	ntokens = G_number_of_tokens(tokens);

	for (i = 0; i < ntokens; i++) {
	    fprintf(stdout, "%d|%s|%d:%d:%d\n",
		    i + 1, tokens[i], colors[i].r, colors[i].g, colors[i].b);
	}
    }

    size = atoi(size_opt->answer);
    scale = atof(scale_opt->answer);

    /* open vector */
    Vect_set_open_level(2);
    Vect_open_old(&Map, map_opt->answer, "");

    ctype = CTYPE_PIE;
    if (ctype_opt->answer[0] == 'b')
	ctype = CTYPE_BAR;

    if (D_open_driver() != 0)
	G_fatal_error(_("No graphics device selected. "
			"Use d.mon to select graphics device."));
    
    /* should we plot the maximum reference on bar plots? */
    if (max_reference_opt->answer != NULL) {

	/* loop through the given values */
	for (i = 0; i < ncols; i++) {
	    if (max_reference_opt->answers[i] == NULL)
		break;

	    max_reference[i] = atof(max_reference_opt->answers[i]);	/* remember to convert to float */
	}
    }


    D_setup(0);

    ret = plot(ctype, &Map, type, field,
	       columns_opt->answer, ncols,
	       sizecol_opt->answer, size, scale,
	       &ocolor, colors, y_center, max_reference);

    D_save_command(G_recreate_command());
    D_close_driver();

    Vect_close(&Map);

    exit(EXIT_SUCCESS);
}
