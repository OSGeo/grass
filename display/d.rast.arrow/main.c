/*
 ****************************************************************************
 *
 * MODULE:       d.rast.arrow
 * AUTHOR(S):    Chris Rewerts, Agricultural Engineering, Purdue University
 * PURPOSE:      Draw arrows on slope/aspect maps. 
 * COPYRIGHT:    (C) 2000, 2010 by the GRASS Development Team
 *
 *              This program is free software under the GNU General Public
 *              License (>=v2). Read the file COPYING that comes with GRASS
 *              for details.
 *
 *****************************************************************************/

/* some minor cleanup done by Andreas Lange, andreas.lange@rhein-main.de
 * Update to handle NULLs and floating point aspect maps: Hamish Bowman, Aug 2004
 * Update for 360 degree arrows and magnitude scaling:  Hamish Bowman, Oct 2005
 */

/*
 *   Chris Rewerts, Agricultural Engineering, Purdue University
 *   rewerts@ecn.purdue.edu  March 1991
 *
 *   d.rast.arrow
 *
 *   Usage:  d.rast.arrow
 * 
 *   This program used Dgrid's sources as a beginning. Purpose of Darrow
 *   is to read an aspect layer produced by slope.aspect or by the 
 *   programs created for the ANSWERS or AGNPS Hydrology Toolbox
 *   endeavors.  d.rast.arrow draws an arrow on the graphic display
 *   of each cell, so that the flow pattern computed as an aspect
 *   layer can be easily seen. Other symbols ("?", "X") may be drawn
 *   as needed.
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/display.h>
#include <grass/colors.h>
#include <grass/glocale.h>

# define RpD ((2 * M_PI) / 360.)	/* radians/degree */
# define D2R(d) (double)(d * RpD)	/* degrees->radians */

static void arrow_mag(double, double);
static void arrow_360(double);
static void arrow_se(void);
static void arrow_ne(void);
static void arrow_nw(void);
static void arrow_sw(void);
static void arrow_e(void);
static void arrow_w(void);
static void arrow_s(void);
static void arrow_n(void);
static void draw_x(void);
static void unknown_(void);

static char *layer_name;
static int map_type, arrow_color, grid_color, x_color, unknown_color;
static int row, col;

