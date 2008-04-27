/*
 * $Id$
 *
 ****************************************************************************
 *
 * MODULE:       d.rast.arrow
 * AUTHOR(S):    Chris Rewerts, Agricultural Engineering, Purdue University
 * PURPOSE:      Draw arrows on slope/aspect maps. 
 * COPYRIGHT:    (C) 2000 by the GRASS Development Team
 *
 *		This program is free software under the GNU General Public
 *		License (>=v2). Read the file COPYING that comes with GRASS
 *		for details.
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

#define MAIN
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

int D_x, D_y ;
double D_ew, D_ns;
char *mapset;
char layer_name[128];
int layer_set;
int map_type, arrow_color, grid_color, x_color, unknown_color;


int main (int argc, char **argv)
{
    extern double D_ew, D_ns;
    extern int D_x, D_y ; 
    char window_name[128] ;
    struct Cell_head window ;
    int t, b, l, r ;
    char full_name[128] ;
    RASTER_MAP_TYPE raster_type, mag_raster_type = -1;
    int layer_fd;
    void *raster_row, *ptr;
    int nrows, ncols, row, col;
    int aspect_c = -1;
    float aspect_f = -1.0;
    double ew_res, ns_res;
    double D_south, D_west ;
    double D_north, D_east ;
    double U_to_D_xconv, U_to_D_yconv ;
    double U_west, U_south ;
    double U_east, U_north ;
    double U_start;
    double U_x, U_y ;

    double scale;
    int skip, no_arrow;
    char *mag_map = NULL, *mag_mapset = NULL;
    void *mag_raster_row = NULL, *mag_ptr = NULL;
    double length = -1;
    int mag_fd = -1;
    struct FPRange range;
    double mag_min, mag_max;

    struct GModule *module;
    struct Option *opt1, *opt2, *opt3, *opt4, *opt5,
		*opt6, *opt7, *opt8, *opt9;

    G_gisinit(argv[0]);

    module = G_define_module();
    module->keywords = _("display");
    module->description =
	_("Draws arrows representing cell aspect direction "
	  "for a raster map containing aspect data.");

    opt1 = G_define_option() ;
    opt1->key        = "map" ;
    opt1->type       = TYPE_STRING ;
    opt1->required   = NO ;
    opt1->multiple   = NO ;
    opt1->gisprompt  = "old,cell,raster" ;
    opt1->description= _("Name of raster aspect map to be displayed");

    opt2 = G_define_option() ;
    opt2->key        = "type" ;
    opt2->type       = TYPE_STRING ;
    opt2->required   = NO ;
    opt2->answer     = "grass" ;
    opt2->options    = "grass,compass,agnps,answers";
    opt2->description= _("Type of existing raster aspect map");

    opt3 = G_define_option() ;
    opt3->key        = "arrow_color" ;
    opt3->type       = TYPE_STRING ;
    opt3->required   = NO ;
    opt3->answer     = "green" ;
    opt3->options    = D_COLOR_LIST;
    opt3->description= _("Color for drawing arrows");

    opt4 = G_define_option() ;
    opt4->key        = "grid_color" ;
    opt4->type       = TYPE_STRING ;
    opt4->required   = NO ;
    opt4->answer     = "gray" ;
    opt4->options    = D_COLOR_LIST ",none";
    opt4->description= _("Color for drawing grid or \"none\"");

    opt5 = G_define_option() ;
    opt5->key        = "x_color" ;
    opt5->type       = TYPE_STRING ;
    opt5->required   = NO ;
    opt5->answer     = DEFAULT_FG_COLOR ;
    opt5->options    = D_COLOR_LIST;
    opt5->description= _("Color for drawing X's (Null values)");

    opt6 = G_define_option() ;
    opt6->key        = "unknown_color" ;
    opt6->type       = TYPE_STRING ;
    opt6->required   = NO ;
    opt6->answer     = "red" ;
    opt6->options    = D_COLOR_LIST; 
    opt6->description= _("Color for showing unknown information");

    opt9 = G_define_option();
    opt9->key	     = "skip";
    opt9->type       = TYPE_INTEGER;
    opt9->required   = NO;
    opt9->answer     = "1";
    opt9->description= _("Draw arrow every Nth grid cell");

    opt7 = G_define_option();
    opt7->key	     = "magnitude_map";
    opt7->type       = TYPE_STRING;
    opt7->required   = NO;
    opt7->multiple   = NO;
    opt7->gisprompt  = "old,cell,raster";
    opt7->description= _("Raster map containing values used for arrow length");

    opt8 = G_define_option();
    opt8->key	     = "scale";
    opt8->type       = TYPE_DOUBLE;
    opt8->required   = NO;
    opt8->answer     = "1.0";
    opt8->description= _("Scale factor for arrows (magnitude map)");


    /* Check command line */
    if (G_parser(argc, argv))
	exit(-1);


    if (opt1->answer) {
	G_strncpy(layer_name, opt1->answer, sizeof(layer_name)-1);
	if((mapset = G_find_cell2 (layer_name, "")) == NULL)
	    G_fatal_error(_("Raster map <%s> not found"), layer_name);
	layer_set = 1;
    }
    else
	layer_set = 0;


    arrow_color   = D_translate_color(opt3->answer) ;
    x_color       = D_translate_color(opt5->answer) ;
    unknown_color = D_translate_color(opt6->answer) ;

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
    if(scale <= 0.0)
	G_fatal_error(_("Illegal value for scale factor"));

    skip = atoi(opt9->answer);
    if(skip <= 0)
	G_fatal_error(_("Illegal value for skip factor"));


    if(opt7->answer) {
	if(map_type != 1 && map_type != 4)
	    G_fatal_error(_("Magnitude is only supported for GRASS and compass aspect maps."));

	mag_map = opt7->answer;
    	if((mag_mapset = G_find_cell2(mag_map, "")) == NULL)
    	    G_fatal_error(_("Raster map <%s> not found"), mag_map);
    }
    else if(scale != 1.0)
	G_warning(_("Scale option requires magnitude_map"));


