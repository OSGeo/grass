
/****************************************************************************
 *
 * MODULE:       r.sunhours
 * AUTHOR(S):    Markus Metz
 * PURPOSE:      Calculates solar azimuth and angle, and 
 *               sunshine hours (also called daytime period)
 * COPYRIGHT:    (C) 2010-2013 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

/* TODO: always use solpos if time is Greenwich standard time */

#define MAIN

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/gprojects.h>
#include <grass/glocale.h>
#include "solpos00.h"

void set_solpos_time(struct posdata *, int, int, int, int, int, int);
void set_solpos_longitude(struct posdata *, double );
int roundoff(double *);

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct
    {
	struct Option *elev, *azimuth, *sunhours, *year,
	    *month, *day, *hour, *minutes, *seconds;
	struct Flag *lst_time, *no_solpos;
    } parm;
    struct Cell_head window;
    FCELL *elevbuf, *azimuthbuf, *sunhourbuf;
    struct History hist;

    /* projection information of input map */
    struct Key_Value *in_proj_info, *in_unit_info;
    struct pj_info iproj;	/* input map proj parameters  */
    struct pj_info oproj;	/* output map proj parameters  */

    char *elev_name, *azimuth_name, *sunhour_name;
    int elev_fd, azimuth_fd, sunhour_fd;
    double ha, ha_cos, s_gamma, s_elevation, s_azimuth;
    double s_declination, sd_sin, sd_cos;
    double se_sin, sa_cos;
    double east, east_ll, north, north_ll;
    double north_gc, north_gc_sin, north_gc_cos;  /* geocentric latitude */
    double ba2;
    int year, month, day, hour, minutes, seconds;
    int doy;   					/* day of year */
    int row, col, nrows, ncols;
    int do_reproj = 0;
    int lst_time = 1;
    int use_solpos = 0;
    struct posdata pd;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("solar"));
    module->label = _("Calculates solar elevation, solar azimuth, and sun hours.");
    module->description = _("Solar elevation: the angle between the direction of the geometric center "
			    "of the sun's apparent disk and the (idealized) horizon. "
			    "Solar azimuth: the angle from due north in clockwise direction.");
    
    parm.elev = G_define_standard_option(G_OPT_R_OUTPUT);
    parm.elev->key = "elevation";
    parm.elev->label = _("Output raster map with solar elevation angle");
    parm.elev->required = NO;

    parm.azimuth = G_define_standard_option(G_OPT_R_OUTPUT);
    parm.azimuth->key = "azimuth";
    parm.azimuth->label = _("Output raster map with solar azimuth angle");
    parm.azimuth->required = NO;
    
    parm.sunhours = G_define_standard_option(G_OPT_R_OUTPUT);
    parm.sunhours->key = "sunhour";
    parm.sunhours->label = _("Output raster map with sunshine hours");
    parm.sunhours->description = _("Sunshine hours require solpos and Greenwich standard time");
    parm.sunhours->required = NO;

    parm.year = G_define_option();
    parm.year->key = "year";
    parm.year->type = TYPE_INTEGER;
    parm.year->required = YES;
    parm.year->description = _("Year");
    parm.year->options = "1950-2050";
    parm.year->guisection = _("Time");

    parm.month = G_define_option();
    parm.month->key = "month";
    parm.month->type = TYPE_INTEGER;
    parm.month->required = NO;
    parm.month->label = _("Month");
    parm.month->description = _("If not given, day is interpreted as day of the year");
    parm.month->options = "1-12";
    parm.month->guisection = _("Time");

    parm.day = G_define_option();
    parm.day->key = "day";
    parm.day->type = TYPE_INTEGER;
    parm.day->required = YES;
    parm.day->description = _("Day");
    parm.day->options = "1-366";
    parm.day->guisection = _("Time");

    parm.hour = G_define_option();
    parm.hour->key = "hour";
    parm.hour->type = TYPE_INTEGER;
    parm.hour->required = NO;
    parm.hour->description = _("Hour");
    parm.hour->options = "0-24";
    parm.hour->answer = "12";
    parm.hour->guisection = _("Time");

    parm.minutes = G_define_option();
    parm.minutes->key = "minute";
    parm.minutes->type = TYPE_INTEGER;
    parm.minutes->required = NO;
    parm.minutes->description = _("Minutes");
    parm.minutes->options = "0-60";
    parm.minutes->answer = "0";
    parm.minutes->guisection = _("Time");

    parm.seconds = G_define_option();
    parm.seconds->key = "second";
    parm.seconds->type = TYPE_INTEGER;
    parm.seconds->required = NO;
    parm.seconds->description = _("Seconds");
    parm.seconds->options = "0-60";
    parm.seconds->answer = "0";
    parm.seconds->guisection = _("Time");

    parm.lst_time = G_define_flag();
    parm.lst_time->key = 't';
    parm.lst_time->description = _("Time is local sidereal time, not Greenwich standard time");

    parm.no_solpos = G_define_flag();
    parm.no_solpos->key = 's';
    parm.no_solpos->description = _("Do not use solpos algorithm of NREL");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    G_get_window(&window);

    /* require at least one output */
    elev_name = parm.elev->answer;
    azimuth_name = parm.azimuth->answer;
    sunhour_name = parm.sunhours->answer;
    if (!elev_name && !azimuth_name && !sunhour_name)
	G_fatal_error(_("No output requested, exiting."));

    year = atoi(parm.year->answer);
    if (parm.month->answer)
	month = atoi(parm.month->answer);
    else
	month = -1;

    day = atoi(parm.day->answer);
    hour = atoi(parm.hour->answer);
    minutes = atoi(parm.minutes->answer);
    seconds = atoi(parm.seconds->answer);

    lst_time = (parm.lst_time->answer != 0);
    use_solpos = (parm.no_solpos->answer == 0);

    /* init variables */
    ha = 180;
    ha_cos = 0;
    sd_cos = 0;
    sd_sin = 1;
    north_gc_cos = 0;
    north_gc_sin = 1;
    se_sin = 0;

    if (use_solpos && lst_time) {
	G_warning(_("NREL solpos algorithm uses Greenwich standard time."));
	G_warning(_("Time will be interpreted as Greenwich standard time."));

	lst_time = 0;
    }
    if (!use_solpos) {
	if (lst_time)
	    G_message(_("Time will be interpreted as local sidereal time."));
	else
	    G_message(_("Time will be interpreted as Greenwich standard time."));
	
	if (sunhour_name)
	    G_fatal_error(_("Sunshine hours require NREL solpos."));
    }

    if ((G_projection() != PROJECTION_LL)) {
	if (window.proj == 0)
	    G_fatal_error(_("Current projection is x,y (undefined)."));

	do_reproj = 1;

	/* read current projection info */
	if ((in_proj_info = G_get_projinfo()) == NULL)
	    G_fatal_error(_("Cannot get projection info of current location"));

	if ((in_unit_info = G_get_projunits()) == NULL)
	    G_fatal_error(_("Cannot get projection units of current location"));

	if (pj_get_kv(&iproj, in_proj_info, in_unit_info) < 0)
	    G_fatal_error(_("Cannot get projection key values of current location"));

	G_free_key_value(in_proj_info);
	G_free_key_value(in_unit_info);

	/*  output projection to lat/long w/ same ellipsoid as input */
	oproj.zone = 0;
	oproj.meters = 1.;
	sprintf(oproj.proj, "ll");
	if ((oproj.pj = pj_latlong_from_proj(iproj.pj)) == NULL)
	    G_fatal_error(_("Unable to update lat/long projection parameters"));
    }

    /* always init pd */
    S_init(&pd);
    pd.function = S_GEOM;
    if (use_solpos) {
	pd.function = S_ZENETR;
	if (azimuth_name)
	    pd.function = S_SOLAZM;
	if (sunhour_name)
	    pd.function |= S_SRSS;
    }
    if (month == -1)
	doy = day;
    else
	doy = dom2doy2(year, month, day);
    
    set_solpos_time(&pd, year, 1, doy, hour, minutes, seconds);
    set_solpos_longitude(&pd, 0);
    pd.latitude = 0;
    S_solpos(&pd);

    if (lst_time) {
	/* hour angle */
	/***************************************************************
	 * The hour angle of a point on the Earth's surface is the angle
	 * through which the earth would turn to bring the meridian of
	 * the point directly under the sun. This angular displacement
	 * represents time (1 hour = 15 degrees).
	 * The hour angle is negative in the morning, zero at 12:00,
	 * and positive in the afternoon
	 ***************************************************************/

	ha = 15.0 * (hour + minutes / 60.0 + seconds / 3600.0) - 180.;
	G_debug(1, "Solar hour angle, degrees: %.2f", ha);
	ha *= DEG2RAD;
	ha_cos = cos(ha);
	roundoff(&ha_cos);
    }

    if (!use_solpos) {
	/* sun declination */
	/***************************************************************
	 * The declination of the sun is the angle between
	 * the rays of the sun and the plane of the Earth's equator.
	 ***************************************************************/

	s_gamma = (2 * M_PI * (doy - 1)) / 365;
	G_debug(1, "fractional year in radians: %.2f", s_gamma);
	/* sun declination for day of year with Fourier series representation
	 * NOTE: based on 1950, override with solpos */
	s_declination = (0.006918 - 0.399912 * cos(s_gamma) + 0.070257 * sin(s_gamma) -
			 0.006758 * cos(2 * s_gamma) + 0.000907 * sin(2 * s_gamma) -
			 0.002697 * cos(3 * s_gamma) + 0.00148 * sin(3 * s_gamma));

	G_debug(1, "sun declination: %.5f", s_declination * RAD2DEG);
	G_debug(1, "sun declination (solpos): %.5f", pd.declin);

	if (lst_time) {
	    north_ll = (window.north + window.south) / 2;
	    east_ll = (window.east + window.west) / 2;
	    if (do_reproj) {
		if (pj_do_proj(&east_ll, &north_ll, &iproj, &oproj) < 0)
		    G_fatal_error(_("Error in pj_do_proj (projection of input coordinate pair)"));
	    }
	    pd.timezone = east_ll / 15.;
	    pd.time_updated = 1;
	    set_solpos_longitude(&pd, east_ll);
	    G_debug(1, "fake timezone: %.2f", pd.timezone);
	    S_solpos(&pd);
	    G_debug(1, "Solar hour angle (solpos), degrees: %.2f", pd.hrang);
	}

	/* always use solpos sun declination */
	s_declination = pd.declin * DEG2RAD;
	sd_sin = sin(s_declination);
	roundoff(&sd_sin);
	sd_cos = cos(s_declination);
	roundoff(&sd_cos);

	G_debug(1, "sun declination (solpos): %.5f", s_declination * RAD2DEG);

	if (0 && lst_time) {
	    pd.timezone = 0;
	    pd.time_updated = pd.longitude_updated = 1;
	    S_solpos(&pd);
	}
    }

    if (elev_name) {
	if ((elev_fd = Rast_open_new(elev_name, FCELL_TYPE)) < 0)
	    G_fatal_error(_("Unable to create raster map <%s>"), elev_name);

	elevbuf = Rast_allocate_f_buf();
    }
    else {
	elevbuf = NULL;
	elev_fd = -1;
    }

    if (azimuth_name) {
	if ((azimuth_fd = Rast_open_new(azimuth_name, FCELL_TYPE)) < 0)
	    G_fatal_error(_("Unable to create raster map <%s>"), azimuth_name);

	azimuthbuf = Rast_allocate_f_buf();
    }
    else {
	azimuthbuf = NULL;
	azimuth_fd = -1;
    }

    if (sunhour_name) {
	if ((sunhour_fd = Rast_open_new(sunhour_name, FCELL_TYPE)) < 0)
	    G_fatal_error(_("Unable to create raster map <%s>"), sunhour_name);

	sunhourbuf = Rast_allocate_f_buf();
    }
    else {
	sunhourbuf = NULL;
	sunhour_fd = -1;
    }

    if (elev_name && azimuth_name) {
	G_message(_("Calculating solar elevation and azimuth..."));
    }
    else if (elev_name) {
	G_message(_("Calculating solar elevation..."));
    }
    else if (azimuth_name) {
	G_message(_("Calculating solar azimuth..."));
    }

    nrows = Rast_window_rows();
    ncols = Rast_window_cols();

    ba2 = 6356752.3142 / 6378137.0;
    ba2 = ba2 * ba2;

    for (row = 0; row < nrows; row++) {

	G_percent(row, nrows, 2);
	
	/* get cell center northing */
	north = window.north - (row + 0.5) * window.ns_res;
	north_ll = north;

	for (col = 0; col < ncols; col++) {
	    long int retval;
	    s_elevation = s_azimuth = -1.;

	    /* get cell center easting */
	    east = window.west + (col + 0.5) * window.ew_res;
	    east_ll = east;

	    if (do_reproj) {
		north_ll = north;

		if (pj_do_proj(&east_ll, &north_ll, &iproj, &oproj) < 0)
		    G_fatal_error(_("Error in pj_do_proj (projection of input coordinate pair)"));
	    }

	    /* geocentric latitude */
	    north_gc = atan(ba2 * tan(DEG2RAD * north_ll));
	    north_gc_sin = sin(north_gc);
	    roundoff(&north_gc_sin);
	    north_gc_cos = cos(north_gc);
	    roundoff(&north_gc_cos);

	    if (!lst_time) {
		set_solpos_longitude(&pd, east_ll);
		pd.latitude = north_gc * RAD2DEG;
		retval = S_solpos(&pd);
		S_decode(retval, &pd);
		G_debug(3, "solpos hour angle: %.5f", pd.hrang);
	    }

	    /* solar elevation angle */
	    if (!use_solpos) {
		if (!lst_time) {
		    ha = pd.hrang;
		    ha_cos = cos(ha * DEG2RAD);
		    roundoff(&ha_cos);
		}
		se_sin = ha_cos * sd_cos * north_gc_cos + sd_sin * north_gc_sin;
		roundoff(&se_sin);
		s_elevation = RAD2DEG * asin(se_sin);
	    }
	    else /* use_solpos && !lst_time */
		s_elevation = pd.elevetr;

	    if (elev_name)
		elevbuf[col] = s_elevation;

	    if (azimuth_name) {
		/* solar azimuth angle */
		if (!use_solpos) {
		    sa_cos = (se_sin * north_gc_sin - sd_sin) /
		             (cos(DEG2RAD * s_elevation) * north_gc_cos);
		    roundoff(&sa_cos);
		    s_azimuth = RAD2DEG * acos(sa_cos);
		    
		    /* morning */
		    s_azimuth = 180. - RAD2DEG * acos(sa_cos);
		    if (ha > 0)   /* afternoon */
			s_azimuth = 360.0 - s_azimuth;
		}
		else
		    s_azimuth = pd.azim;

		azimuthbuf[col] = s_azimuth;
	    }
	    
	    if (sunhour_name) {
		sunhourbuf[col] = (pd.ssetr - pd.sretr) / 60.;
		if (sunhourbuf[col] > 24.)
		    sunhourbuf[col] = 24.;
		if (sunhourbuf[col] < 0.)
		    sunhourbuf[col] = 0.;
	    }

	}
	if (elev_name)
	    Rast_put_f_row(elev_fd, elevbuf);
	if (azimuth_name)
	    Rast_put_f_row(azimuth_fd, azimuthbuf);
	if (sunhour_name)
	    Rast_put_f_row(sunhour_fd, sunhourbuf);
    }
    G_percent(1, 1, 2);

    if (elev_name) {
	Rast_close(elev_fd);
	/* writing history file */
	Rast_short_history(elev_name, "raster", &hist);
	Rast_command_history(&hist);
	Rast_write_history(elev_name, &hist);
    }
    if (azimuth_name) {
	Rast_close(azimuth_fd);
	/* writing history file */
	Rast_short_history(azimuth_name, "raster", &hist);
	Rast_command_history(&hist);
	Rast_write_history(azimuth_name, &hist);
    }
    if (sunhour_name) {
	Rast_close(sunhour_fd);
	/* writing history file */
	Rast_short_history(sunhour_name, "raster", &hist);
	Rast_command_history(&hist);
	Rast_write_history(sunhour_name, &hist);
    }

    G_done_msg(" ");

    exit(EXIT_SUCCESS);
}

void set_solpos_time(struct posdata *pdat, int year, int month, int day,
                    int hour, int minute, int second)
{
    pdat->year = year; 
    pdat->month = month; 
    pdat->day = day; 
    pdat->daynum = day; 
    pdat->hour = hour; 
    pdat->minute = minute; 
    pdat->second = second;
    pdat->timezone = 0;

    pdat->time_updated = 1; 
    pdat->longitude_updated = 1;
}

void set_solpos_longitude(struct posdata *pdat, double longitude)
{
    pdat->longitude = longitude;

    pdat->longitude_updated = 1;
}

int roundoff(double *x)
{
    /* watch out for the roundoff errors */
    if (fabs(*x) > 1.0) {
	if (*x > 0.0)
	    *x = 1.0;
	else
	    *x = -1.0;

	return 1;
    }

    return 0;
}
