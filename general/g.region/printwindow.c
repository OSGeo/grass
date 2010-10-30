#include <string.h>
#include <stdlib.h>
#include <projects.h>
#include <grass/gis.h>
#include <grass/gprojects.h>
#include <grass/glocale.h>
#include "local_proto.h"

#define DEG2RAD(a) ((a) * M_PI / 180.0)
#define RAD2DEG(a) ((a) * 180.0 / M_PI)


void print_window(struct Cell_head *window, int print_flag)
{
    const char *prj, *datum, *ellps;
    int x, width = 11;

    char north[30], south[30], east[30], west[30], nsres[30], ewres[30],
	nsres3[30], ewres3[30], tbres[30];
    char buf[50];

    double ew_dist1, ew_dist2, ns_dist1, ns_dist2;
    double longitude, latitude;

    if (print_flag & PRINT_SH)
	x = G_projection() == PROJECTION_LL ? -1 : 0;
    else
	x = window->proj;

    G_format_northing(window->north, north, x);
    G_format_northing(window->south, south, x);
    G_format_easting(window->east, east, x);
    G_format_easting(window->west, west, x);
    G_format_resolution(window->ew_res, ewres, x);
    G_format_resolution(window->ew_res3, ewres3, x);
    G_format_resolution(window->ns_res, nsres, x);
    G_format_resolution(window->ns_res3, nsres3, x);
    G_format_resolution(window->tb_res, tbres, -1);
    G_begin_distance_calculations();

    /* EW Dist at North edge */
    ew_dist1 =
	G_distance(window->east, window->north, window->west, window->north);
    /* EW Dist at South Edge */
    ew_dist2 =
	G_distance(window->east, window->south, window->west, window->south);
    /* NS Dist at East edge */
    ns_dist1 =
	G_distance(window->east, window->north, window->east, window->south);
    /* NS Dist at West edge */
    ns_dist2 =
	G_distance(window->west, window->north, window->west, window->south);

    /*  width */
    if (print_flag & PRINT_REG)
	width = 11;

    if (print_flag & PRINT_CENTER || print_flag & PRINT_MBBOX)
	width = 16;

    if (print_flag & PRINT_LL || print_flag & PRINT_NANGLE)
	width = 18;

    if (print_flag & PRINT_EXTENT)
	width = 19;

    /* flag.dist_res */
    if (print_flag & PRINT_METERS) {
	sprintf(ewres, "%.8f", ((ew_dist1 + ew_dist2) / 2) / window->cols);
	G_trim_decimal(ewres);
	sprintf(ewres3, "%.8f", ((ew_dist1 + ew_dist2) / 2) / window->cols3);
	G_trim_decimal(ewres3);
	sprintf(nsres, "%.8f", ((ns_dist1 + ns_dist2) / 2) / window->rows);
	G_trim_decimal(nsres);
	sprintf(nsres3, "%.8f", ((ns_dist1 + ns_dist2) / 2) / window->rows3);
	G_trim_decimal(nsres3);
	sprintf(tbres, "%.8f",
		(window->top - window->bottom) / window->depths);
	G_trim_decimal(tbres);
    }

    /* flag.print & flag.gprint */
    if (print_flag & PRINT_REG) {
	prj = G_database_projection_name();
	if (!prj)
	    prj = "** unknown **";
	/*
	   please remove before GRASS 7 released
	   backward compatibility issue

	   if (print_flag & PRINT_SH)
	   {
	   fprintf(stdout, "projection=%d\n", window->proj);
	   fprintf(stdout, "zone=%d\n", window->zone);
	   }
	   else
	   {
	   fprintf(stdout, "%-*s: %d (%s)\n", width, "projection", window->proj, prj);
	   fprintf(stdout, "%-*s: %d\n", width, "zone", window->zone);
	   }
	 */

	if (!(print_flag & PRINT_SH)) {
	    fprintf(stdout, "%-*s %d (%s)\n", width, "projection:",
		    window->proj, prj);
	    fprintf(stdout, "%-*s %d\n", width, "zone:", window->zone);
	}

	/* don't print datum/ellipsoid in XY-Locations */
	if (window->proj != 0) {
	    datum = G_database_datum_name();
	    if (!datum)
		datum = "** unknown (default: WGS84) **";
	    ellps = G_database_ellipse_name();
	    if (!ellps)
		ellps = "** unknown (default: WGS84) **";

	    /*
	       please remove before GRASS 7 released
	       backward compatibility issue

	       if (print_flag & PRINT_SH)
	       {
	       if (datum[0] != '*')
	       fprintf(stdout, "datum=%s\n", datum);
	       else
	       fprintf(stdout, "datum=wgs84\n");
	       if (ellps[0] != '*')
	       fprintf(stdout, "ellipsoid=%s\n", ellps);
	       else
	       fprintf(stdout, "ellipsoid=wgs84\n");
	       }
	       else
	       {
	       fprintf(stdout, "%-*s %s\n", width, "datum:", datum);
	       fprintf(stdout, "%-*s %s\n", width, "ellipsoid:", ellps);
	       }
	     */

	    if (!(print_flag & PRINT_SH)) {
		fprintf(stdout, "%-*s %s\n", width, "datum:", datum);
		fprintf(stdout, "%-*s %s\n", width, "ellipsoid:", ellps);
	    }
	}

	if (print_flag & PRINT_SH) {
	    fprintf(stdout, "n=%s\n", north);
	    fprintf(stdout, "s=%s\n", south);
	    fprintf(stdout, "w=%s\n", west);
	    fprintf(stdout, "e=%s\n", east);
	    if (print_flag & PRINT_3D) {
		fprintf(stdout, "t=%g\n", window->top);
		fprintf(stdout, "b=%g\n", window->bottom);
	    }
	    fprintf(stdout, "nsres=%s\n", nsres);
	    if (print_flag & PRINT_3D) {
		fprintf(stdout, "nsres3=%s\n", nsres3);
	    }
	    fprintf(stdout, "ewres=%s\n", ewres);
	    if (print_flag & PRINT_3D) {
		fprintf(stdout, "ewres3=%s\n", ewres3);
		fprintf(stdout, "tbres=%s\n", tbres);
	    }
	    fprintf(stdout, "rows=%d\n", window->rows);
	    if (print_flag & PRINT_3D)
		fprintf(stdout, "rows3=%d\n", window->rows3);
	    fprintf(stdout, "cols=%d\n", window->cols);
	    if (print_flag & PRINT_3D) {
		fprintf(stdout, "cols3=%d\n", window->cols3);
		fprintf(stdout, "depths=%d\n", window->depths);
	    }
#ifdef HAVE_LONG_LONG_INT
	    fprintf(stdout, "cells=%lld\n",
		    (long long)window->rows * window->cols);
	    if (print_flag & PRINT_3D)
		fprintf(stdout, "cells3=%lld\n",
			(long long)window->rows3 * window->cols3 *
			window->depths);
#else
	    fprintf(stdout, "cells=%ld\n", (long)window->rows * window->cols);
	    if (print_flag & PRINT_3D)
		fprintf(stdout, "cells3=%ld\n",
			(long)window->rows3 * window->cols3 * window->depths);
#endif
	}
	else {
	    fprintf(stdout, "%-*s %s\n", width, "north:", north);
	    fprintf(stdout, "%-*s %s\n", width, "south:", south);
	    fprintf(stdout, "%-*s %s\n", width, "west:", west);
	    fprintf(stdout, "%-*s %s\n", width, "east:", east);
	    if (print_flag & PRINT_3D) {
		fprintf(stdout, "%-*s %.8f\n", width, "top:", window->top);
		fprintf(stdout, "%-*s %.8f\n", width, "bottom:",
			window->bottom);
	    }
	    fprintf(stdout, "%-*s %s\n", width, "nsres:", nsres);
	    if (print_flag & PRINT_3D) {
		fprintf(stdout, "%-*s %s\n", width, "nsres3:", nsres3);
	    }
	    fprintf(stdout, "%-*s %s\n", width, "ewres:", ewres);
	    if (print_flag & PRINT_3D) {
		fprintf(stdout, "%-*s %s\n", width, "ewres3:", ewres3);
		fprintf(stdout, "%-*s %s\n", width, "tbres:", tbres);
	    }

	    fprintf(stdout, "%-*s %d\n", width, "rows:", window->rows);
	    if (print_flag & PRINT_3D) {
		fprintf(stdout, "%-*s %d\n", width, "rows3:", window->rows3);
	    }
	    fprintf(stdout, "%-*s %d\n", width, "cols:", window->cols);
	    if (print_flag & PRINT_3D) {
		fprintf(stdout, "%-*s %d\n", width, "cols3:", window->cols3);
		fprintf(stdout, "%-*s %d\n", width, "depths:",
			window->depths);
	    }
#ifdef HAVE_LONG_LONG_INT
	    fprintf(stdout, "%-*s %lld\n", width, "cells:",
		    (long long)window->rows * window->cols);
	    if (print_flag & PRINT_3D)
		fprintf(stdout, "%-*s %lld\n", width, "cells3:",
			(long long)window->rows3 * window->cols3 *
			window->depths);
#else
	    fprintf(stdout, "%-*s %ld\n", width, "cells:",
		    (long)window->rows * window->cols);
	    if (print_flag & PRINT_3D)
		fprintf(stdout, "%-*s %ld\n", width, "cells3:",
			(long)window->rows3 * window->cols3 * window->depths);
#endif
	}
    }

    /* flag.lprint: show boundaries in lat/long  MN 2001 */
    if (print_flag & PRINT_LL) {
	double lo1, la1, lo2, la2, lo3, la3, lo4, la4;
	double mid_n_lo, mid_n_la, mid_s_lo, mid_s_la, mid_w_lo, mid_w_la,
	    mid_e_lo, mid_e_la;

	/* if coordinates are not in lat/long format, transform them: */
	if ((G_projection() != PROJECTION_LL) && window->proj != 0) {
	    /* projection information of input map */
	    struct Key_Value *in_proj_info, *in_unit_info;
	    struct pj_info iproj;	/* input map proj parameters  */
	    struct pj_info oproj;	/* output map proj parameters  */

	    /* read current projection info */
	    if ((in_proj_info = G_get_projinfo()) == NULL)
		G_fatal_error(_("Can't get projection info of current location"));

	    if ((in_unit_info = G_get_projunits()) == NULL)
		G_fatal_error(_("Can't get projection units of current location"));

	    if (pj_get_kv(&iproj, in_proj_info, in_unit_info) < 0)
		G_fatal_error(_("Can't get projection key values of current location"));

	    G_free_key_value(in_proj_info);
	    G_free_key_value(in_unit_info);

	    /*  output projection to lat/long w/ same ellipsoid as input */
	    oproj.zone = 0;
	    oproj.meters = 1.;
	    sprintf(oproj.proj, "ll");
	    if ((oproj.pj = pj_latlong_from_proj(iproj.pj)) == NULL)
		G_fatal_error(_("Unable to update lat/long projection parameters"));

	    /* for DEBUG
	       pj_print_proj_params(&iproj,&oproj);
	     */

	    /* do the transform
	     * syntax: pj_do_proj(outx, outy, in_info, out_info) 
	     *
	     *  1 ------ 2
	     *  |        |  map corners
	     *  |        |
	     *  4--------3
	     */

	    latitude = window->north;
	    longitude = window->west;
	    if (pj_do_proj(&longitude, &latitude, &iproj, &oproj) < 0)
		G_fatal_error(_("Error in pj_do_proj (projection of input coordinate pair)"));

	    lo1 = longitude;
	    la1 = latitude;

	    latitude = window->north;
	    longitude = window->east;
	    if (pj_do_proj(&longitude, &latitude, &iproj, &oproj) < 0)
		G_fatal_error(_("Error in pj_do_proj (projection of input coordinate pair)"));

	    lo2 = longitude;
	    la2 = latitude;

	    latitude = window->south;
	    longitude = window->east;
	    if (pj_do_proj(&longitude, &latitude, &iproj, &oproj) < 0)
		G_fatal_error(_("Error in pj_do_proj (projection of input coordinate pair)"));

	    lo3 = longitude;
	    la3 = latitude;

	    latitude = window->south;
	    longitude = window->west;
	    if (pj_do_proj(&longitude, &latitude, &iproj, &oproj) < 0)
		G_fatal_error(_("Error in pj_do_proj (projection of input coordinate pair)"));

	    lo4 = longitude;
	    la4 = latitude;

	    /* 
	     * map corners and side mids:
	     *          mid_n
	     *       1 ---+---2
	     *       |        |
	     * mid_w +        + mid_e 
	     *       |        |
	     *       4----+---3
	     *          mid_s
	     *
	     * lo: longitude
	     * la: latitude
	     */

	    /* side mids for easting, northing center: */
	    mid_n_lo = (lo2 + lo1) / 2.;
	    mid_n_la = (la2 + la1) / 2.;
	    mid_s_lo = (lo3 + lo4) / 2.;
	    mid_s_la = (la3 + la4) / 2.;
	    mid_w_lo = (lo1 + lo4) / 2.;	/* not needed */
	    mid_w_la = (la1 + la4) / 2.;	/* not needed */
	    mid_e_lo = (lo3 + lo2) / 2.;	/* not needed */
	    mid_e_la = (la3 + la2) / 2.;	/* not needed */

	    G_debug(3, "mid_n_lo %f", mid_n_lo);
	    G_debug(3, "mid_s_lo %f", mid_s_lo);
	    G_debug(3, "mid_n_la %f", mid_n_la);
	    G_debug(3, "mid_s_la %f", mid_s_la);
	    G_debug(3, "mid_w_lo %f", mid_w_lo);
	    G_debug(3, "mid_e_lo %f", mid_e_lo);
	    G_debug(3, "mid_w_la %f", mid_w_la);
	    G_debug(3, "mid_e_la %f", mid_e_la);

	    if (print_flag & PRINT_SH) {
		fprintf(stdout, "nw_long=%.8f\nnw_lat=%.8f\n", lo1, la1);
		fprintf(stdout, "ne_long=%.8f\nne_lat=%.8f\n", lo2, la2);
		fprintf(stdout, "se_long=%.8f\nse_lat=%.8f\n", lo3, la3);
		fprintf(stdout, "sw_long=%.8f\nsw_lat=%.8f\n", lo4, la4);
		G_format_easting((mid_n_lo + mid_s_lo) / 2., buf,
				 PROJECTION_LL);
		fprintf(stdout, "center_long=%.8f\n",
			(mid_n_lo + mid_s_lo) / 2.);
		G_format_northing((mid_n_la + mid_s_la) / 2., buf,
				  PROJECTION_LL);
		fprintf(stdout, "center_lat=%.8f\n",
			(mid_n_la + mid_s_la) / 2.);

	    }
	    else {
		G_format_easting(lo1, buf, PROJECTION_LL);
		fprintf(stdout, "%-*s long: %s ", width, "north-west corner:",
			buf);
		G_format_northing(la1, buf, PROJECTION_LL);
		fprintf(stdout, "lat: %s\n", buf);

		G_format_easting(lo2, buf, PROJECTION_LL);
		fprintf(stdout, "%-*s long: %s ", width, "north-east corner:",
			buf);
		G_format_northing(la2, buf, PROJECTION_LL);
		fprintf(stdout, "lat: %s\n", buf);

		G_format_easting(lo3, buf, PROJECTION_LL);
		fprintf(stdout, "%-*s long: %s ", width, "south-east corner:",
			buf);
		G_format_northing(la3, buf, PROJECTION_LL);
		fprintf(stdout, "lat: %s\n", buf);

		G_format_easting(lo4, buf, PROJECTION_LL);
		fprintf(stdout, "%-*s long: %s ", width, "south-west corner:",
			buf);
		G_format_northing(la4, buf, PROJECTION_LL);
		fprintf(stdout, "lat: %s\n", buf);

		G_format_easting((mid_n_lo + mid_s_lo) / 2., buf,
				 PROJECTION_LL);
		fprintf(stdout, "%-*s %11s\n", width, "center longitude:",
			buf);

		G_format_northing((mid_n_la + mid_s_la) / 2., buf,
				  PROJECTION_LL);
		fprintf(stdout, "%-*s %11s\n", width, "center latitude:",
			buf);
	    }

	    if (!(print_flag & PRINT_REG)) {
		if (print_flag & PRINT_SH) {
		    fprintf(stdout, "rows=%d\n", window->rows);
		    fprintf(stdout, "cols=%d\n", window->cols);
		}
		else {
		    fprintf(stdout, "%-*s %d\n", width, "rows:",
			    window->rows);
		    fprintf(stdout, "%-*s %d\n", width, "cols:",
			    window->cols);
		}
	    }
	}
	else {			/* in lat/long already */

	    if (window->proj != 0)
		G_message(_("You are already in Lat/Long. Use the -p flag instead."));
	    else
		G_message(_("You are in a simple XY location, projection to Lat/Lon "
			   "is not possible. Use the -p flag instead."));
	}
    }

    /* flag.eprint */
    if (print_flag & PRINT_EXTENT) {
	if (print_flag & PRINT_SH) {
	    fprintf(stdout, "ns_extent=%f\n", window->north - window->south);
	    fprintf(stdout, "ew_extent=%f\n", window->east - window->west);
	}
	else {
	    if (G_projection() != PROJECTION_LL) {
		fprintf(stdout, "%-*s %f\n", width, "north-south extent:",
			window->north - window->south);
		fprintf(stdout, "%-*s %f\n", width, "east-west extent:",
			window->east - window->west);
	    }
	    else {
		G_format_northing(window->north - window->south, buf,
				  PROJECTION_LL);
		fprintf(stdout, "%-*s %s\n", width, "north-south extent:",
			buf);
		G_format_easting(window->east - window->west, buf,
				 PROJECTION_LL);
		fprintf(stdout, "%-*s %s\n", width, "east-west extent:", buf);
	    }
	}
    }

    /* flag.center */
    if (print_flag & PRINT_CENTER) {
	if (print_flag & PRINT_SH) {
	    fprintf(stdout, "center_easting=%f\n",
		    ((window->west - window->east) / 2. + window->east));
	    fprintf(stdout, "center_northing=%f\n",
		    ((window->north - window->south) / 2. + window->south));
	}
	else {
	    if (G_projection() != PROJECTION_LL) {
		fprintf(stdout, "%-*s %f\n", width, "center easting:",
			((window->west - window->east) / 2. + window->east));
		fprintf(stdout, "%-*s %f\n", width, "center northing:",
			((window->north - window->south) / 2. +
			 window->south));
	    }
	    else {
		G_format_northing((window->north - window->south) / 2. +
				  window->south, buf, PROJECTION_LL);
		fprintf(stdout, "%-*s %s\n", width, "north-south center:",
			buf);
		G_format_easting((window->west - window->east) / 2. +
				 window->east, buf, PROJECTION_LL);
		fprintf(stdout, "%-*s %s\n", width, "east-west center:", buf);
	    }
	}
    }


    /* flag.gmt_style */
    if (print_flag & PRINT_GMT)
	fprintf(stdout, "%s/%s/%s/%s\n", west, east, south, north);

    /* flag.wms_style */
    if (print_flag & PRINT_WMS) {
	G_format_northing(window->north, north, -1);
	G_format_northing(window->south, south, -1);
	G_format_easting(window->east, east, -1);
	G_format_easting(window->west, west, -1);
	fprintf(stdout, "bbox=%s,%s,%s,%s\n", west, south, east, north);
    }


    /* flag.nangle */
    if (print_flag & PRINT_NANGLE) {
	double convergence;

	if (G_projection() == PROJECTION_XY)
	    convergence = 0./0.;
	else if (G_projection() == PROJECTION_LL)
	    convergence = 0.0;
	else {
	    struct Key_Value *in_proj_info, *in_unit_info;
	    double lo1, la1, lo2, la2, lo3, la3, lo4, la4;
	    double mid_n_lo, mid_n_la, mid_s_lo, mid_s_la;
	    double lat_center, lon_center;
	    struct pj_info iproj, oproj;	/* proj parameters  */
	    struct FACTORS fact;

	    /* read current projection info */
	    if ((in_proj_info = G_get_projinfo()) == NULL)
		G_fatal_error(_("Can't get projection info of current location"));

	    if ((in_unit_info = G_get_projunits()) == NULL)
		G_fatal_error(_("Can't get projection units of current location"));

	    if (pj_get_kv(&iproj, in_proj_info, in_unit_info) < 0)
		G_fatal_error(_("Can't get projection key values of current location"));

	    /*  output projection to lat/long w/ same ellipsoid as input */
	    oproj.zone = 0;
	    oproj.meters = 1.;
	    sprintf(oproj.proj, "ll");
	    if ((oproj.pj = pj_latlong_from_proj(iproj.pj)) == NULL)
		G_fatal_error(_("Unable to update lat/long projection parameters"));

	    /* for DEBUG
	       pj_print_proj_params(&iproj, &oproj);
	     */

	    /* do the transform
	     * syntax: pj_do_proj(outx, outy, in_info, out_info) 
	     *
	     *  1 ------ 2
	     *  |        |  map corners
	     *  |        |
	     *  4--------3
	     */

	    latitude = window->north;
	    longitude = window->west;
	    if (pj_do_proj(&longitude, &latitude, &iproj, &oproj) < 0)
		G_fatal_error(_("Error in pj_do_proj (projection of input coordinate pair)"));

	    lo1 = longitude;
	    la1 = latitude;

	    latitude = window->north;
	    longitude = window->east;
	    if (pj_do_proj(&longitude, &latitude, &iproj, &oproj) < 0)
		G_fatal_error(_("Error in pj_do_proj (projection of input coordinate pair)"));

	    lo2 = longitude;
	    la2 = latitude;

	    latitude = window->south;
	    longitude = window->east;
	    if (pj_do_proj(&longitude, &latitude, &iproj, &oproj) < 0)
		G_fatal_error(_("Error in pj_do_proj (projection of input coordinate pair)"));

	    lo3 = longitude;
	    la3 = latitude;

	    latitude = window->south;
	    longitude = window->west;
	    if (pj_do_proj(&longitude, &latitude, &iproj, &oproj) < 0)
		G_fatal_error(_("Error in pj_do_proj (projection of input coordinate pair)"));

	    lo4 = longitude;
	    la4 = latitude;

	    /* 
	     * map corners and side mids:
	     *          mid_n
	     *       1 ---+---2
	     *       |        |
	     * mid_w +        + mid_e 
	     *       |        |
	     *       4----+---3
	     *          mid_s
	     *
	     * lo: longitude
	     * la: latitude
	     */

	    /* side mids for easting, northing center: */
	    mid_n_lo = (lo2 + lo1) / 2.;
	    mid_n_la = (la2 + la1) / 2.;
	    mid_s_lo = (lo3 + lo4) / 2.;
	    mid_s_la = (la3 + la4) / 2.;

	    lat_center = (mid_n_la + mid_s_la) / 2.;
	    lon_center = (mid_n_lo + mid_s_lo) / 2.;

	    LP lp = { DEG2RAD(lon_center), DEG2RAD(lat_center) };
	    pj_factors(lp, iproj.pj, 0.0, &fact);
	    convergence = RAD2DEG(fact.conv);

	    G_free_key_value(in_proj_info);
	    G_free_key_value(in_unit_info);
	}

	if (print_flag & PRINT_SH)
	    fprintf(stdout, "converge_angle=%f\n", convergence);
	else
	    fprintf(stdout, "%-*s %f\n", width, "convergence angle:",
		    convergence);
    }

    /* flag.bbox
       Calculate the largest boudingbox in lat-lon coordinates 
       and print it to stdout
     */
    if (print_flag & PRINT_MBBOX) {
	double lo1, la1, lo2, la2, lo3, la3, lo4, la4;
	double sh_ll_w, sh_ll_e, sh_ll_n, sh_ll_s;

	/*double sh_ll_rows, sh_ll_cols; */

	/* Needed to calculate the LL bounding box */
	if ((G_projection() != PROJECTION_XY)) {
	    /* projection information of input and output map */
	    struct Key_Value *in_proj_info, *in_unit_info, *out_proj_info,
		*out_unit_info;
	    struct pj_info iproj;	/* input map proj parameters  */
	    struct pj_info oproj;	/* output map proj parameters  */
	    char buff[100], dum[100];

	    /* read current projection info */
	    if ((in_proj_info = G_get_projinfo()) == NULL)
		G_fatal_error(_("Can't get projection info of current location"));

	    if ((in_unit_info = G_get_projunits()) == NULL)
		G_fatal_error(_("Can't get projection units of current location"));

	    if (pj_get_kv(&iproj, in_proj_info, in_unit_info) < 0)
		G_fatal_error(_("Can't get projection key values of current location"));

	    /*  output projection to lat/long  and wgs84 ellipsoid */
	    out_proj_info = G_create_key_value();
	    out_unit_info = G_create_key_value();

	    G_set_key_value("proj", "ll", out_proj_info);

	    if (G_get_datumparams_from_projinfo(in_proj_info, buff, dum) < 0)
		G_fatal_error(_("WGS84 output not possible as this location does not contain "
			       "datum transformation parameters. Try running g.setproj."));
	    else
		G_set_key_value("datum", "wgs84", out_proj_info);

	    G_set_key_value("unit", "degree", out_unit_info);
	    G_set_key_value("units", "degrees", out_unit_info);
	    G_set_key_value("meters", "1.0", out_unit_info);

	    if (pj_get_kv(&oproj, out_proj_info, out_unit_info) < 0)
		G_fatal_error(_("Unable to update lat/long projection parameters"));

	    G_free_key_value(in_proj_info);
	    G_free_key_value(in_unit_info);
	    G_free_key_value(out_proj_info);
	    G_free_key_value(out_unit_info);


	    /*  output projection to lat/long w/ same ellipsoid as input */
	    /*
	       oproj.zone = 0;
	       oproj.meters = 1.;
	       sprintf(oproj.proj, "ll");
	       if ((oproj.pj = pj_latlong_from_proj(iproj.pj)) == NULL)
	       G_fatal_error(_("Unable to  up lat/long projection parameters"));            
	     */

	    /* do the transform
	     * syntax: pj_do_proj(outx, outy, in_info, out_info) 
	     *
	     *  1 ------ 2
	     *  |        |  map corners
	     *  |        |
	     *  4--------3
	     */

	    latitude = window->north;
	    longitude = window->west;
	    if (pj_do_proj(&longitude, &latitude, &iproj, &oproj) < 0)
		G_fatal_error(_("Error in pj_do_proj (projection of input coordinate pair)"));

	    lo1 = longitude;
	    la1 = latitude;

	    latitude = window->north;
	    longitude = window->east;
	    if (pj_do_proj(&longitude, &latitude, &iproj, &oproj) < 0)
		G_fatal_error(_("Error in pj_do_proj (projection of input coordinate pair)"));

	    lo2 = longitude;
	    la2 = latitude;

	    latitude = window->south;
	    longitude = window->east;
	    if (pj_do_proj(&longitude, &latitude, &iproj, &oproj) < 0)
		G_fatal_error(_("Error in pj_do_proj (projection of input coordinate pair)"));

	    lo3 = longitude;
	    la3 = latitude;

	    latitude = window->south;
	    longitude = window->west;
	    if (pj_do_proj(&longitude, &latitude, &iproj, &oproj) < 0)
		G_fatal_error(_("Error in pj_do_proj (projection of input coordinate pair)"));

	    lo4 = longitude;
	    la4 = latitude;

	    /*Calculate the largest bounding box */
	    if (fabs(lo3) > fabs(lo4)) {
		if (fabs(lo4) < fabs(lo1))
		    sh_ll_w = lo4;
		else
		    sh_ll_w = lo1;
		if (fabs(lo3) > fabs(lo2))
		    sh_ll_e = lo3;
		else
		    sh_ll_e = lo2;
	    }
	    else {
		if (fabs(lo4) > fabs(lo1))
		    sh_ll_w = lo4;
		else
		    sh_ll_w = lo1;
		if (fabs(lo3) < fabs(lo2))
		    sh_ll_e = lo3;
		else
		    sh_ll_e = lo2;
	    }

	    if (fabs(la4) < fabs(la1)) {
		if (fabs(la1) > fabs(la2))
		    sh_ll_n = la1;
		else
		    sh_ll_n = la2;
		if (fabs(la4) < fabs(la3))
		    sh_ll_s = la4;
		else
		    sh_ll_s = la3;
	    }
	    else {
		if (fabs(la1) < fabs(la2))
		    sh_ll_n = la1;
		else
		    sh_ll_n = la2;
		if (fabs(la4) > fabs(la3))
		    sh_ll_s = la4;
		else
		    sh_ll_s = la3;
	    }

	    /* print the larg bounding box */
	    if (print_flag & PRINT_SH) {
		fprintf(stdout, "ll_n=%.8f\n", sh_ll_n);
		fprintf(stdout, "ll_s=%.8f\n", sh_ll_s);
		fprintf(stdout, "ll_w=%.8f\n", sh_ll_w);
		fprintf(stdout, "ll_e=%.8f\n", sh_ll_e);
		fprintf(stdout, "ll_clon=%.8f\n",
			(sh_ll_e - sh_ll_w) / 2. + sh_ll_w);
		fprintf(stdout, "ll_clat=%.8f\n",
			(sh_ll_n - sh_ll_s) / 2. + sh_ll_s);
	    }
	    else {
		G_format_northing(sh_ll_n, buf, PROJECTION_LL);
		fprintf(stdout, "%-*s  %s\n", width, "north latitude:", buf);
		G_format_northing(sh_ll_s, buf, PROJECTION_LL);
		fprintf(stdout, "%-*s  %s\n", width, "south latitude:", buf);
		G_format_easting(sh_ll_w, buf, PROJECTION_LL);
		fprintf(stdout, "%-*s  %s\n", width, "west longitude:", buf);
		G_format_easting(sh_ll_e, buf, PROJECTION_LL);
		fprintf(stdout, "%-*s  %s\n", width, "east longitude:", buf);
		G_format_easting((sh_ll_e - sh_ll_w) / 2. + sh_ll_w, buf,
				 PROJECTION_LL);
		fprintf(stdout, "%-*s %s\n", width, "center longitude:", buf);
		G_format_northing((sh_ll_n - sh_ll_s) / 2. + sh_ll_s, buf,
				  PROJECTION_LL);
		fprintf(stdout, "%-*s  %s\n", width, "center latitude:", buf);
	    }

	    /*It should be calculated which number of rows and cols we have in LL */
	    /*
	       fprintf (stdout, "LL_ROWS=%f \n",sh_ll_rows);
	       fprintf (stdout, "LL_COLS=%f \n",sh_ll_cols);
	     */
	}
	else {
	    G_warning(_("Lat/Long calculations are not possible from a simple XY system"));
	}
    }
}