int main(int argc, char **argv)
{
    struct Cell_head window;
    RASTER_MAP_TYPE raster_type, mag_raster_type = -1;
    int layer_fd;
    void *raster_row, *ptr;
    int nrows, ncols;
    int aspect_c = -1;
    float aspect_f = -1.0;

    double scale;
    int skip, no_arrow;
    char *mag_map = NULL;
    void *mag_raster_row = NULL, *mag_ptr = NULL;
    double length = -1;
    int mag_fd = -1;
    struct FPRange range;
    double mag_min, mag_max;

    struct GModule *module;
    struct Option *opt1, *opt2, *opt3, *opt4, *opt5,
	*opt6, *opt7, *opt8, *opt9;
    struct Flag *align;

    double t, b, l, r;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("display"));
    G_add_keyword(_("map annotations"));
    G_add_keyword(_("raster"));
    module->description =
	_("Draws arrows representing cell aspect direction "
	  "for a raster map containing aspect data.");

    opt1 = G_define_standard_option(G_OPT_R_MAP);
    opt1->description = _("Name of raster aspect map to be displayed");

    opt2 = G_define_option();
    opt2->key = "type";
    opt2->type = TYPE_STRING;
    opt2->required = NO;
    opt2->answer = "grass";
    opt2->options = "grass,compass,agnps,answers";
    opt2->description = _("Type of existing raster aspect map");

    opt3 = G_define_option();
    opt3->key = "arrow_color";
    opt3->type = TYPE_STRING;
    opt3->required = NO;
    opt3->answer = "green";
    opt3->gisprompt = "old_color,color,color";
    opt3->description = _("Color for drawing arrows");
    opt3->guisection = _("Colors");
    
    opt4 = G_define_option();
    opt4->key = "grid_color";
    opt4->required = NO;
    opt4->answer = "gray";
    opt4->gisprompt = "old,color_none,color";
    opt4->description = _("Color for drawing grid or \"none\"");
    opt4->guisection = _("Colors");

    opt5 = G_define_option();
    opt5->key = "x_color";
    opt5->type = TYPE_STRING;
    opt5->required = NO;
    opt5->answer = DEFAULT_FG_COLOR;
    opt5->gisprompt = "old,color_none,color";
    opt5->description = _("Color for drawing X's (null values)");
    opt5->guisection = _("Colors");

    opt6 = G_define_option();
    opt6->key = "unknown_color";
    opt6->type = TYPE_STRING;
    opt6->required = NO;
    opt6->answer = "red";
    opt6->gisprompt = "old,color_none,color";
    opt6->description = _("Color for showing unknown information");
    opt6->guisection = _("Colors");

    opt9 = G_define_option();
    opt9->key = "skip";
    opt9->type = TYPE_INTEGER;
    opt9->required = NO;
    opt9->answer = "1";
    opt9->description = _("Draw arrow every Nth grid cell");

    opt7 = G_define_option();
    opt7->key = "magnitude_map";
    opt7->type = TYPE_STRING;
    opt7->required = NO;
    opt7->multiple = NO;
    opt7->gisprompt = "old,cell,raster";
    opt7->description =
	_("Raster map containing values used for arrow length");

    opt8 = G_define_option();
    opt8->key = "scale";
    opt8->type = TYPE_DOUBLE;
    opt8->required = NO;
    opt8->answer = "1.0";
    opt8->description = _("Scale factor for arrows (magnitude map)");

    align = G_define_flag();
    align->key = 'a';
    align->description = _("Align grids with raster cells");


    /* Check command line */
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);


    layer_name = opt1->answer;

    arrow_color = D_translate_color(opt3->answer);
    x_color = D_translate_color(opt5->answer);
    unknown_color = D_translate_color(opt6->answer);

    if (strcmp("none", opt4->answer) == 0)
	grid_color = -1;
    else
	grid_color = D_translate_color(opt4->answer);


    if (strcmp("grass", opt2->answer) == 0)
	map_type = 1;
    else if (strcmp("agnps", opt2->answer) == 0)
	map_type = 2;
    else if (strcmp("answers", opt2->answer) == 0)
	map_type = 3;
    else if (strcmp("compass", opt2->answer) == 0)
	map_type = 4;


    scale = atof(opt8->answer);
    if (scale <= 0.0)
	G_fatal_error(_("Illegal value for scale factor"));

    skip = atoi(opt9->answer);
    if (skip <= 0)
	G_fatal_error(_("Illegal value for skip factor"));


    if (opt7->answer) {
	if (map_type != 1 && map_type != 4)
	    G_fatal_error(_("Magnitude is only supported for GRASS and compass aspect maps."));

	mag_map = opt7->answer;
    }
    else if (scale != 1.0)
	G_warning(_("Scale option requires magnitude_map"));


    /* Setup driver and check important information */
    if (D_open_driver() != 0)
      	G_fatal_error(_("No graphics device selected. "
			"Use d.mon to select graphics device."));
    
    D_setup(0);

    /* Read in the map window associated with window */
    G_get_window(&window);

    if (align->answer) {
	struct Cell_head wind;

	Rast_get_cellhd(layer_name, "", &wind);

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

    D_set_src(t, b, l, r);
    D_update_conversions();

    /* figure out arrow scaling if using a magnitude map */
    if (opt7->answer) {
	Rast_init_fp_range(&range);	/* really needed? */
	if (Rast_read_fp_range(mag_map, "", &range) != 1)
	    G_fatal_error(_("Problem reading range file"));
	Rast_get_fp_range_min_max(&range, &mag_min, &mag_max);

	scale *= 1.5 / fabs(mag_max);
	G_debug(3, "scaling=%.2f  rast_max=%.2f", scale, mag_max);
    }

    if (grid_color > 0) {	/* ie not "none" */
	/* Set color */
	D_use_color(grid_color);

	/* Draw vertical grids */
	for (col = 0; col < ncols; col++)
	    D_line_abs(col, 0, col, nrows);

	/* Draw horizontal grids */
	for (row = 0; row < nrows; row++)
	    D_line_abs(0, row, ncols, row);
    }

    /* open the raster map */
    layer_fd = Rast_open_old(layer_name, "");

    raster_type = Rast_get_map_type(layer_fd);

    /* allocate the cell array */
    raster_row = Rast_allocate_buf(raster_type);


    if (opt7->answer) {
	/* open the magnitude raster map */
	mag_fd = Rast_open_old(mag_map, "");

	mag_raster_type = Rast_get_map_type(mag_fd);

	/* allocate the cell array */
	mag_raster_row = Rast_allocate_buf(mag_raster_type);
    }


    /* loop through cells, find value, determine direction (n,s,e,w,ne,se,sw,nw),
       and call appropriate function to draw an arrow on the cell */

    for (row = 0; row < nrows; row++) {
	Rast_get_row(layer_fd, raster_row, row, raster_type);
	ptr = raster_row;

	if (opt7->answer) {
	    Rast_get_row(mag_fd, mag_raster_row, row, mag_raster_type);
	    mag_ptr = mag_raster_row;
	}

	for (col = 0; col < ncols; col++) {

	    if (row % skip != 0)
		no_arrow = TRUE;
	    else
		no_arrow = FALSE;

	    if (col % skip != 0)
		no_arrow = TRUE;

	    /* find aspect direction based on cell value */
	    if (raster_type == CELL_TYPE)
		aspect_f = *((CELL *) ptr);
	    else if (raster_type == FCELL_TYPE)
		aspect_f = *((FCELL *) ptr);
	    else if (raster_type == DCELL_TYPE)
		aspect_f = *((DCELL *) ptr);


	    if (opt7->answer) {

		if (mag_raster_type == CELL_TYPE)
		    length = *((CELL *) mag_ptr);
		else if (mag_raster_type == FCELL_TYPE)
		    length = *((FCELL *) mag_ptr);
		else if (mag_raster_type == DCELL_TYPE)
		    length = *((DCELL *) mag_ptr);

		length *= scale;

		if (Rast_is_null_value(mag_ptr, mag_raster_type)) {
		    G_debug(5, "Invalid arrow length [NULL]. Skipping.");
		    no_arrow = TRUE;
		}
		else if (length <= 0.0) {	/* use fabs() or theta+=180? */
		    G_debug(5, "Illegal arrow length [%.3f]. Skipping.",
			    length);
		    no_arrow = TRUE;
		}
	    }

	    if (no_arrow) {
		ptr = G_incr_void_ptr(ptr, Rast_cell_size(raster_type));
		if (opt7->answer)
		    mag_ptr =
			G_incr_void_ptr(mag_ptr,
					Rast_cell_size(mag_raster_type));
		no_arrow = FALSE;
		continue;
	    }

	    /* treat AGNPS and ANSWERS data like old zero-as-null CELL */
	    /*   TODO: update models */
	    if (map_type == 2 || map_type == 3) {
		if (Rast_is_null_value(ptr, raster_type))
		    aspect_c = 0;
		else
		    aspect_c = (int)(aspect_f + 0.5);
	    }


	    /** Now draw the arrows **/

	    /* case switch for standard GRASS aspect map 
	       measured in degrees counter-clockwise from east */
	    if (map_type == 1) {
		D_use_color(arrow_color);

		if (Rast_is_null_value(ptr, raster_type)) {
		    D_use_color(x_color);
		    draw_x();
		    D_use_color(arrow_color);
		}
		else if (aspect_f >= 0.0 && aspect_f <= 360.0) {
		    if (opt7->answer)
			arrow_mag(aspect_f, length);
		    else
			arrow_360(aspect_f);
		}
		else {
		    D_use_color(unknown_color);
		    unknown_();
		    D_use_color(arrow_color);
		}
	    }


	    /* case switch for AGNPS type aspect map */
	    else if (map_type == 2) {
		D_use_color(arrow_color);
		switch (aspect_c) {
		case 0:
		    D_use_color(x_color);
		    draw_x();
		    D_use_color(arrow_color);
		    break;
		case 1:
		    arrow_n();
		    break;
		case 2:
		    arrow_ne();
		    break;
		case 3:
		    arrow_e();
		    break;
		case 4:
		    arrow_se();
		    break;
		case 5:
		    arrow_s();
		    break;
		case 6:
		    arrow_sw();
		    break;
		case 7:
		    arrow_w();
		    break;
		case 8:
		    arrow_nw();
		    break;
		default:
		    D_use_color(unknown_color);
		    unknown_();
		    D_use_color(arrow_color);
		    break;
		}
	    }


	    /* case switch for ANSWERS type aspect map */
	    else if (map_type == 3) {
		D_use_color(arrow_color);
		if (aspect_c >= 15 && aspect_c <= 360)	/* start at zero? */
		    arrow_360((double)aspect_c);
		else if (aspect_c == 400) {
		    D_use_color(unknown_color);
		    unknown_();
		    D_use_color(arrow_color);
		}
		else {
		    D_use_color(x_color);
		    draw_x();
		    D_use_color(arrow_color);
		}
	    }

	    /* case switch for compass type aspect map
	       measured in degrees clockwise from north */
	    else if (map_type == 4) {
		D_use_color(arrow_color);

		if (Rast_is_null_value(ptr, raster_type)) {
		    D_use_color(x_color);
		    draw_x();
		    D_use_color(arrow_color);
		}
		else if (aspect_f >= 0.0 && aspect_f <= 360.0) {
		    if (opt7->answer)
			arrow_mag(90 - aspect_f, length);
		    else
			arrow_360(90 - aspect_f);
		}
		else {
		    D_use_color(unknown_color);
		    unknown_();
		    D_use_color(arrow_color);
		}
	    }

	    ptr = G_incr_void_ptr(ptr, Rast_cell_size(raster_type));
	    if (opt7->answer)
		mag_ptr =
		    G_incr_void_ptr(mag_ptr, Rast_cell_size(mag_raster_type));
	}
    }

    Rast_close(layer_fd);
    if (opt7->answer)
	Rast_close(mag_fd);

    D_save_command(G_recreate_command());
    D_close_driver();

    exit(EXIT_SUCCESS);
}

