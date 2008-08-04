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
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/display.h>
#include <grass/colors.h>
#include <grass/glocale.h>

#define MAIN
int draw_number(double, int, RASTER_MAP_TYPE);

int D_x, D_y;
double D_ew, D_ns;

int main(int argc, char **argv)
{
    DCELL *cell;
    char *mapset;
    char full_name[128];
    char window_name[64];
    double D_north, D_east;
    double D_south, D_west;
    double U_east, U_north;
    double U_start;
    double U_to_D_xconv, U_to_D_yconv;
    double U_west, U_south;
    double U_x, U_y;
    double ew_res, ns_res;
    extern double D_ew, D_ns;
    extern int D_x, D_y;
    int fixed_color, grid_color;
    int R, G, B;
    int layer_fd;
    int nrows, ncols, row, col;
    int t, b, l, r;
    int digits;
    struct Cell_head window;
    struct Colors colors;
    struct GModule *module;
    struct Option *opt1, *opt2, *opt3, *prec;
    struct Flag *text_color;
    RASTER_MAP_TYPE map_type, inmap_type;

    /* Initialize the GIS calls */
    G_gisinit(argv[0]);

    module = G_define_module();
    module->keywords = _("display");
    module->description =
	_("Overlays cell category values on a raster map layer "
	  "displayed to the graphics monitor.");

    opt1 = G_define_standard_option(G_OPT_R_MAP);
    opt1->required = NO;

    opt2 = G_define_option();
    opt2->key = "grid_color";
    opt2->type = TYPE_STRING;
    opt2->required = NO;
    opt2->answer = "gray";
    opt2->options = D_COLOR_LIST ",none";
    opt2->key_desc = "color";
    opt2->description = _("Color for drawing grid, or \"none\"");

    opt3 = G_define_option();
    opt3->key = "text_color";
    opt3->type = TYPE_STRING;
    opt3->required = NO;
    opt3->answer = DEFAULT_FG_COLOR;
    opt3->options = D_COLOR_LIST;
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

    text_color = G_define_flag();
    text_color->key = 'f';
    text_color->description = _("Get text color from cell color value");

    /* Check command line */
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    if (R_open_driver() != 0)
	G_fatal_error(_("No graphics device selected"));

    if (opt1->answer)
	strcpy(full_name, opt1->answer);
    else {
	if (D_get_cell_name(full_name))
	    G_fatal_error(_("No raster map exists in current window"));
    }

    if (strcmp("none", opt2->answer) == 0)
	grid_color = -1;
    else
	grid_color = D_translate_color(opt2->answer);

    if (text_color->answer)
	fixed_color = 0;
    else
	fixed_color = 1;

    mapset = G_find_cell(full_name, "");
    if (mapset == NULL)
	G_fatal_error(_("Raster map <%s> not found"), full_name);

    layer_fd = G_open_cell_old(full_name, mapset);
    if (layer_fd < 0)
	G_fatal_error(_("Unable to open raster map <%s>"), full_name);

    /* determine the inputmap type (CELL/FCELL/DCELL) */
    inmap_type = G_get_raster_map_type(layer_fd);
    map_type = DCELL_TYPE;

    /* Setup driver and check important information */

    if (D_get_cur_wind(window_name))
	G_fatal_error(_("No current window"));

    if (D_set_cur_wind(window_name))
	G_fatal_error(_("Current window not available"));

    /* Read in the map window associated with window */

    G_get_window(&window);

    if (D_check_map_window(&window))
	G_fatal_error(_("Setting map window"));

    if (G_set_window(&window) == -1)
	G_fatal_error(_("Current window not settable"));

    /* Determine conversion factors */

    if (D_get_screen_window(&t, &b, &l, &r))
	G_fatal_error(_("Getting screen window"));
    if (D_do_conversions(&window, t, b, l, r))
	G_fatal_error(_("Error in calculating conversions"));

    /* where are we, both geographically and on the screen? */

    D_south = D_get_d_south();
    D_north = D_get_d_north();
    D_east = D_get_d_east();
    D_west = D_get_d_west();

    U_west = D_get_u_west();
    U_east = D_get_u_east();
    U_south = D_get_u_south();
    U_north = D_get_u_north();

    U_to_D_xconv = D_get_u_to_d_xconv();
    U_to_D_yconv = D_get_u_to_d_yconv();

    /* number of rows and cols in window */

    nrows = window.rows;
    ncols = window.cols;

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

    /* resolutions */
    ew_res = window.ew_res;
    ns_res = window.ns_res;

    /* how many screen units of distance for each cell */
    D_ew = (D_east - D_west) / ncols;
    D_ns = (D_south - D_north) / nrows;

    /*set the number of significant digits */
    sscanf(prec->answer, "%i", &digits);

	/*-- DEBUG ----------------------------------------
	fprintf (stdout,"ew_res:  %.2f\n", window.ew_res);
	fprintf (stdout,"ns_res:  %.2f\n", window.ns_res);
	fprintf (stdout,"D_ew:  %f D_ns:  %f \n", D_ew, D_ns); 
	fprintf (stdout,"nrows:    %d\n", nrows);
	fprintf (stdout,"ncols:    %d\n", ncols);
	fprintf (stdout,"t:  %d\n", t);
	fprintf (stdout,"b:  %d\n", b);
	fprintf (stdout,"l:  %d\n", l);
	fprintf (stdout,"r:  %d\n", r);
	fprintf (stdout,"U_west:    %f\n", U_west);
	fprintf (stdout,"U_east:    %f\n", U_east);
	fprintf (stdout,"U_south:   %f\n", U_south);
	fprintf (stdout,"U_north:   %f\n", U_north);
	fprintf (stdout,"D_west:    %f\n", D_west);
	fprintf (stdout,"D_east:    %f\n", D_east);
	fprintf (stdout,"D_south:   %f\n", D_south);
	fprintf (stdout,"D_north:   %f\n", D_north);
	fprintf (stdout,"U_to_D_xconv:      %f\n", U_to_D_xconv);
	fprintf (stdout,"U_to_D_yconv:      %f\n", U_to_D_yconv);
	--------------------------------------------------------*/

    if (grid_color > 0) {	/* ie not "none" */
	/* Set grid color */
	R_standard_color(grid_color);

	/* Draw vertical grids */
	U_start = U_east;
	for (U_x = U_start; U_x >= U_west; U_x -= ew_res) {
	    D_x = (int)((U_x - U_west) * U_to_D_xconv + D_west);
	    R_move_abs(D_x, (int)D_south);
	    R_cont_abs(D_x, (int)D_north);
	}

	/* Draw horizontal grids */
	U_start = U_north;
	for (U_y = U_start; U_y >= U_south; U_y -= ns_res) {
	    D_y = (int)((U_south - U_y) * U_to_D_yconv + D_south);
	    R_move_abs((int)D_west, D_y);
	    R_cont_abs((int)D_east, D_y);
	}
    }

    /* allocate the cell array */
    cell = G_allocate_raster_buf(map_type);

    /* read the color table in the color structures of the displayed map */
    if (G_read_colors(full_name, mapset, &colors) == -1)
	G_fatal_error(_("Color file for <%s> not available"), full_name);

    /* fixed text color */
    if (fixed_color == 1)
	R_standard_color(D_translate_color(opt3->answer));

    /* loop through cells, find value, and draw text for value */
    for (row = 0; row < nrows; row++) {
	G_get_raster_row(layer_fd, cell, row, map_type);

	/* determine screen y coordinate of top of current cell */

	D_y = (int)(row * D_ns + D_north);

	for (col = 0; col < ncols; col++) {
	    /* determine screen x coordinate of west side of current cell */
	    D_x = (int)(col * D_ew + D_west);

	    if (fixed_color == 0) {
		G_get_raster_color(&cell[col], &R, &G, &B, &colors, map_type);
		R_RGB_color(R, G, B);
	    }

	    draw_number(cell[col], digits, inmap_type);
	}
    }

    G_close_cell(layer_fd);
    D_add_to_list(G_recreate_command());
    R_close_driver();

    exit(EXIT_SUCCESS);
}

