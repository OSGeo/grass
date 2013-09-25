
/****************************************************************************
 *
 * MODULE:       r.sunmask
 * AUTHOR(S):    Janne Soimasuo, Finland 1994 (original contributor)
 *               update to FP by Huidae Cho <grass4u gmail.com> 2001
 *               added solpos algorithm feature by Markus Neteler 2001
 *               Brad Douglas <rez touchofmadness.com>, Glynn Clements <glynn gclements.plus.com>,
 *               Hamish Bowman <hamish_b yahoo.com>, Paul Kelly <paul-grass stjohnspoint.co.uk>
 * PURPOSE:      
 * COPYRIGHT:    (C) 1999-2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
/*
 * r.sunmask:
 *   Calculates the cast shadow areas from a DEM
 *
 * input: DEM (int, float, double)
 * output: binary shadow map
 *    no shadow: null()
 *    shadow:    1
 *
 * Algorithm source: unknown (Janne Soimasuo?)
 * Original module author: Janne Soimasuo, Finland 1994
 *
 * GPL >= 2
 *
 **********************
 * Added solpol sun position calculation:
 * Markus Neteler 4/2001
 *
 **********************
 * MN 2/2001: attempt to update to FP
 * Huidae Cho 3/2001: FP update done
 *                    but it's somewhat slow with non-CELL maps
 *********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

#include "global.h"
#include "solpos00.h"

float asol, phi0, sun_zenith, sun_azimuth;	/* from nadir, from north */
int sunset;

/* to be displayed in r.sunmask */
static char *SOLPOSVERSION = "11 April 2001";

extern struct posdata pd, *pdat;	/* declare a posdata struct and a pointer for
					   it (if desired, the structure could be
					   allocated dynamically with G_malloc) */
struct Cell_head window;

union RASTER_PTR
{
    void *v;
    CELL *c;
    FCELL *f;
    DCELL *d;
};

#ifdef	RASTER_VALUE_FUNC
double raster_value(union RASTER_PTR buf, int data_type, int col);
#else
#define	raster_value(buf, data_type, col)	((double)(data_type == CELL_TYPE ? buf.c[col] : (data_type == FCELL_TYPE ? buf.f[col] : buf.d[col])))
#endif