/* --- end of main --- */

/*---------------------------------------------------------------*/


static void arrow_mag(double theta, double length)
{				/* angle is measured in degrees counter-clockwise from east */
    double x, y, dx, dy, mid_x, mid_y;
    double theta_offset;

    theta *= -1;		/* display coords use inverse y */

    /* find the display coordinates of the middle of the cell */
    mid_x = col + (.5);
    mid_y = row + (.5);

    D_begin();

    /* tail */
    D_move_abs(mid_x, mid_y);

    /* head */
    x = mid_x + (length * cos(D2R(theta)));
    y = mid_y + (length * sin(D2R(theta)));
    D_cont_abs(x, y);

    /* fin 1 */
    theta_offset = theta + 20;
    dx = mid_x + (0.6 * length * cos(D2R(theta_offset)));
    dy = mid_y + (0.6 * length * sin(D2R(theta_offset)));
    D_cont_abs(dx, dy);

    /* fin 2 */
    D_move_abs(x, y);
    theta_offset = theta - 20;
    dx = mid_x + (0.6 * length * cos(D2R(theta_offset)));
    dy = mid_y + (0.6 * length * sin(D2R(theta_offset)));
    D_cont_abs(dx, dy);

    D_end();
    D_stroke();
}


static void arrow_360(double theta)
{				/* angle is measured in degrees counter-clockwise from east */
    double x, y, dx, dy, mid_x, mid_y;
    double max_radius, theta_offset;

    theta *= -1;		/* display coords use inverse y */
    max_radius = 0.8 / 2;

    /* find the display coordinates of the middle of the cell */
    mid_x = col + (0.5);
    mid_y = row + (0.5);

    D_begin();

    /* head */
    x = mid_x + (max_radius * cos(D2R(theta)));
    y = mid_y + (max_radius * sin(D2R(theta)));
    D_move_abs(x, y);

    /* tail */
    dx = -2 * (max_radius * cos(D2R(theta)));
    dy = -2 * (max_radius * sin(D2R(theta)));
    D_cont_rel(dx, dy);

    /* fin 1 */
    D_move_abs(x, y);
    theta_offset = theta + 90;
    dx = mid_x + (0.5 * max_radius * cos(D2R(theta_offset)));
    dy = mid_y + (0.5 * max_radius * sin(D2R(theta_offset)));
    D_cont_abs(dx, dy);

    /* fin 2 */
    D_move_abs(x, y);
    theta_offset = theta - 90;
    dx = mid_x + (0.5 * max_radius * cos(D2R(theta_offset)));
    dy = mid_y + (0.5 * max_radius * sin(D2R(theta_offset)));
    D_cont_abs(dx, dy);

    D_end();
    D_stroke();
}