/* Setup driver and check important information */
    if (R_open_driver() != 0)
	G_fatal_error (_("No graphics device selected"));

    if (D_get_cur_wind(window_name))
	G_fatal_error(_("No current window"));

    if (D_set_cur_wind(window_name))
	G_fatal_error(_("Current window not available"));

/* Read in the map window associated with window */
    G_get_window(&window) ;

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
    D_south = D_get_d_south() ;
    D_north = D_get_d_north() ;
    D_east =  D_get_d_east() ;
    D_west =  D_get_d_west() ;

    U_west = D_get_u_west() ;
    U_east = D_get_u_east() ;
    U_south = D_get_u_south() ;
    U_north = D_get_u_north() ;

    U_to_D_xconv = D_get_u_to_d_xconv() ;
    U_to_D_yconv = D_get_u_to_d_yconv() ;

/* number of rows and cols in window */
    nrows = window.rows;
    ncols = window.cols;

/*
    if ((nrows > 75) || (ncols > 75)){ 
	fprintf (stdout,"\n"); 
	fprintf (stdout,"Current window size:\n"); 
	fprintf (stdout,"rows:    %d\n", nrows);
	fprintf (stdout,"columns: %d\n", ncols);
	fprintf (stdout,"\n"); 
	fprintf (stdout,"Your current window setting may be too large.\n"); 
	fprintf (stdout,"Cells displayed on your graphics window may be too\n"); 
	fprintf (stdout,"small for arrows to be visible.\n\n"); 
	if (!G_yes("Do you wish to continue", 0))
	    exit(0);
    }
*/

/* resolutions */
    ew_res = window.ew_res;
    ns_res = window.ns_res;

/* how many screen units of distance for each cell */
    D_ew = (D_east - D_west) / ncols;
    D_ns = (D_south - D_north) / nrows; 

    /* figure out arrow scaling if using a magnitude map */
    if(opt7->answer) {
	G_init_fp_range(&range); /* really needed? */
	if (G_read_fp_range(mag_map, mag_mapset, &range) != 1 )
	    G_fatal_error (_("Problem reading range file"));
	G_get_fp_range_min_max(&range, &mag_min, &mag_max);

	scale *= 1.5*((D_ew < D_ns) ? D_ew : D_ns) / fabs(mag_max);
	G_debug(3, "scaling=%.2f  rast_max=%.2f  D_ew=%.2f", scale, mag_max, D_ew);
    }

