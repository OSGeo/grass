#include <math.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/display.h>
#include <grass/gprojects.h>
#include <grass/glocale.h>

#include "local_proto.h"


int plot_grid(double grid_size, double east, double north, int do_text,
	      int gcolor, int tcolor, int fontsize, int mark_type,
	      double line_width)
{
    double x, y, y0;
    double e1, e2;
    struct Cell_head window;
    double row_dist, colm_dist;
    char text[128];

    G_get_set_window(&window);

    /* pull right and bottom edges back one pixel; display lib bug? */
    row_dist = D_d_to_u_row(0.) - D_d_to_u_row(1.);
    colm_dist = D_d_to_u_col(1.) - D_d_to_u_col(0.);
    window.south = window.south + row_dist;
    window.east = window.east - colm_dist;


    /* Draw vertical grids */
    if (window.west > east)
	x = ceil((window.west - east) / grid_size) * grid_size + east;
    else
	x = east - ceil((east - window.west) / grid_size) * grid_size;

    while (x <= window.east) {

	if (mark_type == MARK_GRID) {
	    D_use_color(gcolor);
	    if (line_width)
		D_line_width(line_width);
	    D_line_abs(x, window.north, x, window.south);
	    D_line_width(0);	/* reset so text doesn't use it */
	}

	if (do_text) {
	    D_use_color(tcolor);
	    G_format_easting(x, text, G_projection());
	    D_text_rotation(270.0);
	    D_text_size(fontsize, fontsize);

	    /* Positioning -
	       x: 4 pixels to the right of the grid line, + 0.5 rounding factor.
	       y: End of text is 7 pixels up from bottom of screen, +.5 rounding.
	       fontsize*.81 = actual text width FOR DEFAULT FONT (NOT FreeType)
	     */
	    D_pos_abs(x + 4.5 * D_get_d_to_u_xconv(),
		       D_get_u_south()
		       - D_get_d_to_u_yconv() * (strlen(text) * fontsize * 0.81) - 7.5);

	    D_text(text);
	}
	x += grid_size;
    }
    D_text_rotation(0.0);	/* reset */


    /* Draw horizontal grids
     *
     * For latlon, must draw in shorter sections
     * to make sure that each section of the grid
     * line is less than half way around the globe
     */
    e1 = (window.east * 2 + window.west) / 3;
    e2 = (window.west * 2 + window.east) / 3;

    if (window.south > north)
	y = ceil((window.south - north) / grid_size) * grid_size + north;
    else
	y = north - ceil((north - window.south) / grid_size) * grid_size;

    while (y <= window.north) {
	if (mark_type == MARK_GRID) {
	    D_use_color(gcolor);
	    if (line_width)
		D_line_width(line_width);
	    D_line_abs(window.east, y, e1, y);
	    D_line_abs(e1, y, e2, y);
	    D_line_abs(e2, y, window.west, y);
	    D_line_width(0);	/* reset so text doesn't use it */
	}

	if (do_text) {
	    D_use_color(tcolor);
	    G_format_northing(y, text, G_projection());
	    D_text_size(fontsize, fontsize);

	    /* Positioning -
	       x: End of text is 7 pixels left from right edge of screen, +.5 rounding.
	       fontsize*.81 = actual text width FOR DEFAULT FONT (NOT FreeType)
	       y: 4 pixels above each grid line, +.5 rounding.
	     */
	    D_pos_abs(
		D_get_u_east()
		- D_get_d_to_u_xconv() * (strlen(text) * fontsize * 0.81 - 7.5),
		y + D_get_d_to_u_yconv() * 4.5);

	    D_text(text);
	}
	y += grid_size;
    }

    /* draw marks not grid lines */
    if (mark_type != MARK_GRID) {
	/* reset x and y */
	if (window.west > east)
	    x = ceil((window.west - east) / grid_size) * grid_size + east;
	else
	    x = east - ceil((east - window.west) / grid_size) * grid_size;
	if (window.south > north)
	    y0 = ceil((window.south - north) / grid_size) * grid_size + north;
	else
	    y0 = north - ceil((north - window.south) / grid_size) * grid_size;

	/* plot marks */
	while (x <= window.east) {
	    y = y0;		/* reset */
	    while (y <= window.north) {
		if (mark_type == MARK_CROSS)
		    plot_cross(x, y, gcolor, 0.0);
		else if (mark_type == MARK_FIDUCIAL)
		    plot_fiducial(x, y, gcolor, 0.0);
		else if (mark_type == MARK_DOT)
		    plot_dot(x, y, gcolor);
		y += grid_size;
	    }
	    x += grid_size;
	}
    }

    return 0;
}


