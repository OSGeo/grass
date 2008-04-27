/**
 * \file rhumbline.c
 *
 * \brief Rhumbline calculation routines.
 *
 * From "Map Projections" by Peter Richardus and Ron K. Alder, 1972<br>
 * (526.8 R39m in Map & Geography Library)<br>
 * Page 20,21, formulas 2.21, 2.22
 *
 * Formula is the equation of a rhumbline from (lat1,lon1) to 
 * (lat2,lon2). Input is lon, output is lat (all in degrees).
 *
 * <b>Note:</b> Formula only works if 0 < abs(lon2-lon1) < 180.
 * If lon1 == lon2 then rhumbline is the merdian lon1 (and the formula 
 * will fail).
 * <br>
 * <b>WARNING:</b> This code is preliminary. It may not even be correct.
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author GRASS GIS Development Team
 *
 * \date 1999-2006
 */

#include <math.h>
#include <grass/gis.h>
#include "pi.h"


static int adjust_lat(double *);
#if 0
static int adjust_lon(double *);
#endif /* unused */

static double TAN_A, TAN1, TAN2, L;
static int parallel;


/**
 * \fn int G_begin_rhumbline_equation (double lon1, double lat1, double lon2, double lat2)
 *
 * \brief Start rhumbline calculations.
 *
 * <b>Note:</b> This function must be called before other rhumbline 
 * functions to initialize parameters.
 *
 * \param[in] lon1 first longitude
 * \param[in] lat1 first latitude
 * \param[in] lon2 second longitude
 * \param[in] lat2 second latitude
 * \return 1 on success
 * \return 0 on error
 */

int G_begin_rhumbline_equation (
    double lon1,double lat1,double lon2,double lat2)
{
    adjust_lat (&lat1);
    adjust_lat (&lat2);

    if (lon1 == lon2)
    {
	parallel = 1;	/* a lie */
	L = lat1;
	return 0;
    }
    if (lat1 == lat2)
    {
	parallel = 1;
	L = lat1;
	return 1;
    }
    parallel = 0;
    lon1 = Radians(lon1);
    lon2 = Radians(lon2);
    lat1 = Radians(lat1);
    lat2 = Radians(lat2);

    TAN1 = tan (M_PI_4 + lat1/2.0);
    TAN2 = tan (M_PI_4 + lat2/2.0);
    TAN_A = (lon2 - lon1) / (log(TAN2) - log(TAN1));
    L    = lon1;

    return 1;
}


/**
 * \fn double G_rhumbline_lat_from_lon (double lon)
 *
 * \brief Calculates rhumbline latitude.
 *
 * <b>Note:</b> Function only works if lon1 < lon < lon2.
 *
 * \param[in] lon longitude
 * \return double latitude in degrees
 */

double G_rhumbline_lat_from_lon (double lon)
{
    if (parallel) return L;

    lon = Radians(lon);

    return Degrees (2 * atan(exp((lon-L)/TAN_A) * TAN1) - M_PI_2) ;
}


#if 0
static int adjust_lon(double *lon)
{
    while (*lon > 180.0)
	*lon -= 360.0;
    while (*lon < -180.0)
	*lon += 360.0;

    return 0;
}
#endif /* unused */


static int adjust_lat(double *lat)
{
    if (*lat >  90.0) *lat =  90.0;
    if (*lat < -90.0) *lat = -90.0;

    return 0;
}