/*------------------------------------------
    fprintf (stdout,"ew_res:  %.2f\n", window.ew_res);
    fprintf (stdout,"ns_res:  %.2f\n", window.ns_res);
    fprintf (stdout,"D_ew:  %f D_ns:  %f \n", D_ew, D_ns); 
    fprintf (stdout,"nrows:    %d\n", nrows);
    fprintf (stdout,"ncols:    %d\n", ncols);
    fprintf (stdout,"t:  %d\n", t);
    fprintf (stdout,"b:  %d\n", b);
    fprintf (stdout,"l:  %d\n", l);
    fprintf (stdout,"r:  %d\n", r);
    fprintf (stdout,"U_west:	%f\n", U_west);
    fprintf (stdout,"U_east:	%f\n", U_east);
    fprintf (stdout,"U_south:	%f\n", U_south);
    fprintf (stdout,"U_north:	%f\n", U_north);
    fprintf (stdout,"D_west:	%f\n", D_west);
    fprintf (stdout,"D_east:	%f\n", D_east);
    fprintf (stdout,"D_south:	%f\n", D_south);
    fprintf (stdout,"D_north:	%f\n", D_north);
    fprintf (stdout,"U_to_D_xconv:	%f\n", U_to_D_xconv);
    fprintf (stdout,"U_to_D_yconv:	%f\n", U_to_D_yconv);
--------------------------------------------------------*/

    if(grid_color > 0) { /* ie not "none" */
	/* Set color */
	R_standard_color(grid_color);

	/* Draw vertical grids */
	U_start = U_east;
	for (U_x=U_start; U_x>=U_west; U_x -= ew_res)
	{
	    D_x = (int)( ( U_x - U_west ) * U_to_D_xconv + D_west );
	    R_move_abs(D_x, (int)D_south) ;
	    R_cont_abs(D_x, (int)D_north) ;
	}

	/* Draw horizontal grids */
	U_start = U_north;
	for (U_y=U_start; U_y>=U_south; U_y -= ns_res)
	{
	    D_y = (int)( ( U_south - U_y ) * U_to_D_yconv + D_south );
	    R_move_abs((int)D_west, D_y) ;
	    R_cont_abs((int)D_east, D_y) ;
	}
    }

/* if we didn't get a layer name from the arg options, then
   get name of layer that is on the screen */
    if (!layer_set) {
	if(D_get_cell_name (full_name))
	    G_fatal_error(_("No raster map exists in the current window"));

	mapset = G_find_cell (full_name, "");
	if(mapset == NULL)
	    G_fatal_error(_("Raster map <%s> not found"), full_name);

	sscanf (full_name, "%s", layer_name);
    }

    /* open the raster map */
    layer_fd = G_open_cell_old (layer_name, mapset);
    if (layer_fd < 0)
	G_fatal_error(_("Unable to open raster map <%s>"), layer_name);

    raster_type = G_get_raster_map_type(layer_fd);

    /* allocate the cell array */
    raster_row = G_allocate_raster_buf(raster_type);


    if(opt7->answer) {
	/* open the magnitude raster map */
	mag_fd = G_open_cell_old(mag_map, mag_mapset);
	if(mag_fd < 0)
            G_fatal_error("Unable to open raster map <%s>", mag_map);

	mag_raster_type = G_get_raster_map_type(mag_fd);

	/* allocate the cell array */
	mag_raster_row = G_allocate_raster_buf(mag_raster_type);
    }


