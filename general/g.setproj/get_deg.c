/* get_deg.c    1.0   6/182/91
 *    Created by : R.L.Glenn , Soil Conservation Service, USDA
 *    Purpose: function
 *                    Provide a means of collecting user lat/long
 *                    data, in different formats; convert to
 *                       decimal degrees
 *    Input arguments : lat or long string   and
 *                       a 1 for latitude or 0 for longitude
 *    Output arguments: decimal degrees in string
 *
 *            Note: All functions are callable directly see
 *                    g.get_stp       g.get_fips      g.stp_proj  geo
 *
 */
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <grass/gis.h>

int get_deg(char *strng, int ll_swt)
{
    double degrees;

    switch (ll_swt) {
    case 0:
	{
	    if (!G_scan_easting(strng, &degrees, PROJECTION_LL)) {
		fprintf(stderr,
			"\n\t** %s is invalid for longitude **\n", strng);
		G_sleep(2);
		return (0);
	    }
	    break;
	}
    case 1:
	{
	    if (G_scan_northing(strng, &degrees, PROJECTION_LL) == 0) {
		fprintf(stderr,
			"\n\t** %s is invalid for latitude **\n", strng);
		G_sleep(2);
		return (0);
	    }
	    break;
	}
    }				/* end of switch */
    sprintf(strng, "%.10f", degrees);
    return (1);
}