static void arrow_se(void)
{
    double x = col + (.8);
    double y = row + (.8);
    D_begin();
    D_move_abs(x, y);
    D_cont_rel(((-.6)), (((-.6))));
    D_move_abs(x, y);
    D_cont_rel(0, ((-.4)));
    D_move_abs(x, y);
    D_cont_rel(((-.4)), 0);
    D_end();
    D_stroke();
}

static void arrow_ne(void)
{
    double x = col + (.8);
    double y = row + (.2);
    D_begin();
    D_move_abs(x, y);
    D_cont_rel(((-.6)), (((.6))));
    D_move_abs(x, y);
    D_cont_rel(0, ((.4)));
    D_move_abs(x, y);
    D_cont_rel(((-.4)), 0);
    D_end();
    D_stroke();
}

static void arrow_nw(void)
{
    double x = col + (.2);
    double y = row + (.2);
    D_begin();
    D_move_abs(x, y);
    D_cont_rel(((.6)), (((.6))));
    D_move_abs(x, y);
    D_cont_rel(0, ((.4)));
    D_move_abs(x, y);
    D_cont_rel(((.4)), 0);
    D_end();
    D_stroke();
}

static void arrow_sw(void)
{
    double x = col + (.2);
    double y = row + (.8);
    D_begin();
    D_move_abs(x, y);
    D_cont_rel(((.6)), (((-.6))));
    D_move_abs(x, y);
    D_cont_rel(0, ((-.4)));
    D_move_abs(x, y);
    D_cont_rel(((.4)), 0);
    D_end();
    D_stroke();
}