/* loop through cells, find value, determine direction (n,s,e,w,ne,se,sw,nw),
   and call appropriate function to draw an arrow on the cell */

    for(row = 0; row < nrows; row++) {
	G_get_raster_row (layer_fd, raster_row, row, raster_type);
	ptr = raster_row;

	if(opt7->answer) {
	    G_get_raster_row(mag_fd, mag_raster_row, row, mag_raster_type);
	    mag_ptr = mag_raster_row;
	}

	/* determine screen y coordinate of top of current cell */
	D_y = (int)(row * D_ns + D_north) ;

	for(col = 0; col < ncols; col++)
	{

	    if(row % skip != 0) no_arrow = TRUE;
	    else no_arrow = FALSE;

	    if(col % skip != 0) no_arrow = TRUE;

	    /* determine screen x coordinate of west side of current cell */
	    D_x = (int)(col * D_ew + D_west);

	    /* find aspect direction based on cell value */
	    if (raster_type == CELL_TYPE)
		aspect_f = *((CELL *) ptr);
	    else if (raster_type == FCELL_TYPE)
		aspect_f = *((FCELL *) ptr);
	    else if (raster_type == DCELL_TYPE)
		aspect_f = *((DCELL *) ptr);


	    if(opt7->answer) {

		if(mag_raster_type == CELL_TYPE)
		    length = *((CELL *) mag_ptr);
		else if(mag_raster_type == FCELL_TYPE)
		    length = *((FCELL *) mag_ptr);
	 	else if(mag_raster_type == DCELL_TYPE)
		    length = *((DCELL *) mag_ptr);

		length *= scale;

		if(G_is_null_value(mag_ptr, mag_raster_type)) {
		    G_debug(5, "Invalid arrow length [NULL]. Skipping.");
		    no_arrow = TRUE;
		}
		else if(length <= 0.0) { /* use fabs() or theta+=180? */
		    G_debug(5, "Illegal arrow length [%.3f]. Skipping.", length);
		    no_arrow = TRUE;
		}
	    }

	    if(no_arrow) {
		ptr = G_incr_void_ptr(ptr, G_raster_size(raster_type));
		if(opt7->answer)
		    mag_ptr = G_incr_void_ptr(mag_ptr, G_raster_size(mag_raster_type));
		no_arrow = FALSE;
		continue;
	    }

	    /* treat AGNPS and ANSWERS data like old zero-as-null CELL */
	    /*   TODO: update models */
	    if(map_type == 2 || map_type == 3) {
		if(G_is_null_value(ptr, raster_type))
		    aspect_c = 0;
		else
		    aspect_c = (int)(aspect_f + 0.5);
	    }


	    /** Now draw the arrows **/

	/* case switch for standard GRASS aspect map 
	    measured in degrees counter-clockwise from east */
	    if(map_type == 1) {
		R_standard_color(arrow_color);

		if(G_is_null_value(ptr, raster_type)) {
		    R_standard_color(x_color);
		    draw_x();
		    R_standard_color(arrow_color);
		}
		else if(aspect_f >= 0.0 && aspect_f <= 360.0) {
		    if(opt7->answer)
			arrow_mag(aspect_f, length);
		    else
			arrow_360(aspect_f);
		}
		else {
		    R_standard_color(unknown_color);
		    unknown_();
		    R_standard_color(arrow_color);
		}
	    }


	/* case switch for AGNPS type aspect map */
	    else if(map_type == 2) {
	      R_standard_color(arrow_color);
	      switch(aspect_c)
	      {
	       case 0:
		    R_standard_color(x_color);
		    draw_x();
		    R_standard_color(arrow_color);
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
		    R_standard_color(unknown_color);
		    unknown_();
		    R_standard_color(arrow_color);
		    break;
	       }
	     }


	/* case switch for ANSWERS type aspect map */
	    else if(map_type == 3) {
	      R_standard_color(arrow_color);
	      if(aspect_c >= 15 && aspect_c <= 360) /* start at zero? */
		arrow_360((double)aspect_c);
	      else if(aspect_c == 400) {
		R_standard_color(unknown_color);
		unknown_();
		R_standard_color(arrow_color);
	      }
	      else {
		R_standard_color(x_color);
		draw_x();
		R_standard_color(arrow_color);
	      }
	    }

	/* case switch for compass type aspect map
	     measured in degrees clockwise from north */
	    else if(map_type == 4) {
		R_standard_color(arrow_color);

		if(G_is_null_value(ptr, raster_type)) {
		    R_standard_color(x_color);
		    draw_x();
		    R_standard_color(arrow_color);
		}
		else if(aspect_f >= 0.0 && aspect_f <= 360.0) {
		    if(opt7->answer)
			arrow_mag(90 - aspect_f, length);
		    else
			arrow_360(90 - aspect_f);
		}
		else {
		    R_standard_color(unknown_color);
		    unknown_();
		    R_standard_color(arrow_color);
		}
	    }

	    ptr = G_incr_void_ptr(ptr, G_raster_size(raster_type));
	    if(opt7->answer)
		mag_ptr = G_incr_void_ptr(mag_ptr, G_raster_size(mag_raster_type));
	}
    }

    G_close_cell (layer_fd);
    if(opt7->answer) G_close_cell(mag_fd);
    D_add_to_list(G_recreate_command());
    R_close_driver();

    exit(0);
}

/* --- end of main --- */

/*---------------------------------------------------------------*/


