/*
 ****************************************************************************
 *
 * MODULE:       r.his
 * AUTHOR(S):    Glynn Clements - glynn.clements@virgin.net
 * PURPOSE:      Create a color image by composing color, brightness
 *               and haze maps
 * COPYRIGHT:    (C) 2001 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/display.h>
#include "his.h"
#include <grass/glocale.h>

int main(int argc, char **argv)
{
    unsigned char *hue_n, *hue_r, *hue_g, *hue_b;
    unsigned char *int_n, *int_r;
    unsigned char *sat_n, *sat_r;
    unsigned char *dummy;
    CELL *r_array, *g_array, *b_array;
    char *name_h, *name_i, *name_s;
    int intensity;
    int saturation;
    int atrow, atcol;
    int hue_file;
    int int_file = 0;
    int int_used;
    int sat_file = 0;
    int sat_used;
    char *name_r, *name_g, *name_b;
    int r_file = 0;
    int r_used;
    int g_file = 0;
    int g_used;
    int b_file = 0;
    int b_used;
    struct Cell_head window;
    struct Colors hue_colors;
    struct Colors int_colors;
    struct Colors sat_colors;
    struct Colors gray_colors;
    struct History history;
    struct GModule *module;
    struct Option *opt_h, *opt_i, *opt_s;
    struct Option *opt_r, *opt_g, *opt_b;
    struct Flag *nulldraw;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("color transformation"));
    G_add_keyword("RGB");
    G_add_keyword("HIS");
    G_add_keyword("IHS");
    module->description =
	_("Generates red, green and blue raster map layers "
	  "combining hue, intensity and saturation (HIS) "
	  "values from user-specified input raster map layers.");

    opt_h = G_define_option();
    opt_h->key = "h_map";
    opt_h->type = TYPE_STRING;
    opt_h->required = YES;
    opt_h->gisprompt = "old,cell,raster";
    opt_h->description = _("Name of layer to be used for HUE");

    opt_i = G_define_option();
    opt_i->key = "i_map";
    opt_i->type = TYPE_STRING;
    opt_i->required = NO;
    opt_i->gisprompt = "old,cell,raster";
    opt_i->description = _("Name of layer to be used for INTENSITY");

    opt_s = G_define_option();
    opt_s->key = "s_map";
    opt_s->type = TYPE_STRING;
    opt_s->required = NO;
    opt_s->gisprompt = "old,cell,raster";
    opt_s->description = _("Name of layer to be used for SATURATION");

    opt_r = G_define_option();
    opt_r->key = "r_map";
    opt_r->type = TYPE_STRING;
    opt_r->required = YES;
    opt_r->gisprompt = "new,cell,raster";
    opt_r->description = _("Name of output layer to be used for RED");

    opt_g = G_define_option();
    opt_g->key = "g_map";
    opt_g->type = TYPE_STRING;
    opt_g->required = YES;
    opt_g->gisprompt = "new,cell,raster";
    opt_g->description = _("Name of output layer to be used for GREEN");

    opt_b = G_define_option();
    opt_b->key = "b_map";
    opt_b->type = TYPE_STRING;
    opt_b->required = YES;
    opt_b->gisprompt = "new,cell,raster";
    opt_b->description = _("Name of output layer to be used for BLUE");

    nulldraw = G_define_flag();
    nulldraw->key = 'n';
    nulldraw->description = _("Respect NULL values while drawing");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);


    /* read in current window */
    G_get_window(&window);

    /* Get name of layer to be used for hue */
    name_h = opt_h->answer;

    /* Make sure map is available */
    hue_file = Rast_open_old(name_h, "");

    hue_r = G_malloc(window.cols);
    hue_g = G_malloc(window.cols);
    hue_b = G_malloc(window.cols);
    hue_n = G_malloc(window.cols);

    dummy = G_malloc(window.cols);

    /* Reading color lookup table */
    if (Rast_read_colors(name_h, "", &hue_colors) == -1)
	G_fatal_error(_("Color file for <%s> not available"), name_h);

    int_used = 0;

    if (opt_i->answer != NULL) {
	/* Get name of layer to be used for intensity */
	name_i = opt_i->answer;
	int_used = 1;
	/* Make sure map is available */
	int_file = Rast_open_old(name_i, "");

	int_r = G_malloc(window.cols);
	int_n = G_malloc(window.cols);

	/* Reading color lookup table */
	if (Rast_read_colors(name_i, "", &int_colors) == -1)
	    G_fatal_error(_("Color file for <%s> not available"), name_i);
    }

    sat_used = 0;

    if (opt_s->answer != NULL) {
	/* Get name of layer to be used for saturation */
	name_s = opt_s->answer;
	    sat_used = 1;

	    /* Make sure map is available */
	    sat_file = Rast_open_old(name_s, "");

	    sat_r = G_malloc(window.cols);
	    sat_n = G_malloc(window.cols);

	    /* Reading color lookup table */
	    if (Rast_read_colors(name_s, "", &sat_colors) == -1)
		G_fatal_error(_("Color file for <%s> not available"), name_s);
    }

    r_used = 0;

    if (opt_r->answer != NULL) {
	name_r = opt_r->answer;
	r_file = Rast_open_c_new(name_r);
	r_used = 1;
    }

    g_used = 0;

    if (opt_g->answer != NULL) {
	name_g = opt_g->answer;
	g_file = Rast_open_c_new(name_g);
	g_used = 1;
    }

    b_used = 0;

    if (opt_b->answer != NULL) {
	name_b = opt_b->answer;
	b_file = Rast_open_c_new(name_b);
	b_used = 1;
    }

    r_array = Rast_allocate_c_buf();
    g_array = Rast_allocate_c_buf();
    b_array = Rast_allocate_c_buf();

    /* Make color table */
    make_gray_scale(&gray_colors);

    /* Now do the work */
    intensity = 255;		/* default is to not change intensity */
    saturation = 255;		/* default is to not change saturation */


    for (atrow = 0; atrow < window.rows; atrow++) {
	G_percent(atrow, window.rows, 2);

	Rast_get_row_colors(hue_file, atrow, &hue_colors, hue_r, hue_g, hue_b, hue_n);
	if (int_used)
	    Rast_get_row_colors(int_file, atrow, &int_colors, int_r, dummy, dummy, int_n);
	if (sat_used)
	    Rast_get_row_colors(sat_file, atrow, &sat_colors, sat_r, dummy, dummy, sat_n);

	for (atcol = 0; atcol < window.cols; atcol++) {
	    if (nulldraw->answer) {
		if (hue_n[atcol]
		    || (int_used && int_n[atcol])
		    || (sat_used && sat_n[atcol])) {
		    Rast_set_c_null_value(&r_array[atcol], 1);
		    Rast_set_c_null_value(&g_array[atcol], 1);
		    Rast_set_c_null_value(&b_array[atcol], 1);
		    continue;
		}
	    }

	    if (int_used)
		intensity = int_r[atcol];

	    if (sat_used)
		saturation = sat_r[atcol];

	    HIS_to_RGB(hue_r[atcol], hue_g[atcol], hue_b[atcol],
		       intensity, saturation,
		       &r_array[atcol], &g_array[atcol], &b_array[atcol]);
	}

	if (r_used)
	    Rast_put_row(r_file, r_array, CELL_TYPE);

	if (g_used)
	    Rast_put_row(g_file, g_array, CELL_TYPE);

	if (b_used)
	    Rast_put_row(b_file, b_array, CELL_TYPE);
    }
    G_percent(window.rows, window.rows, 5);

    /* Close the cell files */
    Rast_close(hue_file);
    if (int_used)
	Rast_close(int_file);
    if (sat_used)
	Rast_close(sat_file);

    if (r_used) {
	Rast_close(r_file);
	Rast_write_colors(name_r, G_mapset(), &gray_colors);
	Rast_short_history(name_r, "raster", &history);
	Rast_command_history(&history);
	Rast_write_history(name_r, &history);
	Rast_put_cell_title(name_r, "Red extracted from HIS");
    }
    if (g_used) {
	Rast_close(g_file);
	Rast_write_colors(name_g, G_mapset(), &gray_colors);
	Rast_short_history(name_g, "raster", &history);
	Rast_command_history(&history);
	Rast_write_history(name_g, &history);
	Rast_put_cell_title(name_g, "Green extracted from HIS");
    }
    if (b_used) {
	Rast_close(b_file);
	Rast_write_colors(name_b, G_mapset(), &gray_colors);
	Rast_short_history(name_b, "raster", &history);
	Rast_command_history(&history);
	Rast_write_history(name_b, &history);
	Rast_put_cell_title(name_b, "Blue extracted from HIS");
    }

    return EXIT_SUCCESS;
}
