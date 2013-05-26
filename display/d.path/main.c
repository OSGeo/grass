
/****************************************************************
 *
 * MODULE:       d.path
 * 
 * AUTHOR(S):    Radim Blazek
 *               
 * PURPOSE:      shortest path networking on vector map
 *               Uses the DGLib from Roberto Micarelli
 *               
 * COPYRIGHT:    (C) 2002 by the GRASS Development Team
 *
 *               This program is free software under the 
 *               GNU General Public License (>=v2). 
 *               Read the file COPYING that comes with GRASS
 *               for details.
 *
 ****************************************************************/

#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/display.h>
#include <grass/colors.h>
#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>
#include "proto.h"

int main(int argc, char **argv)
{
    struct Option *map, *afield_opt, *nfield_opt, *afcol, *abcol, *ncol,
	*type_opt;
    struct Option *color_opt, *hcolor_opt, *bgcolor_opt, *coor_opt;
    struct Flag *geo_f, *bold_f;
    struct GModule *module;
    struct Map_info Map;
    int type, afield, nfield, geo;
    struct color_rgb color, hcolor, bgcolor;
    int r, g, b;
    double x1, y1, x2, y2;

    /* Initialize the GIS calls */
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("display"));
    G_add_keyword(_("network"));
    G_add_keyword(_("shortest path"));
    module->description =
	_("Finds shortest path for selected starting and ending node.");

    map = G_define_standard_option(G_OPT_V_MAP);

    type_opt = G_define_standard_option(G_OPT_V_TYPE);
    type_opt->options = "line,boundary";
    type_opt->answer = "line,boundary";
    type_opt->description = _("Arc type");

    coor_opt = G_define_option();
    coor_opt->key = "coor";
    coor_opt->key_desc = "x1,y1,x2,y2";
    coor_opt->type = TYPE_STRING;
    coor_opt->required = YES;
    coor_opt->description = _("Starting and ending coordinates");

    afield_opt = G_define_standard_option(G_OPT_V_FIELD);
    afield_opt->key = "alayer";
    afield_opt->answer = "1";
    afield_opt->description = _("Arc layer");

    nfield_opt = G_define_standard_option(G_OPT_V_FIELD);
    nfield_opt->key = "nlayer";
    nfield_opt->answer = "2";
    nfield_opt->description = _("Node layer");

    afcol = G_define_option();
    afcol->key = "afcol";
    afcol->type = TYPE_STRING;
    afcol->required = NO;
    afcol->description = _("Arc forward/both direction(s) cost column");

    abcol = G_define_option();
    abcol->key = "abcol";
    abcol->type = TYPE_STRING;
    abcol->required = NO;
    abcol->description = _("Arc backward direction cost column");

    ncol = G_define_option();
    ncol->key = "ncol";
    ncol->type = TYPE_STRING;
    ncol->required = NO;
    ncol->description = _("Node cost column");

    color_opt = G_define_option();
    color_opt->key = "color";
    color_opt->type = TYPE_STRING;
    color_opt->answer = DEFAULT_FG_COLOR;
    color_opt->description = _("Original line color");
    color_opt->gisprompt = "old_color,color,color";
    color_opt->guisection = _("Rendering");

    hcolor_opt = G_define_option();
    hcolor_opt->key = "hcolor";
    hcolor_opt->type = TYPE_STRING;
    hcolor_opt->answer = "red";
    hcolor_opt->description = _("Highlight color");
    hcolor_opt->gisprompt = "old_color,color,color";
    hcolor_opt->guisection = _("Rendering");

    bgcolor_opt = G_define_option();
    bgcolor_opt->key = "bgcolor";
    bgcolor_opt->type = TYPE_STRING;
    bgcolor_opt->answer = DEFAULT_BG_COLOR;
    bgcolor_opt->description = _("Background color");
    bgcolor_opt->gisprompt = "old_color,color,color";
    bgcolor_opt->guisection = _("Rendering");

    geo_f = G_define_flag();
    geo_f->key = 'g';
    geo_f->description =
	_("Use geodesic calculation for longitude-latitude locations");

    bold_f = G_define_flag();
    bold_f->key = 'b';
    bold_f->description = _("Render bold lines");
    bold_f->guisection = _("Rendering");


    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);


    type = Vect_option_to_types(type_opt);
    afield = atoi(afield_opt->answer);
    nfield = atoi(nfield_opt->answer);


    if (coor_opt->answers[0] == NULL)
	G_fatal_error(_("No coordinates given"));

    if (!G_scan_easting(coor_opt->answers[0], &x1, G_projection()))
	G_fatal_error(_("%s - illegal x value"), coor_opt->answers[0]);
    if (!G_scan_northing(coor_opt->answers[1], &y1, G_projection()))
	G_fatal_error(_("%s - illegal y value"), coor_opt->answers[1]);
    if (!G_scan_easting(coor_opt->answers[2], &x2, G_projection()))
	G_fatal_error(_("%s - illegal x value"), coor_opt->answers[2]);
    if (!G_scan_northing(coor_opt->answers[3], &y2, G_projection()))
	G_fatal_error(_("%s - illegal y value"), coor_opt->answers[3]);


    if (D_open_driver() != 0)
      	G_fatal_error(_("No graphics device selected. "
			"Use d.mon to select graphics device."));
    
    color = G_standard_color_rgb(BLACK);
    if (G_str_to_color(color_opt->answer, &r, &g, &b)) {
	color.r = r;
	color.g = g;
	color.b = b;
    }

    hcolor = G_standard_color_rgb(RED);
    if (G_str_to_color(hcolor_opt->answer, &r, &g, &b)) {
	hcolor.r = r;
	hcolor.g = g;
	hcolor.b = b;
    }

    bgcolor = G_standard_color_rgb(WHITE);
    if (G_str_to_color(bgcolor_opt->answer, &r, &g, &b)) {
	bgcolor.r = r;
	bgcolor.g = g;
	bgcolor.b = b;
    }

    if (geo_f->answer) {
	geo = 1;
	if (G_projection() != PROJECTION_LL)
	    G_fatal_error(_("The current projection is not longitude-latitude"));
    }
    else
	geo = 0;

    Vect_set_open_level(2);
    Vect_open_old(&Map, map->answer, "");

    D_setup(0);

    Vect_net_build_graph(&Map, type, afield, nfield, afcol->answer,
			 abcol->answer, ncol->answer, geo, 0);

    coor_path(&Map, &hcolor, bold_f->answer, x1, y1, x2, y2);

    D_close_driver();

    Vect_close(&Map);

    exit(EXIT_SUCCESS);
}
