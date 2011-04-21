/*
 ****************************************************************************
 *
 * MODULE:       d.rast.num
 * AUTHOR(S):    Raghavan Srinivasan, Agricultural Engineering, Purdue University
 * PURPOSE:      Print numbers of category for raster cells
 * COPYRIGHT:    (C) 2000 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

/* updated by Andreas Lange, andreas.lange@rhein-main.de 
 * for text color support.
 * updated 2004 my MN for FP support
 */

/*
 *   Raghavan Srinivasan, Agricultural Engineering, Purdue University
 *   srin@ecn.purdue.edu  March 1991
 *
 *   d.rast.num
 *
 *   Usage:  d.rast.num
 * 
 *   This program used Dgrid's sources as a beginning. Purpose of Dnumber
 *   is to read the cell layer displayed on the graphics monitor and number
 *   them, if the cell value is other than 0 in an acending order.
 *   d.rast.num draws a number on the graphic display
 *   of each cell, so the cell number could be identified when using hydrologic
 *   models such AGNPS which uses the cell number for all its correspondance.
 *   
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/display.h>
#include <grass/colors.h>
#include <grass/glocale.h>

int draw_number(int, int, double, int, RASTER_MAP_TYPE);

static double D_ew, D_ns;

int main(int argc, char **argv)
{
    DCELL *cell;
    char *map_name;
    int fixed_color, grid_color;
    int R, G, B;
    int layer_fd;
    int nrows, ncols, row, col;
    int digits;
    struct Cell_head window;
    struct Colors colors;
    struct GModule *module;
    struct Option *opt1, *opt2, *opt3, *prec;
    struct Flag *text_color, *align;
    RASTER_MAP_TYPE map_type, inmap_type;
    double t, b, l, r;

    /* Initialize the GIS calls */
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("display"));
    G_add_keyword(_("raster"));
    module->description =
	_("Overlays cell category values on a raster map layer "
	  "displayed to the graphics monitor.");

    opt1 = G_define_standard_option(G_OPT_R_MAP);

    opt2 = G_define_option();
    opt2->key = "grid_color";
    opt2->type = TYPE_STRING;
    opt2->required = NO;
    opt2->answer = "gray";
    opt2->gisprompt = "old_color,color,color_none";
    opt2->key_desc = "color";
    opt2->description = _("Color for drawing grid, or \"none\"");

    opt3 = G_define_option();
    opt3->key = "text_color";
    opt3->type = TYPE_STRING;
    opt3->required = NO;
    opt3->answer = DEFAULT_FG_COLOR;
    opt3->gisprompt = "old_color,color,color";
    opt3->key_desc = "color";
    opt3->description = _("Color for drawing text");

    prec = G_define_option();
    prec->key = "dp";
    prec->type = TYPE_INTEGER;
    prec->required = NO;
    prec->answer = "1";
    prec->options = "0,1,2,3,4,5,6,7,8,9";
    prec->description =
	_("Number of significant digits (floating point only)");

    align = G_define_flag();
    align->key = 'a';
    align->description = _("Align grids with raster cells");

    text_color = G_define_flag();
    text_color->key = 'f';
    text_color->description = _("Get text color from cell color value");

    /* Check command line */
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    map_name = opt1->answer;

    if (strcmp("none", opt2->answer) == 0)
	grid_color = -1;
    else
	grid_color = D_translate_color(opt2->answer);

    if (text_color->answer)
	fixed_color = 0;
    else
	fixed_color = 1;

    /* Read in the map window associated with window */

    G_get_window(&window);

    if (align->answer) {
	struct Cell_head wind;

	Rast_get_cellhd(map_name, "", &wind);

	/* expand window extent by one wind resolution */
	wind.west += wind.ew_res * ((int)((window.west - wind.west) / wind.ew_res) - (window.west < wind.west));
	wind.east += wind.ew_res * ((int)((window.east - wind.east) / wind.ew_res) + (window.east > wind.east));
	wind.south += wind.ns_res * ((int)((window.south - wind.south) / wind.ns_res) - (window.south < wind.south));
	wind.north += wind.ns_res * ((int)((window.north - wind.north) / wind.ns_res) + (window.north > wind.north));

	wind.rows = (wind.north - wind.south) / wind.ns_res;
	wind.cols = (wind.east - wind.west) / wind.ew_res;

	Rast_set_window(&wind);

	nrows = wind.rows;
	ncols = wind.cols;

	t = (wind.north - window.north) * nrows / (wind.north - wind.south);
	b = t + (window.north - window.south) * nrows / (wind.north - wind.south);
	l = (window.west - wind.west) * ncols / (wind.east - wind.west);
	r = l + (window.east - window.west) * ncols / (wind.east - wind.west);
    } else {
        nrows = window.rows;
        ncols = window.cols;

	t = 0;
	b = nrows;
	l = 0;
	r = ncols;
    }

    layer_fd = Rast_open_old(map_name, "");

    /* determine the inputmap type (CELL/FCELL/DCELL) */
    inmap_type = Rast_get_map_type(layer_fd);
    map_type = DCELL_TYPE;

    /* number of rows and cols in window */

    if ((nrows > 75) || (ncols > 75)) {
	G_warning("!!!");
	G_message(_("Current window size:"));
	G_message(_("rows:    %d"), nrows);
	G_message(_("columns: %d"), ncols);

	G_message(_("\nYour current window setting may be too large."
		    " Cells displayed on your graphics window may be too"
		    " small for cell category number to be visible."));
	G_message(" ");
    }
    if ((nrows > 200) || (ncols > 200)) {
	G_fatal_error(_("Aborting."));
    }

    /* Setup driver and check important information */

    if (D_open_driver() != 0)
	G_fatal_error(_("No graphics device selected"));

    D_setup2(0, 0, t, b, l, r);

    D_ns = fabs(D_get_u_to_d_yconv());
    D_ew = fabs(D_get_u_to_d_xconv());

    /*set the number of significant digits */
    sscanf(prec->answer, "%i", &digits);

    if (grid_color > 0) {	/* ie not "none" */
	/* Set grid color */
	D_use_color(grid_color);

	/* Draw vertical grids */
	for (col = 0; col <= ncols; col++)
	    D_line_abs(col, 0, col, nrows);

	/* Draw horizontal grids */
	for (row = 0; row <= nrows; row++)
	    D_line_abs(0, row, ncols, row);
    }

    /* allocate the cell array */
    cell = Rast_allocate_buf(map_type);

    /* read the color table in the color structures of the displayed map */
    if (Rast_read_colors(map_name, "", &colors) == -1)
	G_fatal_error(_("Color file for <%s> not available"), map_name);

    /* fixed text color */
    if (fixed_color == 1)
	D_use_color(D_translate_color(opt3->answer));

    /* loop through cells, find value, and draw text for value */
    for (row = 0; row < nrows; row++) {
	Rast_get_row(layer_fd, cell, row, map_type);

	for (col = 0; col < ncols; col++) {

	    if (fixed_color == 0) {
		Rast_get_color(&cell[col], &R, &G, &B, &colors, map_type);
		D_RGB_color(R, G, B);
	    }

	    draw_number(row, col, cell[col], digits, inmap_type);
	}
    }

    Rast_close(layer_fd);

    D_close_driver();

    exit(EXIT_SUCCESS);
}

