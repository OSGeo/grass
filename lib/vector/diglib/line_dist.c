/*
 ****************************************************************************
 *
 * MODULE:       Vector library 
 *              
 * AUTHOR(S):    Original author CERL, probably Dave Gerdes.
 *               Update to GRASS 5.7 Radim Blazek.
 *
 * PURPOSE:      Lower level functions for reading/writing/manipulating vectors.
 *
 * COPYRIGHT:    (C) 2001 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *              License (>=v2). Read the file COPYING that comes with GRASS
 *              for details.
 *
 *****************************************************************************/
#include <math.h>

#define ZERO(x) ((x) < tolerance && (x) > -tolerance)
#define TOLERANCE 1.0e-10
static double tolerance = TOLERANCE;

int dig_set_distance_to_line_tolerance(double t)
{
    if (t <= 0.0)
	t = TOLERANCE;
    tolerance = t;

    return 0;
}

/*
 *   dig_distance2_point_to_line ()
 *   compute square of distance of point (x,y) to line segment (x1,y1 - x2,y2)
 *   ( works correctly for  x1==x2 && y1==y2 )
 *
 *   returns: square distance
 *   sets (if not NULL): *px, *py - nearest point on segment
 *                       *pdist - distance of px,py from segment start
 *                       *status = 0 if ok, -1 if t < 0  and 1 if t > 1
 *                                 (tells if point is w/in segment space, or past ends)
 */

double dig_distance2_point_to_line(double x, double y, double z,	/* point */
				   double x1, double y1, double z1,	/* line segment */
				   double x2, double y2, double z2, int with_z,	/* use z coordinate, (3D calculation) */
				   double *px, double *py, double *pz,	/* point on segment */
				   double *pdist,	/* distance of point on segment from the first point of segment */
				   int *status)
{
    register double dx, dy, dz;
    register double dpx, dpy, dpz;
    register double tpx, tpy, tpz;
    double t;
    int st;

    st = 0;

    if (!with_z) {
	z = 0;
	z1 = 0;
	z2 = 0;
    }

    dx = x2 - x1;
    dy = y2 - y1;
    dz = z2 - z1;

    if (ZERO(dx) && ZERO(dy) && ZERO(dz)) {	/* line is degenerate */
	dx = x1 - x;
	dy = y1 - y;
	dz = z1 - z;
	tpx = x1;
	tpy = y1;
	tpz = z1;
    }
    else {
	t = (dx * (x - x1) + dy * (y - y1) + dz * (z - z1)) / 
	    (dx * dx + dy * dy + dz * dz);

	if (t <= 0.0) {		/* go to x1,y1,z1 */
	    if (t < 0.0) {
		st = -1;
	    }
	    tpx = x1;
	    tpy = y1;
	    tpz = z1;
	}
	else if (t >= 1.0) {	/* go to x2,y2,z2 */
	    if (t > 1.0) {
		st = 1;
	    }
	    tpx = x2;
	    tpy = y2;
	    tpz = z2;
	}
	else {
	    /* go t from x1,y1,z1 towards x2,y2,z2 */
	    tpx = dx * t + x1;
	    tpy = dy * t + y1;
	    tpz = dz * t + z1;
	}
	dx = tpx - x;
	dy = tpy - y;
	dz = tpz - z;
    }

    if (px)
	*px = tpx;
    if (py)
	*py = tpy;
    if (pz)
	*pz = tpz;
    if (status)
	*status = st;

    if (pdist) {
	dpx = tpx - x1;
	dpy = tpy - y1;
	dpz = tpz - z1;
	*pdist = sqrt(dpx * dpx + dpy * dpy + dpz * dpz);
    }

    return (dx * dx + dy * dy + dz * dz);
}
