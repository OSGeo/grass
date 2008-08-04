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
#include <grass/display.h>
#include <grass/raster.h>
#include "his.h"
#include <grass/glocale.h>

int main(int argc, char **argv)
{
    unsigned char *hue_n, *hue_r, *hue_g, *hue_b;
    unsigned char *int_n, *int_r;
    unsigned char *sat_n, *sat_r;
    unsigned char *dummy;
    CELL *r_array, *g_array, *b_array;
    char *mapset;
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
    module->keywords = _("raster");
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

    mapset = G_find_cell2(name_h, "");
    if (mapset == NULL)
	G_fatal_error(_("Raster map <%s> not found"), opt_h->answer);

    /* Make sure map is available */
    if ((hue_file = G_open_cell_old(name_h, mapset)) == -1)
	G_fatal_error(_("Unable to open raster map <%s>"), name_h);

    hue_r = G_malloc(window.cols);
    hue_g = G_malloc(window.cols);
    hue_b = G_malloc(window.cols);
    hue_n = G_malloc(window.cols);

    dummy = G_malloc(window.cols);

    /* Reading color lookup table */
    if (G_read_colors(name_h, mapset, &hue_colors) == -1)
	G_fatal_error(_("Color file for <%s> not available"), name_h);

    int_used = 0;

    if (opt_i->answer != NULL) {
	/* Get name of layer to be used for intensity */
	name_i = opt_i->answer;
	mapset = G_find_cell2(name_i, "");
	if (mapset != NULL) {
	    int_used = 1;
	    /* Make sure map is available */
	    if ((int_file = G_open_cell_old(name_i, mapset)) == -1)
		G_fatal_error(_("Unable to open raster map <%s>"), name_i);

	    int_r = G_malloc(window.cols);
	    int_n = G_malloc(window.cols);

	    /* Reading color lookup table */
	    if (G_read_colors(name_i, mapset, &int_colors) == -1)
		G_fatal_error(_("Color file for <%s> not available"), name_i);
	}
	else
	    G_fatal_error(_("Raster map <%s> not found"), name_i);

    }

    sat_used = 0;

    if (opt_s->answer != NULL) {
	/* Get name of layer to be used for saturation */
	name_s = opt_s->answer;
	mapset = G_find_cell2(name_s, "");
	if (mapset != NULL) {
	    sat_used = 1;

	    /* Make sure map is available */
	    if ((sat_file = G_open_cell_old(name_s, mapset)) == -1)
		G_fatal_error(_("Unable to open raster map <%s>"), name_s);

	    sat_r = G_malloc(window.cols);
	    sat_n = G_malloc(window.cols);

	    /* Reading color lookup table */
	    if (G_read_colors(name_s, mapset, &sat_colors) == -1)
		G_fatal_error(_("Color file for <%s> not available"), name_s);
	}
	else
	    G_fatal_error(_("Raster map <%s> not found"), name_s);

    }

    r_used = 0;

    if (opt_r->answer != NULL) {
	name_r = opt_r->answer;

	if ((r_file = G_open_cell_new(name_r)) < 0)
	    r_used = 0;
	else
	    r_used = 1;
    }

    g_used = 0;

    if (opt_g->answer != NULL) {
	name_g = opt_g->answer;

	if ((g_file = G_open_cell_new(name_g)) < 0)
	    g_used = 0;
	else
	    g_used = 1;
    }

    b_used = 0;

    if (opt_b->answer != NULL) {
	name_b = opt_b->answer;

	if ((b_file = G_open_cell_new(name_b)) < 0)
	    b_used = 0;
	else
	    b_used = 1;
    }

    r_array = G_allocate_cell_buf();
    g_array = G_allocate_cell_buf();
    b_array = G_allocate_cell_buf();

    /* Make color table */
    make_gray_scale(&gray_colors);

    /* Now do the work */
    intensity = 255;		/* default is to not change intensity */
    saturation = 255;		/* default is to not change saturation */


    for (atrow = 0; atrow < window.rows; atrow++) {
	G_percent(atrow, window.rows, 2);

	if (G_get_raster_row_colors
	    (hue_file, atrow, &hue_colors, hue_r, hue_g, hue_b, hue_n) < 0)
	    G_fatal_error(_("Error reading 'hue' map"));
	if (int_used &&
	    (G_get_raster_row_colors
	     (int_file, atrow, &int_colors, int_r, dummy, dummy, int_n) < 0))
	    G_fatal_error(_("Error reading 'intensity' map"));
	if (sat_used &&
	    (G_get_raster_row_colors
	     (sat_file, atrow, &sat_colors, sat_r, dummy, dummy, sat_n) < 0))
	    G_fatal_error(_("Error reading 'saturation' map"));

	for (atcol = 0; atcol < window.cols; atcol++) {
	    if (nulldraw->answer) {
		if (hue_n[atcol]
		    || (int_used && int_n[atcol])
		    || (sat_used && sat_n[atcol])) {
		    G_set_c_null_value(&r_array[atcol], 1);
		    G_set_c_null_value(&g_array[atcol], 1);
		    G_set_c_null_value(&b_array[atcol], 1);
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
	    if (G_put_raster_row(r_file, r_array, CELL_TYPE) < 0)
		r_used = 0;

	if (g_used)
	    if (G_put_raster_row(g_file, g_array, CELL_TYPE) < 0)
		g_used = 0;

	if (b_used)
	    if (G_put_raster_row(b_file, b_array, CELL_TYPE) < 0)
		b_used = 0;
    }
    G_percent(window.rows, window.rows, 5);

    /* Close the cell files */
    G_close_cell(hue_file);
    if (int_used)
	G_close_cell(int_file);
    if (sat_used)
	G_close_cell(sat_file);

    if (r_used) {
	G_close_cell(r_file);
	G_write_colors(name_r, G_mapset(), &gray_colors);
	G_short_history(name_r, "raster", &history);
	G_command_history(&history);
	G_write_history(name_r, &history);
	G_put_cell_title(name_r, "Red extracted from HIS");
    }
    if (g_used) {
	G_close_cell(g_file);
	G_write_colors(name_g, G_mapset(), &gray_colors);
	G_short_history(name_g, "raster", &history);
	G_command_history(&history);
	G_write_history(name_g, &history);
	G_put_cell_title(name_g, "Green extracted from HIS");
    }
    if (b_used) {
	G_close_cell(b_file);
	G_write_colors(name_b, G_mapset(), &gray_colors);
	G_short_history(name_b, "raster", &history);
	G_command_history(&history);
	G_write_history(name_b, &history);
	G_put_cell_title(name_b, "Blue extracted from HIS");
    }

    return EXIT_SUCCESS;
}
