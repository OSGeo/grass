/*
 * G_calc_solar_position() calculates solar position parameters from
 * given position, date and time
 *
 * Written by Markus Neteler <neteler@geog.uni-hannover.de>
 * with kind help from Morten Hulden <morten untamo.net>
 *
 *----------------------------------------------------------------------------
 * using solpos.c with permission from
 *   From rredc@nrel.gov Wed Mar 21 18:37:25 2001
 *   Message-Id: <v04220805b6de9b1ad6ff@[192.174.39.30]>
 *   Mary Anderberg
 *   http://rredc.nrel.gov
 *   National Renewable Energy Laboratory
 *   1617 Cole Boulevard
 *   Golden, Colorado, USA 80401
 *
 *   http://rredc.nrel.gov/solar/codes_algs/solpos/
 *
 *  G_calc_solar_position is based on: soltest.c
 *    by
 *    Martin Rymes
 *    National Renewable Energy Laboratory
 *    25 March 1998
 *----------------------------------------------------------------------------*/

/* uncomment to get debug output */

#include <math.h>
#include <string.h>
#include <stdio.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/gprojects.h>
#include "solpos00.h"

struct posdata pd, *pdat;	/* declare solpos data struct and a pointer for it */


long calc_solar_position(double longitude, double latitude, double timezone,
			 int year, int month, int day, int hour, int minute,
			 int second)
{

    /* Note: this code is valid from year 1950 to 2050 (solpos restriction)

       - the algorithm will compensate for leap year.
       - longitude, latitude: decimal degree
       - timezone: DO NOT ADJUST FOR DAYLIGHT SAVINGS TIME.
       - timezone: negative for zones west of Greenwich
       - lat/long: east and north positive
       - the atmospheric refraction is calculated for 1013hPa, 15 degrees C
       - time: local time from your watch
     */

    long retval;		/* to capture S_solpos return codes */
    struct Key_Value *in_proj_info, *in_unit_info;	/* projection information of input map */
    struct pj_info iproj;	/* input map proj parameters  */
    struct pj_info oproj;	/* output map proj parameters  */
    struct pj_info tproj;	/* transformation parameters  */
    extern struct Cell_head window;
    int inside;


    /* we don't like to run G_calc_solar_position in xy locations */
    if (window.proj == 0)
	G_fatal_error(_("Unable to calculate sun position in un-projected locations. "
			"Specify sunposition directly."));

    pdat = &pd;			/* point to the structure for convenience */

    /* Initialize structure to default values. (Optional only if ALL input
       parameters are initialized in the calling code, which they are not
       in this example.) */

    S_init(pdat);

    /* check if given point is in current window */
    G_debug(1, "window.north: %f, window.south: %f\n", window.north,
	    window.south);
    G_debug(1, "window.west:  %f, window.east : %f\n", window.west,
	    window.east);
    
    inside = 0;
    if (latitude >= window.south && latitude <= window.north &&
	longitude >= window.west && longitude <= window.east)
	inside = 1;
    if (!inside)
	G_warning(_("Specified point %f, %f outside of current region, "
		    "is that intended? Anyway, it will be used."),
		  longitude, latitude);

    /* if coordinates are not in lat/long format, transform them: */
    if ((G_projection() != PROJECTION_LL) && window.proj != 0) {
	G_debug(1, "Transforming input coordinates to lat/long (req. for solar position)");
	
	/* read current projection info */
	if ((in_proj_info = G_get_projinfo()) == NULL)
	    G_fatal_error(_("Unable to get projection info of current location"));

	if ((in_unit_info = G_get_projunits()) == NULL)
	    G_fatal_error(_("Unable to get projection units of current location"));

	if (pj_get_kv(&iproj, in_proj_info, in_unit_info) < 0)
	    G_fatal_error(_("Unable to get projection key values of current location"));
	
	G_free_key_value(in_proj_info);
	G_free_key_value(in_unit_info);

	/* Try using pj_print_proj_params() instead of all this */
	G_debug(1, "Projection found in location:");
	G_debug(1, "IN: meter: %f zone: %i proj: %s (iproj struct)",
		iproj.meters, iproj.zone, iproj.proj);
	G_debug(1, "IN coord: longitude: %f, latitude: %f", longitude,
		latitude);

	oproj.pj = NULL;
	tproj.def = NULL;

	if (GPJ_init_transform(&iproj, &oproj, &tproj) < 0)
	    G_fatal_error(_("Unable to initialize coordinate transformation"));

	/* XX do the transform 
	 *               outx        outy    in_info  out_info */

	if (GPJ_transform(&iproj, &oproj, &tproj, PJ_FWD,
			  &longitude, &latitude, NULL) < 0)
	    G_fatal_error(_("Error in %s (projection of input coordinate pair)"), 
			   "GPJ_transform()");

	G_debug(1, "Transformation to lat/long:");
	G_debug(1, "OUT: longitude: %f, latitude: %f", longitude,
		latitude);

    }				/* transform if not LL */

    pdat->longitude = longitude;	/* Note that latitude and longitude are  */
    pdat->latitude = latitude;	/*   in DECIMAL DEGREES, not Deg/Min/Sec */
    pdat->timezone = timezone;	/* DO NOT ADJUST FOR DAYLIGHT SAVINGS TIME. */

    pdat->year = year;		/* The year */
    pdat->function &= ~S_DOY;
    pdat->month = month;
    pdat->day = day;		/* the algorithm will compensate for leap year, so
				   you just count days). S_solpos can be
				   configured to accept day-of-the year */

    /* The time of day (STANDARD (GMT) time) */

    pdat->hour = hour;
    pdat->minute = minute;
    pdat->second = second;

    /* Let's assume that the temperature is 20 degrees C and that
       the pressure is 1013 millibars.  The temperature is used for the
       atmospheric refraction correction, and the pressure is used for the
       refraction correction and the pressure-corrected airmass. */

    pdat->temp = 20.0;
    pdat->press = 1013.0;

    /* Finally, we will assume that you have a flat surface
       facing nowhere, tilted at latitude. */

    pdat->tilt = pdat->latitude;	/* tilted at latitude */
    pdat->aspect = 180.0;

    /* perform the calculation */
    retval = S_solpos(pdat);	/* S_solpos function call: returns long value */
    S_decode(retval, pdat);	/* prints an error in case of problems */

    return retval;

}
