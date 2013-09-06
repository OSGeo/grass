#include <stdio.h>
#include <math.h>
#include <grass/vector.h>
#include "sw_defs.h"
#include "defs.h"
#include "write.h"

/*-extend_line()  finds coordinates along the boundary of a window
 *                  that also lie on a specified line (ax+by=c). The
 *                  line can cross at least two boundaries---the line
 *                  that intersects the midpoint of s1 and s2 determines 
 *                  which coordinates are placed in (c_x, c_y).
 *
 * The limits of the window are described by:
 *    e:  east
 *    w:  west
 *    s:  south
 *    n:  north
 * Note that the following constraints must be true:
 *    ( w < e )     ( s < n )
 *
 *    x and y are points on the line ax + by = c that are assumed
 *    to lie within the window.
 *
 *    the c_x and c_y values are changed.
 *
 *    returns: 0 on error, 1 otherwise
 */

int extend_line(double s, double n, double w, double e,
		double a, double b, double c, double x, double y,
		double *c_x, double *c_y, int knownPointAtLeft)
{
    double nx, ny;		/* intersection coordinates */

    if (x >= w && x <= e && y >= s && y <= n) {
	/* vertical line? */
	if (a == 0) {
	    *c_x = knownPointAtLeft ? e : w;
	    *c_y =  y;
	    return 1;
	}

	/* horizontal line? */
	if (b == 0) {
	    *c_x = x;
	    *c_y = knownPointAtLeft ? s : n;
	    return 1;
	}

	/* south */
	nx = (c - b * s) / a;
	if (Vect_point_in_box(nx, s, 0.0, &Box) &&
	    ((nx > x && knownPointAtLeft) || (nx <= x && !knownPointAtLeft)))
	{
	    *c_x = nx;
	    *c_y = s;
	    return 1;
	}

	/* north */
	nx = (c - b * n) / a;
	if (Vect_point_in_box(nx, n, 0.0, &Box) &&
	    ((nx > x && knownPointAtLeft) || (nx <= x && !knownPointAtLeft)))
	{
	    *c_x = nx;
	    *c_y = n;
	    return 1;
	}

	if (knownPointAtLeft) {
	    /* east */
	    ny = (c - a * e) / b;
	    if (Vect_point_in_box(e, ny, 0.0, &Box)) {
		*c_x = e;
		*c_y = ny;
		return 1;
	    }
	}
	else {
	    /* west */
	    ny = (c - a * w) / b;
	    if (Vect_point_in_box(w, ny, 0.0, &Box)) {
		*c_x = w;
		*c_y = ny;
		return 1;
	    }
	}
    }
    return 0;
}
