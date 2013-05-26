
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
#include <grass/raster.h>
#include <grass/display.h>
#include <grass/glocale.h>
#include "his.h"

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
    double bright_mult;


    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("display"));
    G_add_keyword(_("graphics"));
    G_add_keyword(_("color transformation"));
    G_add_keyword("RGB");
    G_add_keyword("HIS");
    G_add_keyword("IHS");
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

    if (D_open_driver() != 0)
	G_fatal_error(_("No graphics device selected. "
			"Use d.mon to select graphics device."));
    
    /* Prepare the raster cell drawing functions */
    D_setup(0);
    D_set_overlay_mode(nulldraw->answer ? 1 : 0);

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

    r_array = Rast_allocate_c_buf();
    g_array = Rast_allocate_c_buf();
    b_array = Rast_allocate_c_buf();

    /* Make color table */
    make_gray_scale(&gray_colors);

    /* Now do the work */
    intensity = 255;		/* default is to not change intensity */
    saturation = 255;		/* default is to not change saturation */

    D_cell_draw_begin();

    next_row = 0;
    for (atrow = 0; atrow < window.rows;) {
	G_percent(atrow, window.rows, 2);

	Rast_get_row_colors
	    (hue_file, atrow, &hue_colors, hue_r, hue_g, hue_b, hue_n);

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

    D_save_command(G_recreate_command());
    
    /* Close down connection to display driver */
    D_close_driver();

    /* Close the raster maps */
    Rast_close(hue_file);
    if (int_used)
	Rast_close(int_file);
    if (sat_used)
	Rast_close(sat_file);

    exit(EXIT_SUCCESS);
}
