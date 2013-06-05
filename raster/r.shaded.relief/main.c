
/****************************************************************************
 *
 * MODULE:       r.shaded.relief
 * AUTHOR(S):	CERL
 *               parameters standardized: Markus Neteler, 2008
 *               updates: Michael Barton, 2004
 *               updates: Gordon Keith, 2003
 *               updates: Andreas Lange, 2001
 *               updates: David Finlayson, 2001
 *               updates: Markus Neteler, 2001, 1999
 *               Converted to Python by Glynn Clements
 *               Converted to C by Markus Metz
 * PURPOSE:	Creates shaded relief map from raster elevation map (DEM)
 * COPYRIGHT:	(C) 1999 - 2008, 2010, 2012 by the GRASS Development Team
 *
 *		This program is free software under the GNU General Public
 *		License (>=v2). Read the file COPYING that comes with GRASS
 *		for details.
 *
 *****************************************************************************/

/*
 *   July 2007 - allow input from other mapsets (Brad Douglas)
 *
 *   May 2005 - fixed wrong units parameter (Markus Neteler)
 *
 *   September 2004 - Added z exaggeration control (Michael Barton) 
 *   April 2004 - updated for GRASS 5.7 by Michael Barton 
 *
 *   9/2004 Adds scale factor input (as per documentation); units set scale only if specified for lat/long regions
 *    Also, adds option of controlling z-exaggeration.
 *
 *   6/2003 fixes for Lat/Long Gordon Keith <gordon.keith@csiro.au>
 *   If n is a number then the ewres and nsres are mulitplied by that scale
 *    to calculate the shading.
 *   If n is the letter M (either case) the number of metres is degree of
 *    latitude is used as the scale.
 *   If n is the letter f then the number of feet in a degree is used.
 *   It scales latitude and longitude equally, so it's only approximately
 *   right, but for shading its close enough. It makes the difference
 *   between an unusable and usable shade.
 *
 *   10/2001 fix for testing for dashes in raster file name
 *        by Andreas Lange <andreas.lange@rhein-main.de>
 *   10/2001 added parser support - Markus Neteler
 *   9/2001 fix to keep NULLs as is (was value 22 before) - Markus Neteler
 *   1/2001 fix for NULL by David Finlayson <david_finlayson@yahoo.com>
 *   11/99 updated $ewres to ewres() and $nsres to nsres()
 *       updated number to FP in r.mapcalc statement Markus Neteler
 */


#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

