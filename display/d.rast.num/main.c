/*
 ****************************************************************************
 *
 * MODULE:       d.rast.num
 * AUTHOR(S):    Raghavan Srinivasan, Agricultural Engineering, Purdue University
 * PURPOSE:      Print numbers of category for raster cells
 * COPYRIGHT:    (C) 2000, 2012 by the GRASS Development Team
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
    struct _opt {
	struct Option *map, *grid_color, *text_color, *prec,
	    *font, *path, *charset;
    } opt;
    struct _flg {
	struct Flag *text_color, *align;
    } flg;
    RASTER_MAP_TYPE map_type, inmap_type;
    double t, b, l, r;
    char *tmpstr1, *tmpstr2;

    /* Initialize the GIS calls */
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("display"));
    G_add_keyword(_("map annotations"));
    G_add_keyword(_("raster"));
    module->description =
	_("Overlays cell category values on a raster map "
	  "displayed in the active graphics frame.");

    opt.map = G_define_standard_option(G_OPT_R_MAP);

    opt.text_color = G_define_standard_option(G_OPT_C);
    opt.text_color->key = "text_color";
    opt.text_color->label = _("Text color");
    opt.text_color->guisection = _("Colors");

    opt.grid_color = G_define_standard_option(G_OPT_CN);
    opt.grid_color->key = "grid_color";
    opt.grid_color->answer = "gray";
    opt.grid_color->label = _("Grid color");
    opt.grid_color->guisection = _("Colors");

    opt.prec = G_define_option();
    opt.prec->key = "precision";
    opt.prec->type = TYPE_INTEGER;
    opt.prec->required = NO;
    opt.prec->answer = "1";
    opt.prec->options = "0,1,2,3,4,5,6,7,8,9";
    opt.prec->description =
	_("Number of significant digits (floating point only)");

    flg.align = G_define_flag();
    flg.align->key = 'a';
    flg.align->description = _("Align grids with raster cells");

    flg.text_color = G_define_flag();
    flg.text_color->key = 'f';
    flg.text_color->description = _("Get text color from cell color value");
    flg.text_color->guisection = _("Colors");

    opt.font = G_define_option();
    opt.font->key = "font";
    opt.font->type = TYPE_STRING;
    opt.font->required = NO;
    opt.font->description = _("Font name");
    opt.font->guisection = _("Font settings");

    opt.path = G_define_standard_option(G_OPT_F_INPUT);
    opt.path->key = "path";
    opt.path->required = NO;
    opt.path->description = _("Path to font file");
    opt.path->gisprompt = "old_file,font,file";
    opt.path->guisection = _("Font settings");

    opt.charset = G_define_option();
    opt.charset->key = "charset";
    opt.charset->type = TYPE_STRING;
    opt.charset->required = NO;
    opt.charset->description =
	_("Text encoding (only applicable to TrueType fonts)");
    opt.charset->guisection = _("Font settings");

    /* Check command line */
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    map_name = opt.map->answer;

    if (strcmp("none", opt.grid_color->answer) == 0)
	grid_color = -1;
    else
	grid_color = D_translate_color(opt.grid_color->answer);

    if (flg.text_color->answer)
	fixed_color = 0;
    else
	fixed_color = 1;

    /* Read in the map window associated with window */

    G_get_window(&window);

    if (flg.align->answer) {
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
        G_asprintf(&tmpstr1, n_("%d row", "%d rows", nrows), nrows);
        G_asprintf(&tmpstr2, n_("%d col", "%d cols", ncols), ncols);
        /* GTC %s will be replaced by strings "X rows" and "Y cols" */
        G_warning(_("Current region size: %s X %s\n"
		    "Your current region setting may be too large. "
		    "Cells displayed on your graphics window may be too "
		    "small for cell category number to be visible."),
		    tmpstr1, tmpstr2);
        G_free(tmpstr1);
        G_free(tmpstr2); 
          
    }
    if ((nrows > 200) || (ncols > 200)) {
	G_fatal_error(_("Aborting (region larger then 200 rows X 200 cols is not allowed)"));
    }

    /* Setup driver and check important information */

    D_open_driver();
    
    if (opt.font->answer)
	D_font(opt.font->answer);
    else if (opt.path->answer)
	D_font(opt.path->answer);

    if (opt.charset->answer)
	D_encoding(opt.charset->answer);
    
    D_setup2(0, 0, t, b, l, r);

    D_ns = fabs(D_get_u_to_d_yconv());
    D_ew = fabs(D_get_u_to_d_xconv());

    /*set the number of significant digits */
    sscanf(opt.prec->answer, "%i", &digits);

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
	D_use_color(D_translate_color(opt.text_color->answer));

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

    D_save_command(G_recreate_command());
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
