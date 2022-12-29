/* Functions: do_geogrid, do_geogrid_numbers, init_proj, get_ll_bounds
 **
 ** These functions are much modified versions of the p.map functions.
 **
 ** Author: Paul W. Carlson     April 1992
 */

#include <string.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/gprojects.h>

#include "local_proto.h"

#define LEFT 0
#define RIGHT 1
#define LOWER 0
#define UPPER 1
#define CENTER 2

/* number of segments in curved geographic lines */
#define SEGS 10

static void init_proj(void);
static struct pj_info info_in, info_out, info_trans;

static void get_ll_bounds(double *, double *, double *, double *);
static void check_coords(double, double, double *, double *, int);

/********************************************
 * Use proj library to create geographic grid
********************************************/
int do_geogrid(void)
{
    double g;
    double east, west, north, south;
    double e1, e2, n1, n2;
    double lat, lon;
    int j, ll;
    double grid;

    if (PS.geogrid <= 0)
	return 1;

    if (color_none(&PS.geogrid_color))
	return 1;

    grid = (double)PS.geogrid;	/* default to degrees */

    if (strncmp(PS.geogridunit, "d", 1) == 0)
	grid = (double)PS.geogrid;	/* degrees */
    if (strncmp(PS.geogridunit, "m", 1) == 0)
	grid = (double)PS.geogrid / 60.;	/* minutes */
    if (strncmp(PS.geogridunit, "s", 1) == 0)
	grid = (double)PS.geogrid / 3600.;	/* seconds */


    /* set color and set line width */
    set_ps_color(&PS.geogrid_color);
    set_line_width(PS.geogrid_width);

    /* draw horizontal lines in 3 pieces -- lat-lon lines must not
     * extend more than half the globe
     * start with first grid line just south of the window north
     */

    /* initialize projection stuff */
    init_proj();

    /* get lat long min max */
    /* probably need something like boardwalk ?? */
    get_ll_bounds(&west, &east, &south, &north);

    G_debug(1, "do_geogrid() LL BOUNDS:  w=%f  e=%f  s=%f  n=%f",
	    west, east, south, north);

    /* Lines of Latitude */
    g = floor(north / grid) * grid;
    e1 = east;
    for (j = 0; g >= south; j++, g -= grid) {
	if (g == north || g == south)
	    continue;
	for (ll = 0; ll < SEGS; ll++) {
	    n1 = n2 = g;
	    e1 = west + (ll * ((east - west) / SEGS));
	    e2 = e1 + ((east - west) / SEGS);
	    if (GPJ_transform(&info_in, &info_out, &info_trans, PJ_INV,
			      &e1, &n1, NULL) < 0)
		G_fatal_error(_("Error in GPJ_transform"));
	    check_coords(e1, n1, &lon, &lat, 1);
	    e1 = lon;
	    n1 = lat;
	    if (GPJ_transform(&info_in, &info_out, &info_trans, PJ_INV,
			      &e2, &n2, NULL) < 0)
		G_fatal_error(_("Error in GPJ_transform"));
	    check_coords(e2, n2, &lon, &lat, 1);
	    e2 = lon;
	    n2 = lat;
	    start_line(e1, n1);
	    sec_draw = 0;
	    G_plot_line(e1, n1, e2, n2);
	    fprintf(PS.fp, " D ");

	}
    }


    /* Lines of Longitude */
    g = floor(east / grid) * grid;
    n1 = north;
    for (j = 0; g > west; j++, g -= grid) {
	if (g == east || g == west)
	    continue;
	for (ll = 0; ll < SEGS; ll++) {
	    e1 = e2 = g;
	    n1 = south + (ll * ((north - south) / SEGS));
	    n2 = n1 + ((north - south) / SEGS);
	    if (GPJ_transform(&info_in, &info_out, &info_trans, PJ_INV,
			      &e1, &n1, NULL) < 0)
		G_fatal_error(_("Error in GPJ_transform"));
	    check_coords(e1, n1, &lon, &lat, 2);
	    e1 = lon;
	    n1 = lat;
	    if (GPJ_transform(&info_in, &info_out, &info_trans, PJ_INV,
			      &e2, &n2, NULL) < 0)
		G_fatal_error(_("Error in GPJ_transform"));
	    check_coords(e2, n2, &lon, &lat, 2);
	    e2 = lon;
	    n2 = lat;
	    start_line(e1, n1);
	    sec_draw = 0;
	    G_plot_line(e1, n1, e2, n2);
	    fprintf(PS.fp, " D ");
	}

    }

    return 0;
}