static void arrow_mag(double theta, double length)
{ /* angle is measured in degrees counter-clockwise from east */
    extern double D_ew, D_ns;
    extern int D_x, D_y;
    int x,y, dx, dy, mid_x, mid_y;
    double theta_offset;

    theta *= -1; /* display coords use inverse y */

    /* find the display coordinates of the middle of the cell */
    mid_x = D_x + (int) (D_ew * .5);
    mid_y = D_y + (int) (D_ns * .5);

    /* tail */
    R_move_abs(mid_x, mid_y);

    /* head */
    x = mid_x + (int) (length * cos(D2R(theta)));
    y = mid_y + (int) (length * sin(D2R(theta)));
    R_cont_abs(x,y);

    /* fin 1 */
    theta_offset = theta+20;
    dx = mid_x + (int) (0.6 * length * cos(D2R(theta_offset)));
    dy = mid_y + (int) (0.6 * length * sin(D2R(theta_offset)));
    R_cont_abs(dx,dy);

    /* fin 2 */
    R_move_abs(x,y);
    theta_offset = theta-20;
    dx = mid_x + (int) (0.6 * length * cos(D2R(theta_offset)));
    dy = mid_y + (int) (0.6 * length * sin(D2R(theta_offset)));
    R_cont_abs(dx,dy);
}


static void arrow_360(double theta)
{ /* angle is measured in degrees counter-clockwise from east */
    extern double D_ew, D_ns;
    extern int D_x, D_y;
    int x,y, dx, dy, mid_x, mid_y;
    double max_radius, theta_offset;

    theta *= -1; /* display coords use inverse y */
    max_radius = ((D_ew < D_ns) ? D_ew : D_ns) * 0.8/2;

    /* find the display coordinates of the middle of the cell */
    mid_x = D_x + (int) (D_ew * 0.5);
    mid_y = D_y + (int) (D_ns * 0.5);

    /* head */
    x = mid_x + (int) (max_radius * cos(D2R(theta)));
    y = mid_y + (int) (max_radius * sin(D2R(theta)));
    R_move_abs(x,y);

    /* tail */
    dx = -2* (int) (max_radius * cos(D2R(theta)));
    dy = -2* (int) (max_radius * sin(D2R(theta)));
    R_cont_rel(dx,dy);

    /* fin 1 */
    R_move_abs(x,y);
    theta_offset = theta+90;
    dx = mid_x + (int) (0.5 * max_radius * cos(D2R(theta_offset)));
    dy = mid_y + (int) (0.5 * max_radius * sin(D2R(theta_offset)));
    R_cont_abs(dx,dy);

    /* fin 2 */
    R_move_abs(x,y);
    theta_offset = theta-90;
    dx = mid_x + (int) (0.5 * max_radius * cos(D2R(theta_offset)));
    dy = mid_y + (int) (0.5 * max_radius * sin(D2R(theta_offset)));
    R_cont_abs(dx,dy);

}

static void arrow_se (void)
{
    extern double D_ew, D_ns;
    extern int D_x, D_y;
    int x,y;

    x = D_x + (int) (D_ew * .8);
    y = D_y + (int) (D_ns * .8);
    R_move_abs(x,y);
    R_cont_rel((int)(D_ew *(-.6)),((int)(D_ns *(-.6))));
    R_move_abs(x,y);
    R_cont_rel(0, (int)(D_ns*(-.4)));
    R_move_abs(x,y);
    R_cont_rel((int)(D_ew*(-.4)),0);

}

static void arrow_ne (void)
{
    extern double D_ew, D_ns;
    extern int D_x, D_y;
    int x,y;

    x = D_x + (int) (D_ew * .8);
    y = D_y + (int) (D_ns * .2);
    R_move_abs(x,y);
    R_cont_rel((int)(D_ew *(-.6)),((int)(D_ns *(.6))));
    R_move_abs(x,y);
    R_cont_rel(0, (int)(D_ns*(.4)));
    R_move_abs(x,y);
    R_cont_rel((int)(D_ew*(-.4)),0);

}

static void arrow_nw (void)
{
    extern double D_ew, D_ns;
    extern int D_x, D_y;
    int x,y;

    x = D_x + (int) (D_ew * .2);
    y = D_y + (int) (D_ns * .2);
    R_move_abs(x,y);
    R_cont_rel((int)(D_ew *(.6)),((int)(D_ns *(.6))));
    R_move_abs(x,y);
    R_cont_rel(0, (int)(D_ns*(.4)));
    R_move_abs(x,y);
    R_cont_rel((int)(D_ew*(.4)),0);

}