/* --- end of main --- */


int draw_number(double number, int prec, RASTER_MAP_TYPE map_type)
{
    extern double D_ew, D_ns;
    extern int D_x, D_y;
    int len, text_size, rite;
    int tt, tb, tl, tr;
    char *itoa(), no[10];
    double dots_per_line, factor = 0.8;
    DCELL dcell = number;
    CELL cell = (int)number;

    R_set_window(D_y, D_y + (int)(D_ns * 0.9), D_x, D_x + (int)(D_ew * 0.9));

    /* maybe ugly, but works */
    if (map_type == CELL_TYPE) {
	if (!G_is_c_null_value(&cell))
	    sprintf(no, "%d", (int)number);
	else
	    sprintf(no, "Null");
    }
    else {
	if (!G_is_d_null_value(&dcell))
	    sprintf(no, "%.*f", prec, number);
	else
	    sprintf(no, "Null");
    }
    len = strlen(no);

    dots_per_line = factor * D_ns;
    text_size = (int)(factor * (float)dots_per_line);
    rite = text_size * len;

    while (rite > D_ew) {
	factor = factor - 0.01;
	text_size = (int)(factor * (float)dots_per_line);
	rite = text_size * len;
    }

    R_text_size(text_size, text_size);
    R_get_text_box(no, &tt, &tb, &tl, &tr);
    /*
       R_get_text_box(num,&tt,&tb,&tl,&tr);
       R_move_abs(D_x+(int)(D_ew*0.1),D_y+(int)(D_ns*0.5)) ;
       R_move_abs(D_x,D_y+(int)(dots_per_line - 1)) ;
     */
    R_move_abs((int)(D_x + ((float)D_ew / 2) - ((float)(tr - tl) / 2)),
	       (int)(D_y + D_ns * 0.7));
    R_text(no);

    return 0;
}