static void arrow_e(void)
{
    double x = col + (.9);
    double y = row + (.5);
    D_begin();
    D_move_abs(x, y);
    D_cont_rel(((-.8)), 0);
    D_move_abs(x, y);
    D_cont_rel(((-.3)), ((-.3)));
    D_move_abs(x, y);
    D_cont_rel(((-.3)), ((.3)));
    D_end();
    D_stroke();
}

static void arrow_w(void)
{
    double x = col + (.1);
    double y = row + (.5);
    D_begin();
    D_move_abs(x, y);
    D_cont_rel(((.8)), 0);
    D_move_abs(x, y);
    D_cont_rel(((.3)), ((-.3)));
    D_move_abs(x, y);
    D_cont_rel(((.3)), ((.3)));
    D_end();
    D_stroke();
}

static void arrow_s(void)
{
    double x = col + (.5);
    double y = row + (.9);
    D_begin();
    D_move_abs(x, y);
    D_cont_rel(0, ((-.8)));
    D_move_abs(x, y);
    D_cont_rel(((.3)), ((-.3)));
    D_move_abs(x, y);
    D_cont_rel(((-.3)), ((-.3)));
    D_end();
    D_stroke();
}

static void arrow_n(void)
{
    double x = col + (.5);
    double y = row + (.1);
    D_begin();
    D_move_abs(x, y);
    D_cont_rel(0, ((.8)));
    D_move_abs(x, y);
    D_cont_rel(((.3)), ((.3)));
    D_move_abs(x, y);
    D_cont_rel(((-.3)), ((.3)));
    D_end();
    D_stroke();
}

static void draw_x(void)
{
    double x = col;
    double y = row;
    D_begin();
    D_move_abs(x, y);
    D_cont_rel(1, 1);
    y = row + 1;
    D_move_abs(x, y);
    D_cont_rel(1, (-1));
    D_end();
    D_stroke();
}

static void unknown_(void)
{
    double x = col + (.3);
    double y = row + (.4);

    D_begin();
    D_move_abs(x, y);
    D_cont_rel(0, ((-.15)));
    D_cont_rel(((.1)), ((-.1)));
    D_cont_rel(((.2)), 0);
    D_cont_rel(((.1)), ((.1)));
    D_cont_rel(0, ((.2)));
    D_cont_rel(((-.1)), ((.1)));
    D_cont_rel(((-.1)), 0);
    D_cont_rel(0, ((.25)));
    D_move_rel(0, ((.1)));
    D_cont_rel(0, ((.1)));
    D_end();
    D_stroke();
}