int plot_geogrid(double size, struct pj_info info_in, struct pj_info info_out,
		 int do_text, int gcolor, int tcolor, int fontsize,
		 int mark_type, double line_width)
{
    double g;
    double e1, e2, n1, n2;
    double east, west, north, south;
    double start_coord;
    double lat, lon;
    int j, ll;
    int SEGS = 100;
    char text[128];
    float border_off = 4.5;
    float extra_y_off;
    float grid_off = 3.;
    double row_dist, colm_dist;
    float font_angle;
    struct Cell_head window;

    /* geo current region */
    G_get_set_window(&window);

    /* Adjust south and east back by one pixel for display bug? */
    row_dist = D_d_to_u_row(0.) - D_d_to_u_row(1.);
    colm_dist = D_d_to_u_col(1.) - D_d_to_u_col(0.);
    window.south += row_dist;
    window.east -= colm_dist;

    /* get lat long min max */
    /* probably need something like boardwalk ?? */
    get_ll_bounds(&west, &east, &south, &north, window, info_in, info_out);

    G_debug(3, "REGION BOUNDS N=%f S=%f E=%f W=%f", north, south, east, west);


    /* Lines of Latitude */
    g = floor(north / size) * size;
    e1 = east;
    for (j = 0; g >= south; j++, g -= size) {
	start_coord = -9999.;
	if (g == north || g == south)
	    continue;

	/* Set grid color */
	D_use_color(gcolor);

	for (ll = 0; ll < SEGS; ll++) {
	    n1 = n2 = g;
	    e1 = west + (ll * ((east - west) / SEGS));
	    e2 = e1 + ((east - west) / SEGS);
	    if (pj_do_proj(&e1, &n1, &info_in, &info_out) < 0)
		G_fatal_error(_("Error in pj_do_proj"));

	    check_coords(e1, n1, &lon, &lat, 1, window, info_in, info_out);
	    e1 = lon;
	    n1 = lat;
	    if (pj_do_proj(&e2, &n2, &info_in, &info_out) < 0)
		G_fatal_error(_("Error in pj_do_proj"));

	    check_coords(e2, n2, &lon, &lat, 1, window, info_in, info_out);
	    e2 = lon;
	    n2 = lat;
	    if (start_coord == -9999.) {
		start_coord = n1;
		font_angle = get_heading((e1 - e2), (n1 - n2));
	    }

	    if (line_width)
		D_line_width(line_width);

	    if (mark_type == MARK_GRID)
		D_line_abs(e1, n1, e2, n2);

	    D_line_width(0);
	}

	if (do_text) {
	    /* Set text color */
	    D_use_color(tcolor);

	    G_format_northing(g, text, PROJECTION_LL);
	    D_text_rotation(font_angle);
	    D_text_size(fontsize, fontsize);
	    D_pos_abs(D_get_u_west() + D_get_d_to_u_xconv() * border_off,
		      start_coord - D_get_d_to_u_yconv() * grid_off);
	    D_text(text);
	}
    }

    /* Lines of Longitude */
    g = floor(east / size) * size;
    n1 = north;
    for (j = 0; g > west; j++, g -= size) {
	start_coord = -9999.;
	extra_y_off = 0.0;
	if (g == east || g == west)
	    continue;

	/* Set grid color */
	D_use_color(gcolor);

	for (ll = 0; ll < SEGS; ll++) {
	    e1 = e2 = g;
	    n1 = north - (ll * ((north - south) / SEGS));
	    n2 = n1 - ((north - south) / SEGS);
	    /*
	       n1 = south + (ll *((north - south)/SEGS));
	       n2 = n1 + ((north - south)/SEGS);
	     */
	    if (pj_do_proj(&e1, &n1, &info_in, &info_out) < 0)
		G_fatal_error(_("Error in pj_do_proj"));

	    check_coords(e1, n1, &lon, &lat, 2, window, info_in, info_out);
	    e1 = lon;
	    n1 = lat;
	    if (pj_do_proj(&e2, &n2, &info_in, &info_out) < 0)
		G_fatal_error(_("Error in pj_do_proj"));

	    check_coords(e2, n2, &lon, &lat, 2, window, info_in, info_out);
	    e2 = lon;
	    n2 = lat;

	    if ((start_coord == -9999.) && (D_u_to_a_row(n1) > 0)) {
		font_angle = get_heading((e1 - e2), (n1 - n2));
		start_coord = e1;

		/* font rotates by bottom-left corner, try to keep top-left cnr on screen */
		if (font_angle - 270 > 0) {
		    extra_y_off = sin((font_angle - 270) * M_PI/180) * fontsize;
		    if (D_u_to_d_row(n1) - D_get_d_north() < extra_y_off + grid_off)
			start_coord = -9999.;  /* wait until the next point south */
		}
	    }

	    if (line_width)
		D_line_width(line_width);

	    if (mark_type == MARK_GRID)
		D_line_abs(e1, n1, e2, n2);

	    D_line_width(0);
	}
	if (do_text) {
	    /* Set text color */
	    D_use_color(tcolor);

	    G_format_easting(g, text, PROJECTION_LL);
	    D_text_rotation(font_angle);
	    D_text_size(fontsize, fontsize);
	    D_pos_abs(start_coord + D_get_d_to_u_xconv() * (grid_off + 1.5),
		      D_get_u_north() + D_get_d_to_u_yconv() *
		      (border_off + extra_y_off));
	    D_text(text);
	}
    }

    D_text_rotation(0.0);	/* reset */

    /* draw marks not grid lines */
    if (mark_type != MARK_GRID) {
	G_warning("Geogrid marks not yet implemented");
#ifdef TODO
	e1 = combine above;
	n1 = combine above;

	/* plot marks */
	while (e1 <= window.east) {
	    n1 = y0;		/* reset */
	    while (n1 <= window.north) {
		if (mark_type == MARK_CROSS)
		    plot_cross(e1, n1, gcolor, 0.0);
		else if (mark_type == MARK_FIDUCIAL)
		    plot_fiducial(e1, n1, gcolor, 0.0);
		else if (mark_type == MARK_DOT)
		    plot_dot(e1, n1, gcolor);
		n1 += grid_size;
	    }
	    e1 += grid_size;
	}
#endif
    /* also TODO: rotate cross and fiducial marks by the converge angle; see g.region -n */
    }

    return 0;

}