/*********************************************
 * do geographic grid numbers
*********************************************/
int do_geogrid_numbers(void)
{
    double g;
    double east, west, north, south;
    double e1, n1;
    double lat, lon;
    double grid;
    char num_text[50];
    int vy, vx /*, hy = 0, hx = 0 */;
    int x, y, last_bottom, last_right;
    /* int first = 1; */
    int margin;

    if (PS.geogrid <= 0 || PS.geogrid_numbers <= 0)
	return 1;

    /* initialize projection stuff */
    init_proj();

    grid = (double)PS.geogrid;	/* default to degrees */
    if (strncmp(PS.geogridunit, "d", 1) == 0)
	grid = (double)PS.geogrid;	/* degrees */
    if (strncmp(PS.geogridunit, "m", 1) == 0)
	grid = (double)PS.geogrid / 60.;	/* minutes */
    if (strncmp(PS.geogridunit, "s", 1) == 0)
	grid = (double)PS.geogrid / 3600.;	/* seconds */

    grid = grid * PS.geogrid_numbers;

    get_ll_bounds(&west, &east, &south, &north);

    /* initialize */
    set_font_name(PS.geogrid_font);
    set_font_size(PS.geogrid_fontsize);
    set_ps_color(&PS.geogrid_numbers_color);

    /* vertical grid numbers
     * these numbers only appear on the right edge
     * center the numbers on each grid line.
     * suppress number if it falls off the map or would overlay the previous
     *  label
     */
    margin = (int)(0.2 * (double)PS.geogrid_fontsize + 0.5);
    if (margin < 2)
	margin = 2;
    fprintf(PS.fp, "/mg %d def\n", margin);
    g = floor(north / grid) * grid;
    last_bottom = (int)PS.map_top;

    for (; g > south; g -= grid) {
	e1 = east;		/* draw at east boundary */
	n1 = g;
	if (GPJ_transform(&info_in, &info_out, &info_trans, PJ_INV,
			  &e1, &n1, NULL) < 0)
	    G_fatal_error(_("Error in GPJ_transform"));
	check_coords(e1, n1, &lon, &lat, 1);
	e1 = lon;
	n1 = lat;
	G_plot_where_xy(e1, n1, &vx, &vy);

	x = (double)vx / 10.;
	y = (double)vy / 10.;

	if (y + PS.grid_fontsize > last_bottom)
	    continue;
	if (y - PS.grid_fontsize < (int)PS.map_bot)
	    continue;
	G_format_northing(g, num_text, PROJECTION_LL);
	text_box_path(x, y, RIGHT, CENTER, num_text, 0);
	set_rgb_color(WHITE);
	fprintf(PS.fp, "F ");
	set_ps_color(&PS.geogrid_numbers_color);
	fprintf(PS.fp, "TIB\n");
	last_bottom = y - PS.geogrid_fontsize;
	/*
	if (first) {
	    first = 0;
	    hy = y + (int)(0.5 * (double)PS.geogrid_fontsize + 0.5) + margin;
	    hx = x + 0.7 * strlen(num_text) * PS.geogrid_fontsize +
		2 * margin;
	}
	*/
    }

    /* horizontal grid numbers along the bottom
     * center the numbers on each grid line.
     * suppress number if it falls of the map or would overlay the previous
     *  label
     */
    g = floor(west / grid) * grid;
    last_right = (int)PS.map_left;
    for (; g < east; g += grid) {
	e1 = g;
	n1 = south;		/* draw at south edge */
	if (GPJ_transform(&info_in, &info_out, &info_trans, PJ_INV,
			  &e1, &n1, NULL) < 0)
	    G_fatal_error(_("Error in GPJ_transform"));
	check_coords(e1, n1, &lon, &lat, 2);
	e1 = lon;
	n1 = lat;
	G_plot_where_xy(e1, n1, &vx, &vy);
	x = (double)vx / 10.;
	y = (double)vy / 10.;

	if (x - PS.grid_fontsize < last_right)
	    continue;
	if (x + PS.grid_fontsize > (int)PS.map_right)
	    continue;
	G_format_easting(g, num_text, PROJECTION_LL);

	vy = y - 0.7 * strlen(num_text) * PS.geogrid_fontsize - 2 * margin;
	vx = x - (int)(0.5 * (double)PS.grid_fontsize + 0.5) - margin;
	/*      if (vx < hx && vy < hy) continue; */
	fprintf(PS.fp, "ZB (%s) PB 90 rotate\n", num_text);
	fprintf(PS.fp, "%d bb sub bl add mg add\n", y);
	fprintf(PS.fp, "%d bt bb sub D2 add mg sub neg TR TB\n", x);
	set_rgb_color(WHITE);
	fprintf(PS.fp, "F ");
	set_ps_color(&PS.geogrid_numbers_color);
	fprintf(PS.fp, "TIB\n");
	last_right = x + PS.geogrid_fontsize;
    }

    return 0;
}

/******************************************************
 * initialize projection stuff and return proj structures
********************************************************/
void init_proj(void)
{
    struct Key_Value *in_proj_keys, *in_unit_keys;

    /* Proj stuff for geo grid */
    /* In Info (current location) */
    in_proj_keys = G_get_projinfo();
    in_unit_keys = G_get_projunits();
    if (pj_get_kv(&info_in, in_proj_keys, in_unit_keys) < 0)
	G_fatal_error(_("Can't get projection key values of current location"));
    G_free_key_value(in_proj_keys);
    G_free_key_value(in_unit_keys);

    /* Out Info (latlong equivalent of In Info */
    info_out.pj = NULL;
    info_trans.def = NULL;

    if (GPJ_init_transform(&info_in, &info_out, &info_trans) < 0)
	G_fatal_error(_("Unable to initialize coordinate transformation"));

    return;

}