/* --- end of main --- */


int draw_number(int row, int col, double number, int prec, RASTER_MAP_TYPE map_type)
{
    int len;
    double text_size, rite;
    double tt, tb, tl, tr;
    char no[32];
    double dots_per_line, factor = 0.8;
    DCELL dcell = number;
    CELL cell = (int)number;
    double dx;

    /* maybe ugly, but works */
    if (map_type == CELL_TYPE) {
	if (!Rast_is_c_null_value(&cell))
	    sprintf(no, "%d", (int)number);
	else
	    sprintf(no, "Null");
    }
    else {
	if (!Rast_is_d_null_value(&dcell))
	    sprintf(no, "%.*f", prec, number);
	else
	    sprintf(no, "Null");
    }
    len = strlen(no);

    dots_per_line = factor * D_ns;
    text_size = factor * dots_per_line;
    rite = text_size * len;

    while (rite > D_ew) {
	factor = factor - 0.01;
	text_size = factor * dots_per_line;
	rite = text_size * len;
    }

    D_text_size(text_size, text_size);

    D_pos_abs(col, row + 0.7);
    D_get_text_box(no, &tt, &tb, &tl, &tr);

    dx = (tr + tl) / 2 - (col + 0.5);
    D_pos_abs(col - dx, row + 0.7);
    D_text(no);

    return 0;
}