static void arrow_sw (void)
{
    extern double D_ew, D_ns;
    extern int D_x, D_y;
    int x,y;

    x = D_x + (int) (D_ew * .2);
    y = D_y + (int) (D_ns * .8);
    R_move_abs(x,y);
    R_cont_rel((int)(D_ew *(.6)),((int)(D_ns *(-.6))));
    R_move_abs(x,y);
    R_cont_rel(0, (int)(D_ns*(-.4)));
    R_move_abs(x,y);
    R_cont_rel((int)(D_ew*(.4)),0);

}
static void arrow_e (void)
{
    extern double D_ew, D_ns;
    extern int D_x, D_y;
    int x,y;

    x = D_x + (int) (D_ew * .9);
    y = D_y + (int) (D_ns * .5);
    R_move_abs(x,y);
    R_cont_rel((int)(D_ew *(-.8)), 0);
    R_move_abs(x,y);
    R_cont_rel((int)(D_ew*(-.3)), (int)(D_ns*(-.3)));
    R_move_abs(x,y);
    R_cont_rel((int)(D_ew*(-.3)),(int)(D_ns*(.3)));

}
static void arrow_w (void)
{
    extern double D_ew, D_ns;
    extern int D_x, D_y;
    int x,y;

    x = D_x + (int) (D_ew * .1);
    y = D_y + (int) (D_ns * .5);
    R_move_abs(x,y);
    R_cont_rel((int)(D_ew *(.8)), 0);
    R_move_abs(x,y);
    R_cont_rel((int)(D_ew*(.3)), (int)(D_ns*(-.3)));
    R_move_abs(x,y);
    R_cont_rel((int)(D_ew*(.3)),(int)(D_ns*(.3)));

}
static void arrow_s (void)
{
    extern double D_ew, D_ns;
    extern int D_x, D_y;
    int x,y;

    x = D_x + (int) (D_ew * .5);
    y = D_y + (int) (D_ns * .9);
    R_move_abs(x,y);
    R_cont_rel(0, (int)(D_ns *(-.8)));
    R_move_abs(x,y);
    R_cont_rel((int)(D_ew*(.3)), (int)(D_ns*(-.3)));
    R_move_abs(x,y);
    R_cont_rel((int)(D_ew*(-.3)),(int)(D_ns*(-.3)));

}
static void arrow_n (void)
{
    extern double D_ew, D_ns;
    extern int D_x, D_y;
    int x,y;

    x = D_x + (int) (D_ew * .5);
    y = D_y + (int) (D_ns * .1);
    R_move_abs(x,y);
    R_cont_rel(0, (int)(D_ns *(.8)));
    R_move_abs(x,y);
    R_cont_rel((int)(D_ew*(.3)), (int)(D_ns*(.3)));
    R_move_abs(x,y);
    R_cont_rel((int)(D_ew*(-.3)),(int)(D_ns*(.3)));

}
static void draw_x (void)
{
    extern double D_ew, D_ns;
    extern int D_x, D_y;
    int x,y;

    x = D_x;
    y = D_y;
    R_move_abs(x,y);
    R_cont_rel((int)D_ew, (int)D_ns );
    y = D_y + (int)D_ns;
    R_move_abs(x,y);
    R_cont_rel((int)D_ew, (int)(D_ns * -1));
}
static void unknown_ (void)
{
    extern double D_ew, D_ns;
    extern int D_x, D_y;
    int x,y;

    x = D_x + (int) (D_ew * .3);
    y = D_y + (int) (D_ns * .4);
    R_move_abs(x,y);
    R_cont_rel(0, (int)(D_ns *(-.15)));
    R_cont_rel((int)(D_ew *(.1)), (int)(D_ns *(-.1)));
    R_cont_rel((int)(D_ew *(.2)), 0);
    R_cont_rel((int)(D_ew *(.1)), (int)(D_ns *(.1)));
    R_cont_rel(0, (int)(D_ns *(.2)));
    R_cont_rel((int)(D_ew *(-.1)), (int)(D_ns *(.1)));
    R_cont_rel((int)(D_ew *(-.1)), 0);
    R_cont_rel(0, (int)(D_ns *(.25)));
    R_move_rel(0, (int)(D_ns *(.1)));
    R_cont_rel(0, (int)(D_ns *(.1)));
}