/******************************************************
 * Use Proj to get min max bounds of region in lat long
********************************************************/
void get_ll_bounds(double *w, double *e, double *s, double *n)
{
    double east, west, north, south;
    double e1, w1, n1, s1;
    double ew, ns;
    int first;

    e1 = PS.w.east;
    w1 = PS.w.west;
    n1 = PS.w.north;
    s1 = PS.w.south;

    /* Get geographic min max from ala boardwalk style */
    /* North */
    first = 0;
    for (ew = PS.w.west; ew <= PS.w.east; ew += PS.w.ew_res) {
	e1 = ew;
	n1 = PS.w.north;
	if (GPJ_transform(&info_in, &info_out, &info_trans, PJ_FWD,
	                  &e1, &n1, NULL) < 0)
	    G_fatal_error(_("Error in GPJ_transform"));
	if (!first) {
	    north = n1;
	    first = 1;
	}
	else {
	    if (n1 > north)
		north = n1;
	}
    }
    /*South */
    first = 0;
    for (ew = PS.w.west; ew <= PS.w.east; ew += PS.w.ew_res) {
	e1 = ew;
	s1 = PS.w.south;
	if (GPJ_transform(&info_in, &info_out, &info_trans, PJ_FWD,
	                  &e1, &s1, NULL) < 0)
	    G_fatal_error(_("Error in GPJ_transform"));
	if (!first) {
	    south = s1;
	    first = 1;
	}
	else {
	    if (s1 < south)
		south = s1;
	}
    }

    /*East */
    first = 0;
    for (ns = PS.w.south; ns <= PS.w.north; ns += PS.w.ns_res) {
	e1 = PS.w.east;
	n1 = ns;
	if (GPJ_transform(&info_in, &info_out, &info_trans, PJ_FWD,
	                  &e1, &n1, NULL) < 0)
	    G_fatal_error(_("Error in GPJ_transform"));
	if (!first) {
	    east = e1;
	    first = 1;
	}
	else {
	    if (e1 > east)
		east = e1;
	}
    }

    /*West */
    first = 0;
    for (ns = PS.w.south; ns <= PS.w.north; ns += PS.w.ns_res) {
	w1 = PS.w.west;
	n1 = ns;
	if (GPJ_transform(&info_in, &info_out, &info_trans, PJ_FWD,
	                  &w1, &n1, NULL) < 0)
	    G_fatal_error(_("Error in GPJ_transform"));
	if (!first) {
	    west = w1;
	    first = 1;
	}
	else {
	    if (w1 < west)
		west = w1;
	}
    }

    *w = west;
    *e = east;
    *s = south;
    *n = north;

    return;
}


/********************************************************************
 * check projected coords to make sure they do not 
 *   go outside region -- if so re-project 
 * This gets a bit wobbly if the projection does not reverse-project
 *   cleanly, e.g. if the eastern edge of a UTM region is many
 *   degrees east of the end of the zone where things get distorted.
 *******************************************************************/
void check_coords(double e, double n, double *lon, double *lat, int par)
{
    double x, y;
    int proj = 0;

    *lat = y = n;
    *lon = x = e;

    if (e < PS.w.west) {
	x = PS.w.west;
	proj = 1;
    }
    if (e > PS.w.east) {
	x = PS.w.east;
	proj = 1;
    }
    if (n < PS.w.south) {
	y = PS.w.south;
	proj = 1;
    }
    if (n > PS.w.north) {
	y = PS.w.north;
	proj = 1;
    }


    if (proj) {
	/* convert original coords to ll */
	if (GPJ_transform(&info_in, &info_out, &info_trans, PJ_FWD,
	                  &e, &n, NULL) < 0)
	    G_fatal_error(_("Error in GPJ_transform"));

	if (par == 1) {
	    /* lines of latitude -- const. northing */
	    /* convert correct UTM to ll */
	    if (GPJ_transform(&info_in, &info_out, &info_trans, PJ_FWD,
			      &x, &y, NULL) < 0)
		G_fatal_error(_("Error in GPJ_transform"));

	    /* convert new ll back to coords */
	    if (GPJ_transform(&info_in, &info_out, &info_trans, PJ_INV,
			      &x, &n, NULL) < 0)
		G_fatal_error(_("Error in GPJ_transform"));
	    *lat = n;
	    *lon = x;
	}
	if (par == 2) {
	    /* lines of longitude -- const. easting */
	    /* convert correct UTM to ll */
	    if (GPJ_transform(&info_in, &info_out, &info_trans, PJ_FWD,
			      &x, &y, NULL) < 0)
		G_fatal_error(_("Error in GPJ_transform"));

	    /* convert new ll back to coords */
	    if (GPJ_transform(&info_in, &info_out, &info_trans, PJ_INV,
			      &e, &y, NULL) < 0)
		G_fatal_error(_("Error in GPJ_transform"));
	    *lat = y;
	    *lon = e;
	}
    }

    return;
}
