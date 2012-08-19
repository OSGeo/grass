#include <grass/gis.h>

double distance(double east, double west)
{
    double incr;
    double meters;
    double e1, e2;

    /* calculate the distance from east edge to west edge at north==0.0
     * for lat-lon this will be along equator. For other databases the
     * north value will make no difference
     *
     * Note, must do lat-lon in 3 pieces, otherwise distance "line" may
     * go the wrong way around the globe
     */

    G_begin_distance_calculations();

    if (east < west) {
	double temp;

	temp = east;
	east = west;
	west = temp;
    }

    incr = (east - west) / 3.0;
    e1 = west + incr;
    e2 = e1 + incr;

    meters = G_distance(west, 0.0, e1, 0.0) +
	     G_distance(e1, 0.0, e2, 0.0) +
	     G_distance(e2, 0.0, east, 0.0);

    return meters;
}
