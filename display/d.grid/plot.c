#include <math.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/display.h>
#include <grass/gprojects.h>
#include <grass/glocale.h>

#include "local_proto.h"


int plot_grid(double grid_size, double east, double north, int do_text,
              int gcolor, int tcolor, int bgcolor, int fontsize,
              int mark_type, double line_width, int direction)
{
    double x, y, y0;
    double e1, e2;
    struct Cell_head window;
    double row_dist, colm_dist;
    char text[128];
    double tx, ty, bt, bb, bl, br, w, h;

    G_get_set_window(&window);

    /* pull right and bottom edges back one pixel; display lib bug? */
    row_dist = D_d_to_u_row(0.) - D_d_to_u_row(1.);
    colm_dist = D_d_to_u_col(1.) - D_d_to_u_col(0.);
    /*    window.south += row_dist;
       window.east -= colm_dist;
     */

    /* Draw vertical grids */
    if (window.west > east)
        x = ceil((window.west - east) / grid_size) * grid_size + east;
    else
        x = east - ceil((east - window.west) / grid_size) * grid_size;

    if (direction != DIRN_LAT) {
        while (x <= window.east) {
            if (mark_type == MARK_GRID) {
                D_use_color(gcolor);
                if (line_width)
                    D_line_width(line_width);
                D_line_abs(x, window.north, x, window.south);
                D_line_width(0);        /* reset so text doesn't use it */
            }
            x += grid_size;
        }
        D_text_rotation(0.0);   /* reset */
    }

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

    if (direction != DIRN_LON) {
        while (y <= window.north) {
            if (mark_type == MARK_GRID) {
                D_use_color(gcolor);
                if (line_width)
                    D_line_width(line_width);
                D_line_abs(window.east, y, e1, y);
                D_line_abs(e1, y, e2, y);
                D_line_abs(e2, y, window.west, y);
                D_line_width(0);        /* reset so text doesn't use it */
            }
            y += grid_size;
        }
        D_text_rotation(0.0);
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
            y = y0;             /* reset */
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

    /* Draw vertical labels */
    if (do_text) {
        if (window.west > east)
            x = ceil((window.west - east) / grid_size) * grid_size + east;
        else
            x = east - ceil((east - window.west) / grid_size) * grid_size;

        if (direction != DIRN_LAT) {
            while (x <= window.east) {
                D_use_color(tcolor);
                G_format_easting(x, text, G_projection());
                D_text_rotation(270.0);
                D_text_size(fontsize, fontsize);

                /* Positioning -
                   x: 4 pixels to the right of the grid line, + 0.5 rounding factor.
                   y: End of text is 7 pixels up from bottom of screen, +.5 rounding.
                   fontsize*.81 = actual text width FOR DEFAULT FONT (NOT FreeType)
                 */
                D_pos_abs(x + 4.5 * D_get_d_to_u_xconv(), D_get_u_south()
                          - D_get_d_to_u_yconv() * (strlen(text)
                          * fontsize * 0.81 + 7.5));

                tx = x + 4.5 * D_get_d_to_u_xconv();
                ty = D_get_u_south() -
                    D_get_d_to_u_yconv() * (strlen(text) * fontsize * 0.81 + 7.5);

                if (bgcolor != 0) {
                    D_get_text_box(text, &bt, &bb, &bl, &br);
                    w = br - bl;
                    h = bt - bb;

                    if (w > 0)
                        w += 0.2 * fontsize * fabs(D_get_d_to_u_xconv());
                    else  /* D_text() does not draw " ". */
                        w = 0.8 * fontsize * fabs(D_get_d_to_u_xconv());
                    if (h > 0)
                        h += 0.2 * fontsize * fabs(D_get_d_to_u_yconv());
                    else  /* D_text() does not draw " ". */
                        h = 0.8 * fontsize * fabs(D_get_d_to_u_yconv());

                    bl = tx - w/2;
                    bt = ty + h/10;
                    br = tx + w + w/2;
                    bb = ty - h - h/10;

                    D_use_color(bgcolor);
                    D_box_abs(bl, bt, br, bb);
                }

                D_use_color(tcolor);
                D_pos_abs(tx, ty);
                D_text(text);
                x += grid_size;
            }
            D_text_rotation(0.0);   /* reset */
        }
    }

    /* Draw horizontal labels */
    if (do_text) {
        if (window.south > north)
            y = ceil((window.south - north) / grid_size) * grid_size + north;
        else
            y = north - ceil((north - window.south) / grid_size) * grid_size;

        if (direction != DIRN_LON) {
            while (y <= window.north) {

                D_use_color(tcolor);
                G_format_northing(y, text, G_projection());
                D_text_size(fontsize, fontsize);

                /* Positioning -
                   x: End of text is 7 pixels left from right edge of screen, +.5 rounding.
                   fontsize*.81 = actual text width FOR DEFAULT FONT (NOT FreeType)
                   y: 4 pixels above each grid line, +.5 rounding.
                 */
                D_pos_abs(D_get_u_east()
                          -
                          D_get_d_to_u_xconv() * (strlen(text) * fontsize *
                                                  0.81 + 7.5),
                          y - D_get_d_to_u_yconv() * 4.5);

                tx = D_get_u_east() -
                     D_get_d_to_u_xconv() * (strlen(text) * fontsize * 0.81 +
                                             7.5);
                ty = y - D_get_d_to_u_yconv() * 4.5;

                if (bgcolor != 0) {
                    D_get_text_box(text, &bt, &bb, &bl, &br);
                    w = br - bl;
                    h = bt - bb;

                    if (w > 0)
                        w += 0.2 * fontsize * fabs(D_get_d_to_u_xconv());
                    else  /* D_text() does not draw " ". */
                        w = 0.8 * fontsize * fabs(D_get_d_to_u_xconv());
                    if (h > 0)
                        h += 0.2 * fontsize * fabs(D_get_d_to_u_yconv());
                    else  /* D_text() does not draw " ". */
                        h = 0.8 * fontsize * fabs(D_get_d_to_u_yconv());

                    bl = tx - w/10;
                    bt = ty + h + h/2;
                    br = tx + w + w/10;
                    bb = ty - h/2;
                    D_use_color(bgcolor);
                    D_box_abs(bl, bt, br, bb);
                }

                D_use_color(tcolor);
                D_pos_abs(tx, ty);
                D_text(text);

                y += grid_size;
            }
            D_text_rotation(0.0);
        }
    }

    return 0;
}


int plot_geogrid(double size, struct pj_info *info_in, struct pj_info *info_out,
                 struct pj_info *info_trans, int do_text, int gcolor, 
		 int tcolor, int bgcolor, int fontsize, int mark_type,
		 double line_width, int direction)
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
    double tx, ty, bt, bb, bl, br, w, h;

    /* geo current region */
    G_get_set_window(&window);

    /* Adjust south and east back by one pixel for display bug? */
    row_dist = D_d_to_u_row(0.) - D_d_to_u_row(1.);
    colm_dist = D_d_to_u_col(1.) - D_d_to_u_col(0.);
    window.south += row_dist;
    window.east -= colm_dist;

    /* get lat long min max */
    /* probably need something like boardwalk ?? */
    get_ll_bounds(&west, &east, &south, &north, window, info_in, info_out, info_trans);

    G_debug(3, "REGION BOUNDS N=%f S=%f E=%f W=%f", north, south, east, west);


    /* Lines of Latitude */
    g = floor(north / size) * size;
    e1 = east;
    for (j = 0; g >= south; j++, g -= size) {
        start_coord = -9999.;
        if (g == north || g == south || direction == DIRN_LON)
            continue;

        /* Set grid color */
        D_use_color(gcolor);

        for (ll = 0; ll < SEGS; ll++) {
            n1 = n2 = g;
            e1 = west + (ll * ((east - west) / SEGS));
            e2 = e1 + ((east - west) / SEGS);
	    if (GPJ_transform(info_in, info_out, info_trans, PJ_INV,
			      &e1, &n1, NULL) < 0)
		G_fatal_error(_("Error in GPJ_transform()"));

            check_coords(e1, n1, &lon, &lat, 1, window, info_in, info_out, info_trans);
            e1 = lon;
            n1 = lat;
	    if (GPJ_transform(info_in, info_out, info_trans, PJ_INV,
			      &e2, &n2, NULL) < 0)
		G_fatal_error(_("Error in GPJ_transform()"));

            check_coords(e2, n2, &lon, &lat, 1, window, info_in, info_out, info_trans);
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
        }

    /* Lines of Longitude */
    g = floor(east / size) * size;
    n1 = north;
    for (j = 0; g > west; j++, g -= size) {
        start_coord = -9999.;
        extra_y_off = 0.0;
        if (g == east || g == west || direction == DIRN_LAT)
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
	    if (GPJ_transform(info_in, info_out, info_trans, PJ_INV,
			      &e1, &n1, NULL) < 0)
		G_fatal_error(_("Error in GPJ_transform()"));

            check_coords(e1, n1, &lon, &lat, 2, window, info_in, info_out, info_trans);
            e1 = lon;
            n1 = lat;
	    if (GPJ_transform(info_in, info_out, info_trans, PJ_INV,
			      &e2, &n2, NULL) < 0)
		G_fatal_error(_("Error in GPJ_transform()"));

            check_coords(e2, n2, &lon, &lat, 2, window, info_in, info_out, info_trans);
            e2 = lon;
            n2 = lat;

            if ((start_coord == -9999.) && (D_u_to_a_row(n1) > 0)) {
                font_angle = get_heading((e1 - e2), (n1 - n2));
                start_coord = e1;

                /* font rotates by bottom-left corner, try to keep top-left cnr on screen */
                if (font_angle - 270 > 0) {
                    extra_y_off =
                        sin((font_angle - 270) * M_PI / 180) * fontsize;
                    if (D_u_to_d_row(n1) - D_get_d_north() <
                        extra_y_off + grid_off)
                        start_coord = -9999.;   /* wait until the next point south */
                }
            }

            if (line_width)
                D_line_width(line_width);

            if (mark_type == MARK_GRID)
                D_line_abs(e1, n1, e2, n2);

            D_line_width(0);
        }
        }
    D_text_rotation(0.0);       /* reset */

    /* draw marks not grid lines */
    if (mark_type != MARK_GRID) {
        G_warning(_("Geo-grid option only available for LL projection, use without -g/-w"));
#ifdef TODO
        e1 = combine above;
        n1 = combine above;

        /* plot marks */
        while (e1 <= window.east) {
            n1 = y0;            /* reset */
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

    /* geo current region */
    G_get_set_window(&window);

    /* Adjust south and east back by one pixel for display bug? */
    row_dist = D_d_to_u_row(0.) - D_d_to_u_row(1.);
    colm_dist = D_d_to_u_col(1.) - D_d_to_u_col(0.);
    window.south += row_dist;
    window.east -= colm_dist;

    /* get lat long min max */
    /* probably need something like boardwalk ?? */
    get_ll_bounds(&west, &east, &south, &north, window, info_in, info_out, info_trans);

    G_debug(3, "REGION BOUNDS N=%f S=%f E=%f W=%f", north, south, east, west);


    /* Labels of Latitude */
    if (do_text) {
        g = floor(north / size) * size;
        e1 = east;
        for (j = 0; g >= south; j++, g -= size) {
            start_coord = -9999.;
            if (g == north || g == south || direction == DIRN_LON)
                continue;

            /* Set grid color */
            for (ll = 0; ll < SEGS; ll++) {
                n1 = n2 = g;
                e1 = west + (ll * ((east - west) / SEGS));
                e2 = e1 + ((east - west) / SEGS);
		if (GPJ_transform(info_in, info_out, info_trans, PJ_INV,
				  &e1, &n1, NULL) < 0)
		    G_fatal_error(_("Error in GPJ_transform()"));

                check_coords(e1, n1, &lon, &lat, 1, window, info_in, info_out, info_trans);
                e1 = lon;
                n1 = lat;
		if (GPJ_transform(info_in, info_out, info_trans, PJ_INV,
				  &e2, &n2, NULL) < 0)
		    G_fatal_error(_("Error in GPJ_transform()"));

                check_coords(e2, n2, &lon, &lat, 1, window, info_in, info_out, info_trans);
                e2 = lon;
                n2 = lat;
                if (start_coord == -9999.) {
                    start_coord = n1;
                    font_angle = get_heading((e1 - e2), (n1 - n2));
                }
            }

            G_format_northing(g, text, PROJECTION_LL);
            D_text_rotation(font_angle);
            D_text_size(fontsize, fontsize);
            tx = D_get_u_west() + D_get_d_to_u_xconv() * border_off;
            ty = start_coord - D_get_d_to_u_yconv() * grid_off;

            if (bgcolor != 0) {
                D_get_text_box(text, &bt, &bb, &bl, &br);
                w = br - bl;
                h = bt - bb;
                bl = tx - w/10;
                bt = ty + h + h/2;
                br = tx + w + w/10;
                bb = ty - h/2;
                D_use_color(bgcolor);
                D_box_abs(bl, bt, br, bb);
            }

            D_use_color(tcolor);
            D_pos_abs(tx, ty);
            D_text(text);
        }
    }


    /* Labels of Longitude */
    if (do_text) {
        g = floor(east / size) * size;
        n1 = north;
        for (j = 0; g > west; j++, g -= size) {
            start_coord = -9999.;
            extra_y_off = 0.0;
            if (g == east || g == west || direction == DIRN_LAT)
                continue;

            for (ll = 0; ll < SEGS; ll++) {
                e1 = e2 = g;
                n1 = north - (ll * ((north - south) / SEGS));
                n2 = n1 - ((north - south) / SEGS);

		if (GPJ_transform(info_in, info_out, info_trans, PJ_INV,
				  &e1, &n1, NULL) < 0)
		    G_fatal_error(_("Error in GPJ_transform()"));

                check_coords(e1, n1, &lon, &lat, 2, window, info_in, info_out, info_trans);
                e1 = lon;
                n1 = lat;
		if (GPJ_transform(info_in, info_out, info_trans, PJ_INV,
				  &e2, &n2, NULL) < 0)
		    G_fatal_error(_("Error in GPJ_transform()"));

                check_coords(e2, n2, &lon, &lat, 2, window, info_in, info_out, info_trans);
                e2 = lon;
                n2 = lat;

                if ((start_coord == -9999.) && (D_u_to_a_row(n1) > 0)) {
                    font_angle = get_heading((e1 - e2), (n1 - n2));
                    start_coord = e1;

                    /* font rotates by bottom-left corner, try to keep top-left cnr on screen */
                    if (font_angle - 270 > 0) {
                        extra_y_off =
                            sin((font_angle - 270) * M_PI / 180) * fontsize;
                        if (D_u_to_d_row(n1) - D_get_d_north() <
                            extra_y_off + grid_off)
                            start_coord = -9999.;   /* wait until the next point south */
                    }
                }
            }

            G_format_easting(g, text, PROJECTION_LL);
            D_text_rotation(font_angle);
            D_text_size(fontsize, fontsize);
            tx = start_coord + D_get_d_to_u_xconv() * (grid_off + 1.5);
            ty = D_get_u_north() + D_get_d_to_u_yconv() * (border_off +
                                                           extra_y_off);

            if (bgcolor != 0) {
                D_get_text_box(text, &bt, &bb, &bl, &br);
                w = br - bl;
                h = bt - bb;
                bl = tx - w/2;
                bt = ty + h/10;
                br = tx + w + w/2;
                bb = ty - h - h/10;
                D_use_color(bgcolor);
                D_box_abs(bl, bt, br, bb);
            }

            D_use_color(tcolor);
            D_pos_abs(tx, ty);
            D_text(text);
        }
        D_text_rotation(0.0);       /* reset */
    }

    return 0;

}

/******************************************************
 * initialze projection stuff and return proj structures
********************************************************/
void init_proj(struct pj_info *info_in, struct pj_info *info_out, 
               struct pj_info *info_trans, int wgs84)
{
    struct Key_Value *in_proj_info, *in_unit_info;

    /* Proj stuff for geo grid */
    /* In Info for current projection */
    in_proj_info = G_get_projinfo();
    in_unit_info = G_get_projunits();
    if (pj_get_kv(info_in, in_proj_info, in_unit_info) < 0)
        G_fatal_error(_("Can't get projection key values of current location"));

    /* Out Info for ll projection */
    info_out->pj = NULL;
    if (wgs84) {
	struct Key_Value *out_proj_info, *out_unit_info;
        char buff[256], dum[256];

        out_proj_info = G_create_key_value();
        out_unit_info = G_create_key_value();

        /* Check that datumparams are defined for this location (otherwise
         * the WGS84 values would be meaningless), and if they are set the 
         * input datum to WGS84 */
#if PROJ_VERSION_MAJOR < 6
	/* PROJ6+ has its own datum transformation parameters */
        if (G_get_datumparams_from_projinfo(in_proj_info, buff, dum) < 0)
            G_fatal_error(_("WGS84 grid output not possible as this location does not contain\n"
                           "datum transformation parameters. Try running g.setproj."));
        else
#endif
            G_set_key_value("datum", "wgs84", out_proj_info);

        /* set input projection to lat/long */
        G_set_key_value("proj", "ll", out_proj_info);

        G_set_key_value("unit", "degree", out_unit_info);
        G_set_key_value("units", "degrees", out_unit_info);
        G_set_key_value("meters", "1.0", out_unit_info);

        if (pj_get_kv(info_out, out_proj_info, out_unit_info) < 0)
            G_fatal_error(_("Unable to set up lat/long projection parameters"));

	G_free_key_value(out_proj_info);
	G_free_key_value(out_unit_info);
    }
    /* else the latlong equivalent is generated by GPJ_init_transform() */

    G_free_key_value(in_proj_info);
    G_free_key_value(in_unit_info);

    info_trans->def = NULL;

    if (GPJ_init_transform(info_in, info_out, info_trans) < 0)
	G_fatal_error(_("Unable to initialize coordinate transformation"));

    return;

}

/******************************************************
 * Use Proj to get min max bounds of region in lat long
********************************************************/
void get_ll_bounds(double *w, double *e, double *s, double *n,
                   struct Cell_head window,
                   struct pj_info *info_in, struct pj_info *info_out,
		   struct pj_info *info_trans)
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
        if (GPJ_transform(info_in, info_out, info_trans, PJ_FWD,
	                  &e1, &n1, NULL) < 0)
            G_fatal_error(_("Error in GPJ_transform()"));
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
        if (GPJ_transform(info_in, info_out, info_trans, PJ_FWD,
	                  &e1, &s1, NULL) < 0)
            G_fatal_error(_("Error in GPJ_transform()"));
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
        if (GPJ_transform(info_in, info_out, info_trans, PJ_FWD,
	                  &e1, &n1, NULL) < 0)
            G_fatal_error(_("Error in GPJ_transform()"));
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
        if (GPJ_transform(info_in, info_out, info_trans, PJ_FWD,
	                  &w1, &n1, NULL) < 0)
            G_fatal_error(_("Error in GPJ_transform()"));
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
             struct pj_info *info_in,
	     struct pj_info *info_out,
	     struct pj_info *info_trans)
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
        if (GPJ_transform(info_in, info_out, info_trans, PJ_FWD,
	                  &e, &n, NULL) < 0)
            G_fatal_error(_("Error in GPJ_transform()"));

        if (par == 1) {
            /* lines of latitude -- const. northing */
            /* convert correct UTM to ll */
	    if (GPJ_transform(info_in, info_out, info_trans, PJ_FWD,
			      &x, &y, NULL) < 0)
		G_fatal_error(_("Error in GPJ_transform()"));

            /* convert new ll back to coords */
	    if (GPJ_transform(info_in, info_out, info_trans, PJ_INV,
			      &x, &n, NULL) < 0)
		G_fatal_error(_("Error in GPJ_transform()"));

            *lat = n;
            *lon = x;
        }
        if (par == 2) {
            /* lines of longitude -- const. easting */
            /* convert correct UTM to ll */
	    if (GPJ_transform(info_in, info_out, info_trans, PJ_FWD,
			      &x, &y, NULL) < 0)
		G_fatal_error(_("Error in GPJ_transform()"));

            /* convert new ll back to coords */
	    if (GPJ_transform(info_in, info_out, info_trans, PJ_INV,
			      &e, &y, NULL) < 0)
		G_fatal_error(_("Error in GPJ_transform()"));

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