/******************************************************
 * initialze projection stuff and return proj structures
********************************************************/
void init_proj(struct pj_info *info_in, struct pj_info *info_out, int wgs84)
{
    struct Key_Value *out_proj_keys, *out_unit_keys;

    /* Proj stuff for geo grid */
    /* Out Info */
    out_proj_keys = G_get_projinfo();
    out_unit_keys = G_get_projunits();
    if (pj_get_kv(info_out, out_proj_keys, out_unit_keys) < 0)
	G_fatal_error(_("Can't get projection key values of current location"));

    /* In Info */
    if (!wgs84) {
	/* Set lat/long to same ellipsoid as location if we're not looking
	 * for the WGS84 values */
	if (GPJ_get_equivalent_latlong(info_in, info_out) < 0)
	    G_fatal_error(_("Unable to set up lat/long projection parameters"));

    }
    else {
	struct Key_Value *in_proj_info, *in_unit_info;
	char buff[100], dum[100];

	in_proj_info = G_create_key_value();
	in_unit_info = G_create_key_value();

	/* Check that datumparams are defined for this location (otherwise
	 * the WGS84 values would be meaningless), and if they are set the 
	 * input datum to WGS84 */
	if (G_get_datumparams_from_projinfo(out_proj_keys, buff, dum) < 0)
	    G_fatal_error(_("WGS84 grid output not possible as this location does not contain\n"
			   "datum transformation parameters. Try running g.setproj."));
	else
	    G_set_key_value("datum", "wgs84", in_proj_info);

	/* set input projection to lat/long */
	G_set_key_value("proj", "ll", in_proj_info);

	G_set_key_value("unit", "degree", in_unit_info);
	G_set_key_value("units", "degrees", in_unit_info);
	G_set_key_value("meters", "1.0", in_unit_info);

	if (pj_get_kv(info_in, in_proj_info, in_unit_info) < 0)
	    G_fatal_error(_("Unable to set up lat/long projection parameters"));

	G_free_key_value(in_proj_info);
	G_free_key_value(in_unit_info);
    }
    G_free_key_value(out_proj_keys);
    G_free_key_value(out_unit_keys);


    return;

}

/******************************************************
 * Use Proj to get min max bounds of region in lat long
********************************************************/
void
get_ll_bounds(double *w,
	      double *e,
	      double *s,
	      double *n,
	      struct Cell_head window,
	      struct pj_info info_in, struct pj_info info_out)
{
    double east, west, north, south;
    double e1, w1, n1, s1;
    double ew, ns;
    double ew_res, ns_res;
    int first;

