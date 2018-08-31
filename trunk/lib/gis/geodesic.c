#include <math.h>
#include <grass/gis.h>
#include "pi.h"


/*
 * This code is preliminary. I don't know if it is even
 * correct.
 */

/*
 * From "Map Projections" by Peter Richardus and Ron K. Alder, 1972
 * (526.8 R39m in Map & Geography Library)
 * page  19, formula 2.17
 *
 * Formula is the equation of a geodesic from (lat1,lon1) to (lat2,lon2)
 * Input is lon, output is lat (all in degrees)
 *
 * Note formula only works if 0 < abs(lon2-lon1) < 180
 * If lon1 == lon2 then geodesic is the merdian lon1 
 * (and the formula will fail)
 * if lon2-lon1=180 then the geodesic is either meridian lon1 or lon2
 */

/* TODO:
 *
 * integrate code from raster/r.surf.idw/ll.c
 */


static void adjust_lat(double *);
static void adjust_lon(double *);

static struct state {
    double A, B;
} state;

static struct state *st = &state;

int G_begin_geodesic_equation(double lon1, double lat1, double lon2,
			      double lat2)
{
    double sin21, tan1, tan2;

    adjust_lon(&lon1);
    adjust_lon(&lon2);
    adjust_lat(&lat1);
    adjust_lat(&lat2);
    if (lon1 > lon2) {
	double temp;
	temp = lon1; lon1 = lon2; lon2 = temp;
	temp = lat1; lat1 = lat2; lat2 = temp;
    }
    if (lon1 == lon2) {
	st->A = st->B = 0.0;
	return 0;
    }
    lon1 = Radians(lon1);
    lon2 = Radians(lon2);
    lat1 = Radians(lat1);
    lat2 = Radians(lat2);

    sin21 = sin(lon2 - lon1);
    tan1 = tan(lat1);
    tan2 = tan(lat2);

    st->A = (tan2 * cos(lon1) - tan1 * cos(lon2)) / sin21;
    st->B = (tan2 * sin(lon1) - tan1 * sin(lon2)) / sin21;

    return 1;
}

/* only works if lon1 < lon < lon2 */

double G_geodesic_lat_from_lon(double lon)
{
    adjust_lon(&lon);
    lon = Radians(lon);

    return Degrees(atan(st->A * sin(lon) - st->B * cos(lon)));
}

static void adjust_lon(double *lon)
{
    while (*lon > 180.0)
	*lon -= 360.0;
    while (*lon < -180.0)
	*lon += 360.0;
}

static void adjust_lat(double *lat)
{
    if (*lat > 90.0)
	*lat = 90.0;
    if (*lat < -90.0)
	*lat = -90.0;
}
