/*!
  \file lib/gis/distance.c
  
  \brief GIS Library - Distance calculation functions.
  
  WARNING: this code is preliminary and may be changed,
  including calling sequences to any of the functions
  defined here.
  
  (C) 2001-2009, 2011 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Original author CERL
*/

#include <math.h>
#include <grass/gis.h>
#include <grass/glocale.h>

static double min4(double, double, double, double);
static double min2(double, double);

static struct state {
    int projection;
    double factor;
} state;

static struct state *st = &state;

/*!
  \brief Begin distance calculations.
  
  Initializes the distance calculations. It is used both for the
  planimetric and latitude-longitude projections.
  
  \return 0 if projection has no metrix (ie. imagery)
  \return 1 if projection is planimetric
  \return 2 if projection is latitude-longitude
*/
int G_begin_distance_calculations(void)
{
    double a, e2;

    st->factor = 1.0;
    switch (st->projection = G_projection()) {
    case PROJECTION_LL:
	G_get_ellipsoid_parameters(&a, &e2);
	G_begin_geodesic_distance(a, e2);
	return 2;
    default:
	st->factor = G_database_units_to_meters_factor();
	if (st->factor <= 0.0) {
	    st->factor = 1.0;	/* assume meter grid */
	    return 0;
	}
	return 1;
    }
}

/*!
  \brief Returns distance in meters.
  
  This routine computes the distance, in meters, from
  <i>x1</i>,<i>y1</i> to <i>x2</i>,<i>y2</i>. If the projection is
  latitude-longitude, this distance is measured along the
  geodesic. Two routines perform geodesic distance calculations.
  
  \param e1,n1 east-north coordinates of first point
  \param e2,n2 east-north coordinates of second point
  
  \return distance
*/
double G_distance(double e1, double n1, double e2, double n2)
{
    if (st->projection == PROJECTION_LL)
	return G_geodesic_distance(e1, n1, e2, n2);
    else
	return st->factor * hypot(e1 - e2, n1 - n2);
}

/*!
  \brief Returns distance between two line segments in meters.
  
  \param ax1,ay1,ax2,ay2 first segment
  \param bx1,by1,bx2,by2 second segment
  
  \return distance value
*/
double G_distance_between_line_segments(double ax1, double ay1,
					double ax2, double ay2,
					double bx1, double by1,
					double bx2, double by2)
{
    double ra, rb;
    double x, y;

    /* if the segments intersect, then the distance is zero */
    if (G_intersect_line_segments(ax1, ay1, ax2, ay2,
				  bx1, by1, bx2, by2, &ra, &rb, &x, &y) > 0)
	return 0.0;
    return
	min4(G_distance_point_to_line_segment(ax1, ay1, bx1, by1, bx2, by2),
	     G_distance_point_to_line_segment(ax2, ay2, bx1, by1, bx2, by2),
	     G_distance_point_to_line_segment(bx1, by1, ax1, ay1, ax2, ay2),
	     G_distance_point_to_line_segment(bx2, by2, ax1, ay1, ax2, ay2)
	);
}

/*!
  \brief Returns distance between a point and line segment in meters.
  
  \param xp,yp point coordinates
  \param x1,y1 segment point coordinates
  \param x2,y2 segment point coordinates
  
  \return distance
*/
double G_distance_point_to_line_segment(double xp, double yp,
					double x1, double y1, double x2,
					double y2)
{
    double dx, dy;
    double x, y;
    double xq, yq, ra, rb;
    int t;

    /* define the perpendicular to the segment through the point */
    dx = x1 - x2;
    dy = y1 - y2;

    if (dx == 0.0 && dy == 0.0)
	return G_distance(x1, y1, xp, yp);

    if (fabs(dy) > fabs(dx)) {
	xq = xp + dy;
	yq = (dx / dy) * (xp - xq) + yp;
    }
    else {
	yq = yp + dx;
	xq = (dy / dx) * (yp - yq) + xp;
    }

    /* find the intersection of the perpendicular with the segment */
    t = G_intersect_line_segments(xp, yp, xq, yq, x1, y1, x2, y2, &ra,
				  &rb, &x, &y);
    switch (t) {
    case 0:
    case 1:
	break;
    default:
	/* parallel/colinear cases shouldn't occur with perpendicular lines */
	G_warning(_("%s: shouldn't happen: "
		    "code=%d P=(%f,%f) S=(%f,%f)(%f,%f)"),
		  "G_distance_point_to_line_segment", t, xp, yp, x1, y1, x2, y2);
	return -1.0;
    }

    /* if x,y lies on the segment, then the distance is from to x,y */
    if (rb >= 0 && rb <= 1.0)
	return G_distance(x, y, xp, yp);

    /* otherwise the distance is the short of the distances to the endpoints
     * of the segment
     */
    return min2(G_distance(x1, y1, xp, yp), G_distance(x2, y2, xp, yp));
}

static double min4(double a, double b, double c, double d)
{
    return min2(min2(a, b), min2(c, d));
}

static double min2(double a, double b)
{
    return a < b ? a : b;
}