    *w = *e = *n = *s = -999.;
    west = east = north = south = -999.;

    e1 = window.east;
    w1 = window.west;
    n1 = window.north;
    s1 = window.south;

    /* calculate resolution based upon 100 rows/cols
     * to prevent possible time consuming parsing in large regions
     */
    ew_res = (e1 - w1) / 100.;
    ns_res = (n1 - s1) / 100.;

    /* Get geographic min max from ala boardwalk style */
    /* North */
    first = 0;
    for (ew = window.west; ew <= window.east; ew += ew_res) {
	e1 = ew;
	n1 = window.north;
	if (pj_do_proj(&e1, &n1, &info_out, &info_in) < 0)
	    G_fatal_error(_("Error in pj_do_proj"));
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
    for (ew = window.west; ew <= window.east; ew += ew_res) {
	e1 = ew;
	s1 = window.south;
	if (pj_do_proj(&e1, &s1, &info_out, &info_in) < 0)
	    G_fatal_error(_("Error in pj_do_proj"));
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
    for (ns = window.south; ns <= window.north; ns += ns_res) {
	e1 = window.east;
	n1 = ns;
	if (pj_do_proj(&e1, &n1, &info_out, &info_in) < 0)
	    G_fatal_error(_("Error in pj_do_proj"));
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
    for (ns = window.south; ns <= window.north; ns += ns_res) {
	w1 = window.west;
	n1 = ns;
	if (pj_do_proj(&w1, &n1, &info_out, &info_in) < 0)
	    G_fatal_error(_("Error in pj_do_proj"));
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

/******************************************************
 * check projected coords to make sure they do not 
 * go outside region -- if so re-project 
********************************************************/
void
check_coords(double e,
	     double n,
	     double *lon,
	     double *lat,
	     int par,
	     struct Cell_head w,
	     struct pj_info info_in, struct pj_info info_out)
{
    double x, y;
    int proj = 0;

    *lat = y = n;
    *lon = x = e;

    if (e < w.west) {
	x = w.west;
	proj = 1;
    }
    if (e > w.east) {
	x = w.east;
	proj = 1;
    }
    if (n < w.south) {
	y = w.south;
	proj = 1;
    }
    if (n > w.north) {
	y = w.north;
	proj = 1;
    }

    if (proj) {
	/* convert original coords to ll */
	if (pj_do_proj(&e, &n, &info_out, &info_in) < 0)
	    G_fatal_error(_("Error in pj_do_proj1"));

	if (par == 1) {
	    /* lines of latitude -- const. northing */
	    /* convert correct UTM to ll */
	    if (pj_do_proj(&x, &y, &info_out, &info_in) < 0)
		G_fatal_error(_("Error in pj_do_proj2"));

	    /* convert new ll back to coords */
	    if (pj_do_proj(&x, &n, &info_in, &info_out) < 0)
		G_fatal_error(_("Error in pj_do_proj3"));
	    *lat = n;
	    *lon = x;
	}
	if (par == 2) {
	    /* lines of longitude -- const. easting */
	    /* convert correct UTM to ll */
	    if (pj_do_proj(&x, &y, &info_out, &info_in) < 0)
		G_fatal_error(_("Error in pj_do_proj5"));

	    /* convert new ll back to coords */
	    if (pj_do_proj(&e, &y, &info_in, &info_out) < 0)
		G_fatal_error(_("Error in pj_do_proj6"));
	    *lat = y;
	    *lon = e;
	}
    }

    return;
}

/*******************************************
 * function to calculate azimuth in degrees
 * from rows and columns
*******************************************/
float get_heading(double rows, double cols)
{
    float azi;

    /* NE Quad or due south */
    if (rows < 0 && cols <= 0) {
	azi = RAD_TO_DEG * atan((cols / rows));
	if (azi < 0.)
	    azi *= -1.;
    }
    /* SE Quad or due east */
    if (rows >= 0 && cols < 0) {
	azi = RAD_TO_DEG * atan((rows / cols));
	if (azi < 0.)
	    azi *= -1.;
	azi = 90. + azi;
    }

    /* SW Quad or due south */
    if (rows > 0 && cols >= 0) {
	azi = RAD_TO_DEG * atan((rows / cols));
	if (azi < 0.)
	    azi *= -1.;
	azi = 270. - azi;
    }

    /* NW Quad or due south */
    if (rows <= 0 && cols > 0) {
	azi = RAD_TO_DEG * atan((rows / cols));
	if (azi < 0.)
	    azi *= -1.;
	azi = 270. + azi;
    }

    return (azi);
}