int main(int argc, char *argv[])
{
    extern struct Cell_head window;
    union RASTER_PTR elevbuf, tmpbuf, outbuf;
    CELL min, max;
    DCELL dvalue, dvalue2, dmin, dmax;
    struct History hist;
    RASTER_MAP_TYPE data_type;
    struct Range range;
    struct FPRange fprange;
    double drow, dcol;
    int elev_fd, output_fd, zeros;
    struct
    {
	struct Option *opt1, *opt2, *opt3, *opt4, *north, *east, *year,
	    *month, *day, *hour, *minutes, *seconds, *timezone;
    } parm;
    struct Flag *flag1, *flag3, *flag4;
    struct GModule *module;
    char *name, *outname;
    double dazi, dalti;
    double azi, alti;
    double nstep, estep;
    double maxh;
    double east, east1, north, north1;
    int row1, col1;
    char OK;
    double timezone;
    int year, month, day, hour, minutes, seconds;
    long retval;
    int solparms, locparms, use_solpos;
    double sunrise, sunset, current_time;
    int sretr = 0, ssetr = 0, sretr_sec = 0, ssetr_sec = 0;
    double dsretr, dssetr;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("solar"));
    G_add_keyword(_("sun position"));
    module->label = _("Calculates cast shadow areas from sun position and elevation raster map.");
    module->description = _("Either exact sun position (A) is specified, or date/time to calculate "
			    "the sun position (B) by r.sunmask itself.");
    
    parm.opt1 = G_define_standard_option(G_OPT_R_ELEV);
    
    parm.opt2 = G_define_standard_option(G_OPT_R_OUTPUT);
    parm.opt2->required = NO;
    
    parm.opt3 = G_define_option();
    parm.opt3->key = "altitude";
    parm.opt3->type = TYPE_DOUBLE;
    parm.opt3->required = NO;
    parm.opt3->options = "0-89.999";
    parm.opt3->description =
	_("Altitude of the sun above horizon, degrees (A)");
    parm.opt3->guisection = _("Position");

    parm.opt4 = G_define_option();
    parm.opt4->key = "azimuth";
    parm.opt4->type = TYPE_DOUBLE;
    parm.opt4->required = NO;
    parm.opt4->options = "0-360";
    parm.opt4->description =
	_("Azimuth of the sun from the north, degrees (A)");
    parm.opt4->guisection = _("Position");

    parm.year = G_define_option();
    parm.year->key = "year";
    parm.year->type = TYPE_INTEGER;
    parm.year->required = NO;
    parm.year->description = _("Year (B)");
    parm.year->options = "1950-2050";
    parm.year->guisection = _("Time");

    parm.month = G_define_option();
    parm.month->key = "month";
    parm.month->type = TYPE_INTEGER;
    parm.month->required = NO;
    parm.month->description = _("Month (B)");
    parm.month->options = "0-12";
    parm.month->guisection = _("Time");

    parm.day = G_define_option();
    parm.day->key = "day";
    parm.day->type = TYPE_INTEGER;
    parm.day->required = NO;
    parm.day->description = _("Day (B)");
    parm.day->options = "0-31";
    parm.day->guisection = _("Time");

    parm.hour = G_define_option();
    parm.hour->key = "hour";
    parm.hour->type = TYPE_INTEGER;
    parm.hour->required = NO;
    parm.hour->description = _("Hour (B)");
    parm.hour->options = "0-24";
    parm.hour->guisection = _("Time");

    parm.minutes = G_define_option();
    parm.minutes->key = "minute";
    parm.minutes->type = TYPE_INTEGER;
    parm.minutes->required = NO;
    parm.minutes->description = _("Minutes (B)");
    parm.minutes->options = "0-60";
    parm.minutes->guisection = _("Time");

    parm.seconds = G_define_option();
    parm.seconds->key = "second";
    parm.seconds->type = TYPE_INTEGER;
    parm.seconds->required = NO;
    parm.seconds->description = _("Seconds (B)");
    parm.seconds->options = "0-60";
    parm.seconds->guisection = _("Time");

    parm.timezone = G_define_option();
    parm.timezone->key = "timezone";
    parm.timezone->type = TYPE_INTEGER;
    parm.timezone->required = NO;
    parm.timezone->label =
	_("Timezone");
    parm.timezone->description = _("East positive, offset from GMT, also use to adjust daylight savings");
    parm.timezone->guisection = _("Time");

    parm.east = G_define_option();
    parm.east->key = "east";
    parm.east->key_desc = "value";
    parm.east->type = TYPE_STRING;
    parm.east->required = NO;
    parm.east->label =
	_("Easting coordinate (point of interest)");
    parm.east->description = _("Default: map center");
    parm.east->guisection = _("Position");

    parm.north = G_define_option();
    parm.north->key = "north";
    parm.north->key_desc = "value";
    parm.north->type = TYPE_STRING;
    parm.north->required = NO;
    parm.north->label =
	_("Northing coordinate (point of interest)");
    parm.north->description = _("Default: map center");
    parm.north->guisection = _("Position");

    flag1 = G_define_flag();
    flag1->key = 'z';
    flag1->description = _("Don't ignore zero elevation");

    flag3 = G_define_flag();
    flag3->key = 's';
    flag3->description = _("Calculate sun position only and exit");
    flag3->guisection = _("Print");
    
    flag4 = G_define_flag();
    flag4->key = 'g';
    flag4->description =
	_("Print the sun position output in shell script style");
    flag4->guisection = _("Print");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    zeros = flag1->answer;

    G_get_window(&window);

    /* if not given, get east and north: XX */
    if (!parm.north->answer || !parm.east->answer) {
	north = (window.north - window.south) / 2. + window.south;
	east = (window.west - window.east) / 2. + window.east;
	G_verbose_message(_("Using map center coordinates: %f %f"), east, north);
    }
    else {			/* user defined east, north: */

	sscanf(parm.north->answer, "%lf", &north);
	sscanf(parm.east->answer, "%lf", &east);
	if (strlen(parm.east->answer) == 0)
	    G_fatal_error(_("Empty east coordinate specified"));
	if (strlen(parm.north->answer) == 0)
	    G_fatal_error(_("Empty north coordinate specified"));
    }

    /* check which method to use for sun position:
       either user defines directly sun position or it is calculated */

    if (parm.opt3->answer && parm.opt4->answer)
	solparms = 1;		/* opt3 & opt4 complete */
    else
	solparms = 0;		/* calculate sun position */

    if (parm.year->answer && parm.month->answer && parm.day->answer &&
	parm.hour->answer && parm.minutes->answer && parm.seconds->answer &&
	parm.timezone->answer)
	locparms = 1;		/* complete */
    else
	locparms = 0;

    if (solparms && locparms)	/* both defined */
	G_fatal_error(_("Either define sun position or location/date/time parameters"));

    if (!solparms && !locparms)	/* nothing defined */
	G_fatal_error(_("Neither sun position nor east/north, date/time/timezone definition are complete"));

    /* if here, one definition was complete */
    if (locparms) {
	G_message(_("Calculating sun position... (using solpos (V. %s) from NREL)"),
		  SOLPOSVERSION);
	use_solpos = 1;
    }
    else {
	G_message(_("Using user defined sun azimuth, altitude settings (ignoring eventual other values)"));
	use_solpos = 0;
    }

    name = parm.opt1->answer;
    outname = parm.opt2->answer;
    if (!use_solpos) {
	sscanf(parm.opt3->answer, "%lf", &dalti);
	sscanf(parm.opt4->answer, "%lf", &dazi);
    }
    else {
	sscanf(parm.year->answer, "%d", &year);
	sscanf(parm.month->answer, "%d", &month);
	sscanf(parm.day->answer, "%d", &day);
	sscanf(parm.hour->answer, "%d", &hour);
	sscanf(parm.minutes->answer, "%d", &minutes);
	sscanf(parm.seconds->answer, "%d", &seconds);
	sscanf(parm.timezone->answer, "%lf", &timezone);
    }

    /* NOTES: G_calc_solar_position ()
       - the algorithm will compensate for leap year.
       - longitude, latitude: decimal degree
       - timezone: DO NOT ADJUST FOR DAYLIGHT SAVINGS TIME.
       - timezone: negative for zones west of Greenwich
       - lat/long: east and north positive
       - the atmospheric refraction is calculated for 1013hPa, 15�C 
       - time: local time from your watch

       Order of parameters:
       long, lat, timezone, year, month, day, hour, minutes, seconds 
     */

    if (use_solpos) {
	G_debug(3, "\nlat:%f  long:%f", north, east);
	retval =
	    calc_solar_position(east, north, timezone, year, month, day,
				hour, minutes, seconds);

	/* Remove +0.5 above if you want round-down instead of round-to-nearest */
	sretr = (int)floor(pdat->sretr);	/* sunrise */
	dsretr = pdat->sretr;
	sretr_sec =
	    (int)
	    floor(((dsretr - floor(dsretr)) * 60 -
		   floor((dsretr - floor(dsretr)) * 60)) * 60);
	ssetr = (int)floor(pdat->ssetr);	/* sunset */
	dssetr = pdat->ssetr;
	ssetr_sec =
	    (int)
	    floor(((dssetr - floor(dssetr)) * 60 -
		   floor((dssetr - floor(dssetr)) * 60)) * 60);

	/* print the results */
	if (retval == 0) {	/* error check */
	    if (flag3->answer) {
		if (flag4->answer) {
		    fprintf(stdout, "date=%d/%02d/%02d\n", pdat->year,
			    pdat->month, pdat->day);
		    fprintf(stdout, "daynum=%d\n", pdat->daynum);
		    fprintf(stdout, "time=%02i:%02i:%02i\n", pdat->hour,
			    pdat->minute, pdat->second);
		    fprintf(stdout, "decimaltime=%f\n",
			    pdat->hour + (pdat->minute * 100.0 / 60.0 +
					  pdat->second * 100.0 / 3600.0) /
			    100.);
		    fprintf(stdout, "longitudine=%f\n", pdat->longitude);
		    fprintf(stdout, "latitude=%f\n", pdat->latitude);
		    fprintf(stdout, "timezone=%f\n", pdat->timezone);
		    fprintf(stdout, "sunazimuth=%f\n", pdat->azim);
		    fprintf(stdout, "sunangleabovehorizon=%f\n",
			    pdat->elevref);

		    if (sretr / 60 <= 24.0) {
			fprintf(stdout, "sunrise=%02d:%02d:%02d\n",
				sretr / 60, sretr % 60, sretr_sec);
			fprintf(stdout, "sunset=%02d:%02d:%02d\n", ssetr / 60,
				ssetr % 60, ssetr_sec);
		    }
		}
		else {
		    fprintf(stdout, "%d/%02d/%02d, daynum: %d, time: %02i:%02i:%02i (decimal time: %f)\n",
			    pdat->year, pdat->month, pdat->day,
			    pdat->daynum, pdat->hour, pdat->minute,
			    pdat->second,
			    pdat->hour + (pdat->minute * 100.0 / 60.0 +
					  pdat->second * 100.0 / 3600.0) /
			    100.);
		    fprintf(stdout, "long: %f, lat: %f, timezone: %f\n",
			    pdat->longitude, pdat->latitude,
			    pdat->timezone);
		    fprintf(stdout, "Solar position: sun azimuth: %f, sun angle above horz. (refraction corrected): %f\n",
			    pdat->azim, pdat->elevref);
		    
		    if (sretr / 60 <= 24.0) {
			fprintf(stdout, "Sunrise time (without refraction): %02d:%02d:%02d\n",
				sretr / 60, sretr % 60, sretr_sec);
			fprintf(stdout, "Sunset time  (without refraction): %02d:%02d:%02d\n",
				ssetr / 60, ssetr % 60, ssetr_sec);
		    }
		}
	    }
	    sunrise = pdat->sretr / 60.;	/* decimal minutes */
	    sunset = pdat->ssetr / 60.;
	    current_time =
		pdat->hour + (pdat->minute / 60.) + (pdat->second / 3600.);
	}
	else			/* fatal error in G_calc_solar_position() */
	    G_fatal_error(_("Please correct settings"));
    }

    if (use_solpos) {
	dalti = pdat->elevref;
	dazi = pdat->azim;
    }				/* otherwise already defined */


    /* check sunrise */
    if (use_solpos) {
	G_debug(3, "current_time:%f sunrise:%f", current_time, sunrise);
	if ((current_time < sunrise)) {
	    if (sretr / 60 <= 24.0)
		G_message(_("Time (%02i:%02i:%02i) is before sunrise (%02d:%02d:%02d)"),
			  pdat->hour, pdat->minute, pdat->second, sretr / 60,
			  sretr % 60, sretr_sec);
	    else
		G_message(_("Time (%02i:%02i:%02i) is before sunrise"),
			  pdat->hour, pdat->minute, pdat->second);

	    G_warning(_("Nothing to calculate. Please verify settings."));
	}
	if ((current_time > sunset)) {
	    if (sretr / 60 <= 24.0)
		G_message(_("Time (%02i:%02i:%02i) is after sunset (%02d:%02d:%02d)"),
			  pdat->hour, pdat->minute, pdat->second, ssetr / 60,
			  ssetr % 60, ssetr_sec);
	    else
		G_message(_("Time (%02i:%02i:%02i) is after sunset"),
			  pdat->hour, pdat->minute, pdat->second);
	    G_warning(_("Nothing to calculate. Please verify settings."));
	}
    }

    if (flag3->answer && (use_solpos == 1)) {	/* we only want the sun position */
	exit(EXIT_SUCCESS);
    }
    else if (flag3->answer && (use_solpos == 0)) {
	/* are you joking ? */
	G_message(_("You already know the sun position"));
	exit(EXIT_SUCCESS);
    }

    if (!outname)
	G_fatal_error(_("Option <%s> required"), parm.opt2->key);

    elev_fd = Rast_open_old(name, "");
    output_fd = Rast_open_c_new(outname);
    
    data_type = Rast_get_map_type(elev_fd);
    elevbuf.v = Rast_allocate_buf(data_type);
    tmpbuf.v = Rast_allocate_buf(data_type);
    outbuf.v = Rast_allocate_buf(CELL_TYPE);	/* binary map */

    if (data_type == CELL_TYPE) {
	if ((Rast_read_range(name, "", &range)) < 0)
	    G_fatal_error(_("Unable to open range file for raster map <%s>"), name);
	Rast_get_range_min_max(&range, &min, &max);
	dmin = (double)min;
	dmax = (double)max;
    }
    else {
	Rast_read_fp_range(name, "", &fprange);
	Rast_get_fp_range_min_max(&fprange, &dmin, &dmax);
    }

    azi = 2 * M_PI * dazi / 360;
    alti = 2 * M_PI * dalti / 360;
    nstep = cos(azi) * window.ns_res;
    estep = sin(azi) * window.ew_res;
    row1 = 0;

    G_message(_("Calculating shadows from DEM..."));
    while (row1 < window.rows) {
	G_percent(row1, window.rows, 2);
	col1 = 0;
	drow = -1;
	Rast_get_row(elev_fd, elevbuf.v, row1, data_type);
	
	while (col1 < window.cols) {
	    dvalue = raster_value(elevbuf, data_type, col1);
	    /*              outbuf.c[col1]=1; */
	    Rast_set_null_value(&outbuf.c[col1], 1, CELL_TYPE);
	    OK = 1;
	    east = Rast_col_to_easting(col1 + 0.5, &window);
	    north = Rast_row_to_northing(row1 + 0.5, &window);
	    east1 = east;
	    north1 = north;
	    if (dvalue == 0.0 && !zeros)
		OK = 0;
	    while (OK == 1)
	    {
		east += estep;
		north += nstep;
		if (north > window.north || north < window.south
		    || east > window.east || east < window.west)
		    OK = 0;
		else {
		    maxh = tan(alti) *
			sqrt((north1 - north) * (north1 - north) +
			     (east1 - east) * (east1 - east));
		    if ((maxh) > (dmax - dvalue))
			OK = 0;
		    else {
			dcol = Rast_easting_to_col(east, &window);
			if (drow != Rast_northing_to_row(north, &window)) {
			    drow = Rast_northing_to_row(north, &window);
			    Rast_get_row(elev_fd, tmpbuf.v, (int)drow,
					 data_type);
			}
			dvalue2 = raster_value(tmpbuf, data_type, (int)dcol);
			if ((dvalue2 - dvalue) > (maxh)) {
			    OK = 0;
			    outbuf.c[col1] = 1;
			}
		    }
		}
	    }
	    G_debug(3, "Analysing col %i", col1);
	    col1 += 1;
	}
	G_debug(3, "Writing result row %i of %i", row1, window.rows);
	Rast_put_row(output_fd, outbuf.c, CELL_TYPE);
	row1 += 1;
    }
    G_percent(1, 1, 1);

    Rast_close(output_fd);
    Rast_close(elev_fd);

    /* writing history file */
    Rast_short_history(outname, "raster", &hist);
    Rast_format_history(&hist, HIST_DATSRC_1, "raster elevation map %s", name);
    Rast_command_history(&hist);
    Rast_write_history(outname, &hist);

    exit(EXIT_SUCCESS);
}

#ifdef	RASTER_VALUE_FUNC
double raster_value(union RASTER_PTR buf, int data_type, int col)
{
    double retval;

    switch (data_type) {
    case CELL_TYPE:
	retval = (double)buf.c[col];
	break;
    case FCELL_TYPE:
	retval = (double)buf.f[col];
	break;
    case DCELL_TYPE:
	retval = (double)buf.d[col];
	break;
    }

    return retval;
}
#endif