int main(int argc, char *argv[])
{
    int in_fd;
    int out_fd;
    DCELL *elev_cell[3], *temp;
    DCELL *out_rast, *out_ptr = NULL;
    DCELL *c1, *c2, *c3, *c4, *c5, *c6, *c7, *c8, *c9;
    int Wrap;			/* global wraparound */
    struct Cell_head window;
    struct History hist;
    struct Colors colors;

    const char *elev_name;
    const char *sr_name;
    int out_type = CELL_TYPE;
    size_t out_size;
    const char *units;
    char buf[GNAME_MAX];
    int nrows, row;
    int ncols, col;

    double zmult, scale, altitude, azimuth;
    double north, east, south, west, ns_med;

    double degrees_to_radians, radians_to_degrees;
    double H, V;
    double dx;			/* partial derivative in ew direction */
    double dy;			/* partial derivative in ns direction */
    double key;
    double slp_in_rad, aspect, cang;

    struct FPRange range;
    DCELL min, max;

    struct GModule *module;
    struct
    {
	struct Option *elevation, *relief, *altitude, *azimuth, *zmult,
	    *scale, *units;
    } parm;
    char *desc;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("elevation"));
    G_add_keyword(_("terrain"));
    module->label = _("Creates shaded relief map from an elevation map (DEM).");
    
    parm.elevation = G_define_standard_option(G_OPT_R_INPUT);

    parm.relief = G_define_standard_option(G_OPT_R_OUTPUT);
    parm.relief->required = NO;
    parm.relief->label = _("Name for output shaded relief map");
    parm.relief->description = _("Default: <input_map>.shade");

    parm.altitude = G_define_option();
    parm.altitude->key = "altitude";
    parm.altitude->type = TYPE_DOUBLE;
    parm.altitude->required = NO;
    parm.altitude->answer = "30";
    parm.altitude->options = "0-90";
    parm.altitude->description = _("Altitude of the sun in degrees above the horizon");

    parm.azimuth = G_define_option();
    parm.azimuth->key = "azimuth";
    parm.azimuth->type = TYPE_DOUBLE;
    parm.azimuth->required = NO;
    parm.azimuth->answer = "270";
    parm.azimuth->options = "0-360";
    parm.azimuth->description =
	_("Azimuth of the sun in degrees to the east of north");

    parm.zmult = G_define_option();
    parm.zmult->key = "zmult";
    parm.zmult->type = TYPE_DOUBLE;
    parm.zmult->required = NO;
    parm.zmult->answer = "1";
    parm.zmult->description = _("Factor for exaggerating relief");

    parm.scale = G_define_option();
    parm.scale->key = "scale";
    parm.scale->type = TYPE_DOUBLE;
    parm.scale->required = NO;
    parm.scale->answer = "1";
    parm.scale->description =
	_("Scale factor for converting meters to elevation units");

    parm.units = G_define_option();
    parm.units->key = "units";
    parm.units->type = TYPE_STRING;
    parm.units->required = NO;
    parm.units->options = "intl,survey";
    parm.units->description = _("Elevation units (overrides scale factor)");
    desc = NULL;
    G_asprintf(&desc,
	       "intl;%s;"
	       "survey;%s",
	       _("international feet"),
	       _("survey feet"));
    parm.units->descriptions = desc;


    degrees_to_radians = M_PI / 180.0;
    radians_to_degrees = 180. / M_PI;

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    elev_name = parm.elevation->answer;
    if (parm.relief->answer) {
	sr_name = parm.relief->answer;
    }
    else {
	char xname[GNAME_MAX], xmapset[GNAME_MAX];
	
	if (G_name_is_fully_qualified(elev_name, xname, xmapset))
	    sprintf(buf, "%s.shade", xname);
	else
	    sprintf(buf, "%s.shade", elev_name);
	sr_name = G_store(buf);
    }

    G_check_input_output_name(elev_name, sr_name, G_FATAL_EXIT);

    if (sscanf(parm.altitude->answer, "%lf", &altitude) != 1 || altitude < 0.0) {
	G_fatal_error(_("%s=%s - must be a non-negative number"),
		      parm.altitude->key, parm.altitude->answer);
    }
    altitude *= degrees_to_radians;

    if (sscanf(parm.azimuth->answer, "%lf", &azimuth) != 1 || azimuth < 0.0) {
	G_fatal_error(_("%s=%s - must be a non-negative number"),
		      parm.azimuth->key, parm.azimuth->answer);
    }
    /* correct azimuth to East (GRASS convention):
     * this seems to be backwards, but in fact it works so leave it. */
    azimuth = (azimuth - 90.) * degrees_to_radians;

    if (sscanf(parm.zmult->answer, "%lf", &zmult) != 1 || zmult == 0.0) {
	G_fatal_error(_("%s=%s - must not be zero"),
		      parm.zmult->key, parm.zmult->answer);
    }

    if (sscanf(parm.scale->answer, "%lf", &scale) != 1 || scale <= 0.0) {
	G_fatal_error(_("%s=%s - must be a positive number"),
		      parm.scale->key,
		      parm.scale->answer);
    }

    G_get_set_window(&window);

    nrows = Rast_window_rows();
    ncols = Rast_window_cols();

    /* horizontal distances are calculated in meters by G_distance() */
    if (parm.units->answer) {
	units = parm.units->answer;
	if (strcmp(units, "intl") == 0) {
	    /* 1 international foot = 0.3048 meters */
	    scale = 1. / 0.3048;

	}
	else if (strcmp(units, "survey") == 0) {
	    /* 1 survey foot = 1200 / 3937 meters */
	    scale = 3937. / 1200.;
	}
    }

    Wrap = 0;
    if (G_projection() == PROJECTION_LL) {
	if ((window.west == (window.east - 360.))
	     || (window.east == (window.west - 360.))) {
	    Wrap = 1;
	    ncols += 2;
	}
    }

    /* H = window.ew_res * 4 * 2/ zmult; *//* horizontal (east-west) run 
       times 4 for weighted difference */
    /* V = window.ns_res * 4 * 2/ zmult; *//* vertical (north-south) run 
       times 4 for weighted difference */


    G_begin_distance_calculations();
    north = Rast_row_to_northing(0.5, &window);
    ns_med = Rast_row_to_northing(1.5, &window);
    south = Rast_row_to_northing(2.5, &window);
    east = Rast_col_to_easting(2.5, &window);
    west = Rast_col_to_easting(0.5, &window);
    V = G_distance(east, north, east, south) * 4 * scale / zmult;
    H = G_distance(east, ns_med, west, ns_med) * 4 * scale / zmult;
    /*    ____________________________
       |c1      |c2      |c3      |
       |        |        |        |
       |        |  north |        |        
       |        |        |        |
       |________|________|________|          
       |c4      |c5      |c6      |
       |        |        |        |
       |  east  | ns_med |  west  |
       |        |        |        |
       |________|________|________|
       |c7      |c8      |c9      |
       |        |        |        |
       |        |  south |        |
       |        |        |        |
       |________|________|________|
     */

    /* open the elevation file for reading */
    in_fd = Rast_open_old(elev_name, "");
    elev_cell[0] = (DCELL *) G_calloc(ncols + 1, sizeof(DCELL));
    Rast_set_d_null_value(elev_cell[0], ncols);
    elev_cell[1] = (DCELL *) G_calloc(ncols, sizeof(DCELL));
    Rast_set_d_null_value(elev_cell[1], ncols);
    elev_cell[2] = (DCELL *) G_calloc(ncols, sizeof(DCELL));
    Rast_set_d_null_value(elev_cell[2], ncols);

    out_fd = Rast_open_new(sr_name, out_type);
    out_rast = Rast_allocate_buf(out_type);
    Rast_set_null_value(out_rast, Rast_window_cols(), out_type);
    Rast_put_row(out_fd, out_rast, out_type);
    out_size = Rast_cell_size(out_type);

    if (Wrap) {
	Rast_get_d_row_nomask(in_fd, elev_cell[1] + 1, 0);
	elev_cell[1][0] = elev_cell[1][Rast_window_cols() - 1];
	elev_cell[1][Rast_window_cols() + 1] = elev_cell[1][2];
    }
    else
	Rast_get_d_row_nomask(in_fd, elev_cell[1], 0);

    if (Wrap) {
	Rast_get_d_row_nomask(in_fd, elev_cell[2] + 1, 1);
	elev_cell[2][0] = elev_cell[2][Rast_window_cols() - 1];
	elev_cell[2][Rast_window_cols() + 1] = elev_cell[2][2];
    }
    else
	Rast_get_d_row_nomask(in_fd, elev_cell[2], 1);

    G_verbose_message(_("Percent complete..."));

    for (row = 2; row < nrows; row++) {
	/*  if projection is Lat/Lon, recalculate  V and H   */
	if (G_projection() == PROJECTION_LL) {
	    north = Rast_row_to_northing((row - 2 + 0.5), &window);
	    ns_med = Rast_row_to_northing((row - 1 + 0.5), &window);
	    south = Rast_row_to_northing((row + 0.5), &window);
	    east = Rast_col_to_easting(2.5, &window);
	    west = Rast_col_to_easting(0.5, &window);
	    V = G_distance(east, north, east, south) * 4 * scale / zmult;
	    H = G_distance(east, ns_med, west, ns_med) * 4 * scale / zmult;
	    /*        ____________________________
	       |c1      |c2      |c3      |
	       |        |        |        |
	       |        |  north |        |        
	       |        |        |        |
	       |________|________|________|          
	       |c4      |c5      |c6      |
	       |        |        |        |
	       |  east  | ns_med |  west  |
	       |        |        |        |
	       |________|________|________|
	       |c7      |c8      |c9      |
	       |        |        |        |
	       |        |  south |        |
	       |        |        |        |
	       |________|________|________|
	     */
	}

	G_percent(row, nrows, 2);
	temp = elev_cell[0];
	elev_cell[0] = elev_cell[1];
	elev_cell[1] = elev_cell[2];
	elev_cell[2] = temp;

	if (Wrap) {
	    Rast_get_d_row_nomask(in_fd, elev_cell[2] + 1, row);
	    elev_cell[2][0] = elev_cell[2][Rast_window_cols() - 1];
	    elev_cell[2][Rast_window_cols() + 1] = elev_cell[2][2];
	}
	else
	    Rast_get_d_row_nomask(in_fd, elev_cell[2], row);

	c1 = elev_cell[0];
	c2 = c1 + 1;
	c3 = c1 + 2;
	c4 = elev_cell[1];
	c5 = c4 + 1;
	c6 = c4 + 2;
	c7 = elev_cell[2];
	c8 = c7 + 1;
	c9 = c7 + 2;

	if (Wrap)
	    out_ptr = out_rast;
	else
	    out_ptr = G_incr_void_ptr(out_rast, out_size);

	/*skip first cell of the row */

	for (col = ncols - 2; col-- > 0;
	     c1++, c2++, c3++, c4++, c5++, c6++, c7++, c8++, c9++) {
	    /*  DEBUG:
	       fprintf(stdout, "\n%.0f %.0f %.0f\n%.0f %.0f %.0f\n%.0f %.0f %.0f\n",
	       *c1, *c2, *c3, *c4, *c5, *c6, *c7, *c8, *c9);
	     */

	    if (Rast_is_d_null_value(c1) || Rast_is_d_null_value(c2) ||
		Rast_is_d_null_value(c3) || Rast_is_d_null_value(c4) ||
		Rast_is_d_null_value(c5) || Rast_is_d_null_value(c6) ||
		Rast_is_d_null_value(c7) || Rast_is_d_null_value(c8) ||
		Rast_is_d_null_value(c9)) {

		Rast_set_null_value(out_ptr, 1, out_type);
		out_ptr = G_incr_void_ptr(out_ptr, out_size);

		continue;
	    }			/* no data */

	    /* shaded relief */
	    /* slope */
	    dx = (*c1 + 2 * *c4 + *c7 - *c3 - 2 * *c6 - *c9) / H;
	    dy = (*c1 + 2 * *c2 + *c3 - *c7 - 2 * *c8 - *c9) / V;

	    key = dx * dx + dy * dy;

	    slp_in_rad = M_PI / 2. - atan(sqrt(key));

	    /* aspect */
	    aspect = atan2(dy, dx);

	    if (aspect != aspect)
		aspect = degrees_to_radians;
	    if (dx != 0 || dy != 0) {
		if (aspect == 0)
		    aspect = 2 * M_PI;
	    }

#if 0
	    /* the original script was rounding aspect. Why? */
	    aspect *= radians_to_degrees;
	    if (aspect < 0)
		aspect = (int)(aspect - 0.5);
	    else
		aspect = (int)(aspect + 0.5);
	    aspect *= degrees_to_radians;
#endif

	    /* shaded relief */
	    cang = sin(altitude) * sin(slp_in_rad) + 
	           cos(altitude) * cos(slp_in_rad) * cos(azimuth - aspect);

	    Rast_set_d_value(out_ptr, (DCELL) 255 * cang, out_type);

	    out_ptr = G_incr_void_ptr(out_ptr, out_size);

	}			/* column for loop */

	Rast_put_row(out_fd, out_rast, out_type);

    }				/* row loop */

    G_percent(row, nrows, 2);

    Rast_close(in_fd);

    Rast_set_null_value(out_rast, Rast_window_cols(), out_type);
    Rast_put_row(out_fd, out_rast, out_type);
    Rast_close(out_fd);

    G_debug(1, "Creating support files...");

    /* write colors for shaded relief */
    Rast_init_colors(&colors);
    Rast_read_fp_range(sr_name, G_mapset(), &range);
    Rast_get_fp_range_min_max(&range, &min, &max);
    min -= 0.01;
    max += 0.01;
    Rast_make_grey_scale_fp_colors(&colors, min, max);
    Rast_write_colors(sr_name, G_mapset(), &colors);
    
    sprintf(buf, "Shaded relief of \"%s\"", elev_name);
    Rast_put_cell_title(sr_name, buf);

    /* writing history file */
    Rast_short_history(sr_name, "raster", &hist);
    Rast_append_format_history(&hist, "r.shaded.relief settings:");
    Rast_append_format_history(&hist,
                               "altitude=%f  azimuth=%f zmult=%f  scale=%f",
			       altitude * radians_to_degrees,
			       azimuth * radians_to_degrees,
			       zmult, scale);
    Rast_format_history(&hist, HIST_DATSRC_1, "raster elevation file %s", elev_name);
    Rast_command_history(&hist);
    Rast_write_history(sr_name, &hist);

    G_message(_("Shaded relief raster map <%s> complete"), sr_name);

    exit(EXIT_SUCCESS);
}
