
/****************************************************************************
 *
 * MODULE:       d.his
 * AUTHOR(S):    James Westervelt, CERL (original contributor)
 *               Markus Neteler <neteler itc.it>, 
 *               Bernhard Reiter <bernhard intevation.de>, 
 *               Huidae Cho <grass4u gmail.com>, 
 *               Glynn Clements <glynn gclements.plus.com>, 
 *               Jan-Oliver Wagner <jan intevation.de>,
 *               Hamish Bowman (brightness option)
 * PURPOSE:      produces a raster map layer using hue, intensity, and
 *               saturation values from two or three user-specified raster
 *               map layers
 * COPYRIGHT:    (C) 1999-2007 by the GRASS Development Team
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
#include <grass/glocale.h>
#include "his.h"

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
    int next_row;
    int hue_file;
    int int_file = 0;
    int int_used;
    int sat_file = 0;
    int sat_used;
    struct Cell_head window;
    struct Colors hue_colors;
    struct Colors int_colors;
    struct Colors sat_colors;
    struct Colors gray_colors;
    struct GModule *module;
    struct Option *opt_h, *opt_i, *opt_s, *brighten;
    struct Flag *nulldraw;
    char window_name[64];
    int t, b, l, r;
    double bright_mult;


    G_gisinit(argv[0]);

    module = G_define_module();
    module->keywords = _("display");
    module->description =
	_("Displays the result obtained by combining "
	  "hue, intensity, and saturation (his) values "
	  "from user-specified input raster map layers.");

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

    brighten = G_define_option();
    brighten->key = "brighten";
    brighten->type = TYPE_INTEGER;
    brighten->description = _("Percent to brighten intensity channel");
    brighten->options = "-99-99";
    brighten->answer = "0";

    nulldraw = G_define_flag();
    nulldraw->key = 'n';
    nulldraw->description = _("Respect NULL values while drawing");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);


    /* it's not truly the percentage to brighten,
       but saying that makes the option easy to use */
    bright_mult = 1 + 0.01 * atoi(brighten->answer);

    /* read in current window */
    G_get_window(&window);

    /* Do screen initializing stuff */

    if (R_open_driver() != 0)
	G_fatal_error(_("No graphics device selected"));

    if (D_get_cur_wind(window_name))
	G_fatal_error(_("No current graphics window"));

    if (D_set_cur_wind(window_name))
	G_fatal_error(_("Current graphics window not available"));

    D_set_cell_name("his result");

    /* Prepare the raster cell drawing functions */
    D_get_screen_window(&t, &b, &l, &r);
    D_set_overlay_mode(nulldraw->answer ? 1 : 0);
    D_cell_draw_setup(t, b, l, r);

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
		G_fatal_error("Unable to open raster map <%s>", name_s);

	    sat_r = G_malloc(window.cols);
	    sat_n = G_malloc(window.cols);

	    /* Reading color lookup table */
	    if (G_read_colors(name_s, mapset, &sat_colors) == -1)
		G_fatal_error(_("Color file for <%s> not available"), name_s);
	}
	else
	    G_fatal_error(_("Raster map <%s> not found"), name_s);
    }

    r_array = G_allocate_cell_buf();
    g_array = G_allocate_cell_buf();
    b_array = G_allocate_cell_buf();

    /* Make color table */
    make_gray_scale(&gray_colors);

    /* Now do the work */
    intensity = 255;		/* default is to not change intensity */
    saturation = 255;		/* default is to not change saturation */

    next_row = 0;
    for (atrow = 0; atrow < window.rows;) {
	G_percent(atrow, window.rows, 2);

	if (G_get_raster_row_colors
	    (hue_file, atrow, &hue_colors, hue_r, hue_g, hue_b, hue_n) < 0)
	    G_fatal_error(_("Error reading hue data"));

	if (int_used &&
	    (G_get_raster_row_colors
	     (int_file, atrow, &int_colors, int_r, dummy, dummy, int_n) < 0))
	    G_fatal_error(_("Error reading intensity data"));

	if (sat_used &&
	    (G_get_raster_row_colors
	     (sat_file, atrow, &sat_colors, sat_r, dummy, dummy, sat_n) < 0))
	    G_fatal_error(_("Error reading saturation data"));

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
		intensity = (int)(int_r[atcol] * bright_mult);

	    if (sat_used)
		saturation = sat_r[atcol];

	    HIS_to_RGB(hue_r[atcol], hue_g[atcol], hue_b[atcol],
		       intensity, saturation,
		       &r_array[atcol], &g_array[atcol], &b_array[atcol]);
	}

	if (atrow == next_row)
	    next_row = D_draw_raster_RGB(next_row,
					 r_array, g_array, b_array,
					 &gray_colors, &gray_colors,
					 &gray_colors, CELL_TYPE, CELL_TYPE,
					 CELL_TYPE);

	if (next_row > 0)
	    atrow = next_row;
	else
	    break;
    }
    G_percent(window.rows, window.rows, 5);
    D_cell_draw_end();

    /* Close down connection to display driver */
    D_add_to_list(G_recreate_command());
    R_close_driver();

    /* Close the raster maps */
    G_close_cell(hue_file);
    if (int_used)
	G_close_cell(int_file);
    if (sat_used)
	G_close_cell(sat_file);

    exit(EXIT_SUCCESS);
}
