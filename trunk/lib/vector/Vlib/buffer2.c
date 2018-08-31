/*!
   \file lib/vector/Vlib/buffer2.c

   \brief Vector library - nearest, adjust, parallel lines

   Higher level functions for reading/writing/manipulating vectors.

   (C) 2001-2009 by the GRASS Development Team

   This program is free software under the 
   GNU General Public License (>=v2). 
   Read the file COPYING that comes with GRASS
   for details.

   \author Original author Radim Blazek (see buffer.c)
   \author Rewritten by Rosen Matev (Google Summer of Code 2008)
 */

#include <stdlib.h>
#include <math.h>
#include <grass/vector.h>
#include <grass/glocale.h>

#include "dgraph.h"

#define LENGTH(DX, DY) (sqrt((DX*DX)+(DY*DY)))
#ifndef MIN
#define MIN(X,Y) ((X<Y)?X:Y)
#endif
#ifndef MAX
#define MAX(X,Y) ((X>Y)?X:Y)
#endif
#define PI M_PI
#define RIGHT_SIDE 1
#define LEFT_SIDE -1
#define LOOPED_LINE 1
#define NON_LOOPED_LINE 0

/* norm_vector() calculates normalized vector form two points */
static void norm_vector(double x1, double y1, double x2, double y2, double *x,
			double *y)
{
    double dx, dy, l;

    dx = x2 - x1;
    dy = y2 - y1;
    if ((dx == 0) && (dy == 0)) {
	/* assume that dx == dy == 0, which should give (NaN,NaN) */
	/* without this, very small dx or dy could result in Infinity */
	*x = 0;
	*y = 0;
	return;
    }
    l = LENGTH(dx, dy);
    *x = dx / l;
    *y = dy / l;

    return;
}

static void rotate_vector(double x, double y, double cosa, double sina,
			  double *nx, double *ny)
{
    *nx = x * cosa - y * sina;
    *ny = x * sina + y * cosa;

    return;
}

/*
 * (x,y) should be normalized vector for common transforms; This func transforms (x,y) to a vector corresponding to da, db, dalpha params
 * dalpha is in radians
 */
static void elliptic_transform(double x, double y, double da, double db,
			       double dalpha, double *nx, double *ny)
{
    double cosa = cos(dalpha);
    double sina = sin(dalpha);

    /*    double cc = cosa*cosa;
       double ss = sina*sina;
       double t = (da-db)*sina*cosa;

       *nx = (da*cc + db*ss)*x + t*y;
       *ny = (da*ss + db*cc)*y + t*x;
       return; */

    double va, vb;

    va = (x * cosa + y * sina) * da;
    vb = (x * (-sina) + y * cosa) * db;
    *nx = va * cosa + vb * (-sina);
    *ny = va * sina + vb * cosa;

    return;
}

/*
 * vect(x,y) must be normalized
 * gives the tangent point of the tangent to ellpise(da,db,dalpha) parallel to vect(x,y)
 * dalpha is in radians
 * ellipse center is in (0,0)
 */
static void elliptic_tangent(double x, double y, double da, double db,
			     double dalpha, double *px, double *py)
{
    double cosa = cos(dalpha);
    double sina = sin(dalpha);
    double u, v, len;

    /* rotate (x,y) -dalpha radians */
    rotate_vector(x, y, cosa, -sina, &x, &y);
    /*u = (x + da*y/db)/2;
       v = (y - db*x/da)/2; */
    u = da * da * y;
    v = -db * db * x;
    len = da * db / sqrt(da * da * v * v + db * db * u * u);
    u *= len;
    v *= len;
    rotate_vector(u, v, cosa, sina, px, py);

    return;
}


/*
 * !!! This is not line in GRASS' sense. See http://en.wikipedia.org/wiki/Line_%28mathematics%29
 */
static void line_coefficients(double x1, double y1, double x2, double y2,
			      double *a, double *b, double *c)
{
    *a = y2 - y1;
    *b = x1 - x2;
    *c = x2 * y1 - x1 * y2;

    return;
}

/*
 * Finds intersection of two straight lines. Returns 0 if the lines are parallel, 1 if they cross,
 * 2 if they are the same line.
 * !!!!!!!!!!!!!!!! FIX THIS TOLLERANCE CONSTANTS BAD (and UGLY) CODE !!!!!!!!!
 */
static int line_intersection(double a1, double b1, double c1, double a2,
			     double b2, double c2, double *x, double *y)
{
    double d;

    if (fabs(a2 * b1 - a1 * b2) == 0) {
	if (fabs(a2 * c1 - a1 * c2) == 0)
	    return 2;
	else
	    return 0;
    }
    else {
	d = a1 * b2 - a2 * b1;
	*x = (b1 * c2 - b2 * c1) / d;
	*y = (c1 * a2 - c2 * a1) / d;
	return 1;
    }
}

static double angular_tolerance(double tol, double da, double db)
{
    double a = MAX(da, db);

    if (tol > a)
	tol = a;

    return 2 * acos(1 - tol / a);
}

/*
 * This function generates parallel line (with loops, but not like the old ones).
 * It is not to be used directly for creating buffers.
 * + added elliptical buffers/par.lines support
 *
 * dalpha - direction of elliptical buffer major axis in degrees
 * da - distance along major axis
 * db: distance along minor (perp.) axis
 * side: side >= 0 - right side, side < 0 - left side
 * when (da == db) we have plain distances (old case)
 * round - 1 for round corners, 0 for sharp corners. (tol is used only if round == 1)
 */
static void parallel_line(struct line_pnts *Points, double da, double db,
			  double dalpha, int side, int round, int caps, int looped,
			  double tol, struct line_pnts *nPoints)
{
    int i, j, res, np;
    double *x, *y;
    double tx, ty, vx, vy, wx, wy, nx, ny, mx, my, rx, ry;
    double vx1, vy1, wx1, wy1;
    double a0, b0, c0, a1, b1, c1;
    double phi1, phi2, delta_phi;
    double nsegments, angular_tol, angular_step;
    int inner_corner, turns360;

    G_debug(3, "parallel_line()");

    if (looped && 0) {
	/* start point != end point */
	return;
    }

    Vect_reset_line(nPoints);

    if (looped) {
	Vect_append_point(Points, Points->x[1], Points->y[1], Points->z[1]);
    }
    np = Points->n_points;
    x = Points->x;
    y = Points->y;

    if ((np == 0) || (np == 1))
	return;

    if ((da == 0) || (db == 0)) {
	Vect_copy_xyz_to_pnts(nPoints, x, y, NULL, np);
	return;
    }

    side = (side >= 0) ? (1) : (-1);	/* normalize variable */
    dalpha *= PI / 180;		/* convert dalpha from degrees to radians */
    angular_tol = angular_tolerance(tol, da, db);

    for (i = 0; i < np - 1; i++) {
	/* save the old values */
	a0 = a1;
	b0 = b1;
	c0 = c1;
	wx = vx;
	wy = vy;


	norm_vector(x[i], y[i], x[i + 1], y[i + 1], &tx, &ty);
	if ((tx == 0) && (ty == 0))
	    continue;

	elliptic_tangent(side * tx, side * ty, da, db, dalpha, &vx, &vy);

	nx = x[i] + vx;
	ny = y[i] + vy;

	mx = x[i + 1] + vx;
	my = y[i + 1] + vy;

	line_coefficients(nx, ny, mx, my, &a1, &b1, &c1);

	if (i == 0) {
	    if (!looped)
		Vect_append_point(nPoints, nx, ny, 0);
	    continue;
	}

	delta_phi = atan2(ty, tx) - atan2(y[i] - y[i - 1], x[i] - x[i - 1]);
	if (delta_phi > PI)
	    delta_phi -= 2 * PI;
	else if (delta_phi <= -PI)
	    delta_phi += 2 * PI;
	/* now delta_phi is in [-pi;pi] */
	turns360 = (fabs(fabs(delta_phi) - PI) < 1e-15);
	inner_corner = (side * delta_phi <= 0) && (!turns360);

	if ((turns360) && (!(caps && round))) {
	    if (caps) {
		norm_vector(0, 0, vx, vy, &tx, &ty);
		elliptic_tangent(side * tx, side * ty, da, db, dalpha, &tx,
				 &ty);
	    }
	    else {
		tx = 0;
		ty = 0;
	    }
	    Vect_append_point(nPoints, x[i] + wx + tx, y[i] + wy + ty, 0);
	    Vect_append_point(nPoints, nx + tx, ny + ty, 0);	/* nx == x[i] + vx, ny == y[i] + vy */
	}
	else if ((!round) || inner_corner) {
	    res = line_intersection(a0, b0, c0, a1, b1, c1, &rx, &ry);
	    /*                if (res == 0) {
	       G_debug(4, "a0=%.18f, b0=%.18f, c0=%.18f, a1=%.18f, b1=%.18f, c1=%.18f", a0, b0, c0, a1, b1, c1);
	       G_fatal_error("Two consecutive line segments are parallel, but not on one straight line! This should never happen.");
	       return;
	       }  */
	    if (res == 1) {
		if (!round)
		    Vect_append_point(nPoints, rx, ry, 0);
		else {
		    /*                    d = dig_distance2_point_to_line(rx, ry, 0, x[i-1], y[i-1], 0, x[i], y[i], 0,
		       0, NULL, NULL, NULL, NULL, NULL);
		       if ( */
		    Vect_append_point(nPoints, rx, ry, 0);
		}
	    }
	}
	else {
	    /* we should draw elliptical arc for outside corner */

	    /* inverse transforms */
	    elliptic_transform(wx, wy, 1 / da, 1 / db, dalpha, &wx1, &wy1);
	    elliptic_transform(vx, vy, 1 / da, 1 / db, dalpha, &vx1, &vy1);

	    phi1 = atan2(wy1, wx1);
	    phi2 = atan2(vy1, vx1);
	    delta_phi = side * (phi2 - phi1);

	    /* make delta_phi in [0, 2pi] */
	    if (delta_phi < 0)
		delta_phi += 2 * PI;

	    nsegments = (int)(delta_phi / angular_tol) + 1;
	    angular_step = side * (delta_phi / nsegments);

	    for (j = 0; j <= nsegments; j++) {
		elliptic_transform(cos(phi1), sin(phi1), da, db, dalpha, &tx,
				   &ty);
		Vect_append_point(nPoints, x[i] + tx, y[i] + ty, 0);
		phi1 += angular_step;
	    }
	}

	if ((!looped) && (i == np - 2)) {
	    Vect_append_point(nPoints, mx, my, 0);
	}
    }

    if (looped) {
	Vect_append_point(nPoints, nPoints->x[0], nPoints->y[0],
			  nPoints->z[0]);
    }

    Vect_line_prune(nPoints);

    if (looped) {
	Vect_line_delete_point(Points, Points->n_points - 1);
    }
}

/* input line must be looped */
static void convolution_line(struct line_pnts *Points, double da, double db,
			     double dalpha, int side, int round, int caps,
			     double tol, struct line_pnts *nPoints)
{
    int i, j, res, np;
    double *x, *y;
    double tx, ty, vx, vy, wx, wy, nx, ny, mx, my, rx, ry;
    double vx1, vy1, wx1, wy1;
    double a0, b0, c0, a1, b1, c1;
    double phi1, phi2, delta_phi;
    double nsegments, angular_tol, angular_step;
    double angle0, angle1;
    int inner_corner, turns360;

    G_debug(3, "convolution_line() side = %d", side);

    np = Points->n_points;
    x = Points->x;
    y = Points->y;
    if ((np == 0) || (np == 1))
	return;
    if ((x[0] != x[np - 1]) || (y[0] != y[np - 1])) {
	G_fatal_error(_("Line is not looped"));
	return;
    }

    Vect_reset_line(nPoints);

    if ((da == 0) || (db == 0)) {
	Vect_copy_xyz_to_pnts(nPoints, x, y, NULL, np);
	return;
    }

    side = (side >= 0) ? (1) : (-1);	/* normalize variable */
    dalpha *= PI / 180;		/* convert dalpha from degrees to radians */
    angular_tol = angular_tolerance(tol, da, db);

    i = np - 2;
    norm_vector(x[i], y[i], x[i + 1], y[i + 1], &tx, &ty);
    elliptic_tangent(side * tx, side * ty, da, db, dalpha, &vx, &vy);
    angle1 = atan2(ty, tx);
    nx = x[i] + vx;
    ny = y[i] + vy;
    mx = x[i + 1] + vx;
    my = y[i + 1] + vy;
    if (!round)
	line_coefficients(nx, ny, mx, my, &a1, &b1, &c1);

    for (i = 0; i <= np - 2; i++) {
	G_debug(4, "point %d, segment %d-%d", i, i, i + 1);
	/* save the old values */
	if (!round) {
	    a0 = a1;
	    b0 = b1;
	    c0 = c1;
	}
	wx = vx;
	wy = vy;
	angle0 = angle1;

	norm_vector(x[i], y[i], x[i + 1], y[i + 1], &tx, &ty);
	if ((tx == 0) && (ty == 0))
	    continue;
	elliptic_tangent(side * tx, side * ty, da, db, dalpha, &vx, &vy);
	angle1 = atan2(ty, tx);
	nx = x[i] + vx;
	ny = y[i] + vy;
	mx = x[i + 1] + vx;
	my = y[i + 1] + vy;
	if (!round)
	    line_coefficients(nx, ny, mx, my, &a1, &b1, &c1);


	delta_phi = angle1 - angle0;
	if (delta_phi > PI)
	    delta_phi -= 2 * PI;
	else if (delta_phi <= -PI)
	    delta_phi += 2 * PI;
	/* now delta_phi is in [-pi;pi] */
	turns360 = (fabs(fabs(delta_phi) - PI) < 1e-15);
	inner_corner = (side * delta_phi <= 0) && (!turns360);


	/* if <line turns 360> and (<caps> and <not round>) */
	if (turns360 && caps && (!round)) {
	    norm_vector(0, 0, vx, vy, &tx, &ty);
	    elliptic_tangent(side * tx, side * ty, da, db, dalpha, &tx, &ty);
	    Vect_append_point(nPoints, x[i] + wx + tx, y[i] + wy + ty, 0);
	    G_debug(4, " append point (c) x=%.16f y=%.16f", x[i] + wx + tx,
		    y[i] + wy + ty);
	    Vect_append_point(nPoints, nx + tx, ny + ty, 0);	/* nx == x[i] + vx, ny == y[i] + vy */
	    G_debug(4, " append point (c) x=%.16f y=%.16f", nx + tx, ny + ty);
	}

	if ((!turns360) && (!round) && (!inner_corner)) {
	    res = line_intersection(a0, b0, c0, a1, b1, c1, &rx, &ry);
	    if (res == 1) {
		Vect_append_point(nPoints, rx, ry, 0);
		G_debug(4, " append point (o) x=%.16f y=%.16f", rx, ry);
	    }
	    else if (res == 2) {
		/* no need to append point in this case */
	    }
	    else
		G_fatal_error(_("Unexpected result of line_intersection() res = %d"),
			      res);
	}

	if (round && (!inner_corner) && (!turns360 || caps)) {
	    /* we should draw elliptical arc for outside corner */

	    /* inverse transforms */
	    elliptic_transform(wx, wy, 1 / da, 1 / db, dalpha, &wx1, &wy1);
	    elliptic_transform(vx, vy, 1 / da, 1 / db, dalpha, &vx1, &vy1);

	    phi1 = atan2(wy1, wx1);
	    phi2 = atan2(vy1, vx1);
	    delta_phi = side * (phi2 - phi1);

	    /* make delta_phi in [0, 2pi] */
	    if (delta_phi < 0)
		delta_phi += 2 * PI;

	    nsegments = (int)(delta_phi / angular_tol) + 1;
	    angular_step = side * (delta_phi / nsegments);

	    phi1 += angular_step;
	    for (j = 1; j <= nsegments - 1; j++) {
		elliptic_transform(cos(phi1), sin(phi1), da, db, dalpha, &tx,
				   &ty);
		Vect_append_point(nPoints, x[i] + tx, y[i] + ty, 0);
		G_debug(4, " append point (r) x=%.16f y=%.16f", x[i] + tx,
			y[i] + ty);
		phi1 += angular_step;
	    }
	}

	Vect_append_point(nPoints, nx, ny, 0);
	G_debug(4, " append point (s) x=%.16f y=%.16f", nx, ny);
	Vect_append_point(nPoints, mx, my, 0);
	G_debug(4, " append point (s) x=%.16f y=%.16f", mx, my);
    }

    /* close the output line */
    Vect_append_point(nPoints, nPoints->x[0], nPoints->y[0], nPoints->z[0]);
    Vect_line_prune ( nPoints );
}

/*
 * side: side >= 0 - extracts contour on right side of edge, side < 0 - extracts contour on left side of edge
 * if the extracted contour is the outer contour, it is returned in ccw order
 * else if it is inner contour, it is returned in cw order
 */
static void extract_contour(struct planar_graph *pg, struct pg_edge *first,
			    int side, int winding, int stop_at_line_end,
			    struct line_pnts *nPoints)
{
    int j;
    int v;			/* current vertex number */
    int v0;
    int eside;			/* side of the current edge */
    double eangle;		/* current edge angle with Ox (according to the current direction) */
    struct pg_vertex *vert;	/* current vertex */
    struct pg_vertex *vert0;	/* last vertex */
    struct pg_edge *edge;	/* current edge; must be edge of vert */

    /*    int cs; *//* on which side are we turning along the contour */
    /* we will always turn right and don't need that one */
    double opt_angle, tangle;
    int opt_j, opt_side, opt_flag;

    G_debug(3, "extract_contour(): v1=%d, v2=%d, side=%d, stop_at_line_end=%d",
	    first->v1, first->v2, side, stop_at_line_end);

    Vect_reset_line(nPoints);

    edge = first;
    if (side >= 0) {
	eside = 1;
	v0 = edge->v1;
	v = edge->v2;
    }
    else {
	eside = -1;
	v0 = edge->v2;
	v = edge->v1;
    }
    vert0 = &(pg->v[v0]);
    vert = &(pg->v[v]);
    eangle = atan2(vert->y - vert0->y, vert->x - vert0->x);

    while (1) {
	Vect_append_point(nPoints, vert0->x, vert0->y, 0);
	G_debug(4, "ec: v0=%d, v=%d, eside=%d, edge->v1=%d, edge->v2=%d", v0,
		v, eside, edge->v1, edge->v2);
	G_debug(4, "ec: append point x=%.18f y=%.18f", vert0->x, vert0->y);

	/* mark current edge as visited on the appropriate side */
	if (eside == 1) {
	    edge->visited_right = 1;
	    edge->winding_right = winding;
	}
	else {
	    edge->visited_left = 1;
	    edge->winding_left = winding;
	}

	opt_flag = 1;
	for (j = 0; j < vert->ecount; j++) {
	    /* exclude current edge */
	    if (vert->edges[j] != edge) {
		tangle = vert->angles[j] - eangle;
		if (tangle < -PI)
		    tangle += 2 * PI;
		else if (tangle > PI)
		    tangle -= 2 * PI;
		/* now tangle is in (-PI, PI) */

		if (opt_flag || (tangle < opt_angle)) {
		    opt_j = j;
		    opt_side = (vert->edges[j]->v1 == v) ? (1) : (-1);
		    opt_angle = tangle;
		    opt_flag = 0;
		}
	    }
	}

	/* 
	G_debug(4, "ec: opt: side=%d opt_flag=%d opt_angle=%.18f opt_j=%d opt_step=%d",
	        side, opt_flag, opt_angle, opt_j, opt_step);
	*/

	/* if line end is reached (no other edges at curr vertex) */
	if (opt_flag) {
	    if (stop_at_line_end) {
		G_debug(3, "    end has been reached, will stop here");
		break;
	    }
	    else {
		opt_j = 0;	/* the only edge of vert is vert->edges[0] */
		opt_side = -eside;	/* go to the other side of the current edge */
		G_debug(3, "    end has been reached, turning around");
	    }
	}

	/* break condition */
	if ((vert->edges[opt_j] == first) && (opt_side == side))
	    break;
	if (opt_side == 1) {
	    if (vert->edges[opt_j]->visited_right) {
		G_warning(_("Next edge was visited (right) but it is not the first one !!! breaking loop"));
		G_debug(4,
			"ec: v0=%d, v=%d, eside=%d, edge->v1=%d, edge->v2=%d",
			v, (edge->v1 == v) ? (edge->v2) : (edge->v1),
			opt_side, vert->edges[opt_j]->v1,
			vert->edges[opt_j]->v2);
		break;
	    }
	}
	else {
	    if (vert->edges[opt_j]->visited_left) {
		G_warning(_("Next edge was visited (left) but it is not the first one !!! breaking loop"));
		G_debug(4,
			"ec: v0=%d, v=%d, eside=%d, edge->v1=%d, edge->v2=%d",
			v, (edge->v1 == v) ? (edge->v2) : (edge->v1),
			opt_side, vert->edges[opt_j]->v1,
			vert->edges[opt_j]->v2);
		break;
	    }
	}

	edge = vert->edges[opt_j];
	eside = opt_side;
	v0 = v;
	v = (edge->v1 == v) ? (edge->v2) : (edge->v1);
	vert0 = vert;
	vert = &(pg->v[v]);
	eangle = vert0->angles[opt_j];
    }
    Vect_append_point(nPoints, vert->x, vert->y, 0);
    Vect_line_prune(nPoints);
    G_debug(4, "ec: append point x=%.18f y=%.18f", vert->x, vert->y);

    return;
}

/*
 * This function extracts the outer contour of a (self crossing) line.
 * It can generate left/right contour if none of the line ends are in a loop.
 * If one or both of them is in a loop, then there's only one contour
 * 
 * side: side > 0 - right contour, side < 0 - left contour, side = 0 - outer contour
 *       if side != 0 and there's only one contour, the function returns it
 * 
 * TODO: Implement side != 0 feature;
 */
static void extract_outer_contour(struct planar_graph *pg, int side,
				  struct line_pnts *nPoints)
{
    int i;
    int flag;
    int v;
    struct pg_vertex *vert;
    struct pg_edge *edge;
    double min_x, min_angle;

    G_debug(3, "extract_outer_contour()");

    if (side != 0) {
	G_fatal_error(_("side != 0 feature not implemented"));
	return;
    }

    /* find a line segment which is on the outer contour */
    flag = 1;
    for (i = 0; i < pg->vcount; i++) {
	if (flag || (pg->v[i].x < min_x)) {
	    v = i;
	    min_x = pg->v[i].x;
	    flag = 0;
	}
    }
    vert = &(pg->v[v]);

    flag = 1;
    for (i = 0; i < vert->ecount; i++) {
	if (flag || (vert->angles[i] < min_angle)) {
	    edge = vert->edges[i];
	    min_angle = vert->angles[i];
	    flag = 0;
	}
    }

    /* the winding on the outer contour is 0 */
    extract_contour(pg, edge, (edge->v1 == v) ? RIGHT_SIDE : LEFT_SIDE, 0, 0,
		    nPoints);

    return;
}

/*
 * Extracts contours which are not visited.
 * IMPORTANT: the outer contour must be visited (you should call extract_outer_contour() to do that),
 *            so that extract_inner_contour() doesn't return it
 *
 * returns: 0 when there are no more inner contours; otherwise, 1
 */
static int extract_inner_contour(struct planar_graph *pg, int *winding,
				 struct line_pnts *nPoints)
{
    int i, w;
    struct pg_edge *edge;

    G_debug(3, "extract_inner_contour()");

    for (i = 0; i < pg->ecount; i++) {
	edge = &(pg->e[i]);
	if (edge->visited_left) {
	    if (!(pg->e[i].visited_right)) {
		w = edge->winding_left - 1;
		extract_contour(pg, &(pg->e[i]), RIGHT_SIDE, w, 0, nPoints);
		*winding = w;
		return 1;
	    }
	}
	else {
	    if (pg->e[i].visited_right) {
		w = edge->winding_right + 1;
		extract_contour(pg, &(pg->e[i]), LEFT_SIDE, w, 0, nPoints);
		*winding = w;
		return 1;
	    }
	}
    }

    return 0;
}

/* point_in_buf - test if point px,py is in d buffer of Points
 ** dalpha is in degrees
 ** returns:  1 in buffer
 **           0 not in buffer
 */
static int point_in_buf(struct line_pnts *Points, double px, double py, double da,
			double db, double dalpha)
{
    int i, np;
    double cx, cy;
    double delta, delta_k, k;
    double vx, vy, wx, wy, mx, my, nx, ny;
    double len, tx, ty, d, da2;

    G_debug(3, "point_in_buf()");

    dalpha *= PI / 180;		/* convert dalpha from degrees to radians */

    np = Points->n_points;
    da2 = da * da;
    for (i = 0; i < np - 1; i++) {
	vx = Points->x[i];
	vy = Points->y[i];
	wx = Points->x[i + 1];
	wy = Points->y[i + 1];

	if (da != db) {
	    mx = wx - vx;
	    my = wy - vy;
	    len = LENGTH(mx, my);
	    elliptic_tangent(mx / len, my / len, da, db, dalpha, &cx, &cy);

	    delta = mx * cy - my * cx;
	    delta_k = (px - vx) * cy - (py - vy) * cx;
	    k = delta_k / delta;
	    /*            G_debug(4, "k = %g, k1 = %g", k, (mx * (px - vx) + my * (py - vy)) / (mx * mx + my * my)); */
	    if (k <= 0) {
		nx = vx;
		ny = vy;
	    }
	    else if (k >= 1) {
		nx = wx;
		ny = wy;
	    }
	    else {
		nx = vx + k * mx;
		ny = vy + k * my;
	    }

	    /* inverse transform */
	    elliptic_transform(px - nx, py - ny, 1 / da, 1 / db, dalpha, &tx,
			       &ty);

	    d = dig_distance2_point_to_line(nx + tx, ny + ty, 0, vx, vy, 0,
					    wx, wy, 0, 0, NULL, NULL, NULL,
					    NULL, NULL);

	    /*            G_debug(4, "sqrt(d)*da = %g, len' = %g, olen = %g", sqrt(d)*da, da*LENGTH(tx,ty), LENGTH((px-nx),(py-ny))); */
	    if (d <= 1) {
		/* G_debug(1, "d=%g", d); */
		return 1;
	    }
	}
	else {
	    d = dig_distance2_point_to_line(px, py, 0, vx, vy, 0, wx, wy, 0,
					    0, NULL, NULL, NULL, NULL, NULL);
	    /*            G_debug(4, "sqrt(d)     = %g", sqrt(d)); */
	    if (d <= da2) {
		return 1;
	    }
	}
    }

    return 0;
}

/* returns 0 for ccw, non-zero for cw
 */
static int get_polygon_orientation(const double *x, const double *y, int n)
{
    double x1, y1, x2, y2;
    double area;

    x2 = x[n - 1];
    y2 = y[n - 1];

    area = 0;
    while (--n >= 0) {
	x1 = x2;
	y1 = y2;

	x2 = *x++;
	y2 = *y++;

	area += (y2 + y1) * (x2 - x1);
    }

    return (area > 0);
}

/* internal */
static void add_line_to_array(struct line_pnts *Points,
			      struct line_pnts ***arrPoints, int *count,
			      int *allocated, int more)
{
    if (*allocated == *count) {
	*allocated += more;
	*arrPoints =
	    G_realloc(*arrPoints, (*allocated) * sizeof(struct line_pnts *));
    }
    (*arrPoints)[*count] = Points;
    (*count)++;

    return;
}

static void destroy_lines_array(struct line_pnts **arr, int count)
{
    int i;

    for (i = 0; i < count; i++)
	Vect_destroy_line_struct(arr[i]);
    G_free(arr);
}

/* area_outer and area_isles[i] must be closed non self-intersecting lines
   side: 0 - auto, 1 - right, -1 left
 */
static void buffer_lines(struct line_pnts *area_outer, struct line_pnts **area_isles,
			 int isles_count, int side, double da, double db,
			 double dalpha, int round, int caps, double tol,
			 struct line_pnts **oPoints, struct line_pnts ***iPoints,
			 int *inner_count)
{
    struct planar_graph *pg2;
    struct line_pnts *sPoints, *cPoints;
    struct line_pnts **arrPoints;
    int i, count = 0;
    int res, winding;
    int auto_side;
    int more = 8;
    int allocated = 0;
    double px, py;

    G_debug(3, "buffer_lines()");

    auto_side = (side == 0);

    /* initializations */
    sPoints = Vect_new_line_struct();
    cPoints = Vect_new_line_struct();
    arrPoints = NULL;

    /* outer contour */
    G_debug(3, "    processing outer contour");
    *oPoints = Vect_new_line_struct();
    if (auto_side)
	side =
	    get_polygon_orientation(area_outer->x, area_outer->y,
				    area_outer->n_points -
				    1) ? LEFT_SIDE : RIGHT_SIDE;
    convolution_line(area_outer, da, db, dalpha, side, round, caps, tol,
		     sPoints);
    pg2 = pg_create(sPoints);
    extract_outer_contour(pg2, 0, *oPoints);
    res = extract_inner_contour(pg2, &winding, cPoints);
    while (res != 0) {
	if (winding == 0) {
	    int check_poly = 1;
	    double area_size;

	    dig_find_area_poly(cPoints, &area_size);
	    if (area_size == 0) {
		G_warning(_("zero area size"));
		check_poly = 0;
	    }
	    if (cPoints->x[0] != cPoints->x[cPoints->n_points - 1] ||
		cPoints->y[0] != cPoints->y[cPoints->n_points - 1]) {

		G_warning(_("Line was not closed"));
		check_poly = 0;
	    }

	    if (check_poly && !Vect_point_in_poly(cPoints->x[0], cPoints->y[0], area_outer)) {
		if (Vect_get_point_in_poly(cPoints, &px, &py) == 0) {
		    if (!point_in_buf(area_outer, px, py, da, db, dalpha)) {
			add_line_to_array(cPoints, &arrPoints, &count, &allocated,
					  more);
			cPoints = Vect_new_line_struct();
		    }
		}
		else {
		    G_warning(_("Vect_get_point_in_poly() failed"));
		}
	    }
	}
	res = extract_inner_contour(pg2, &winding, cPoints);
    }
    pg_destroy_struct(pg2);

    /* inner contours */
    G_debug(3, "    processing inner contours");
    for (i = 0; i < isles_count; i++) {
	if (auto_side)
	    side =
		get_polygon_orientation(area_isles[i]->x, area_isles[i]->y,
					area_isles[i]->n_points -
					1) ? RIGHT_SIDE : LEFT_SIDE;
	convolution_line(area_isles[i], da, db, dalpha, side, round, caps,
			 tol, sPoints);
	pg2 = pg_create(sPoints);
	extract_outer_contour(pg2, 0, cPoints);
	res = extract_inner_contour(pg2, &winding, cPoints);
	while (res != 0) {
	    if (winding == -1) {
		int check_poly = 1;
		double area_size;

		dig_find_area_poly(cPoints, &area_size);
		if (area_size == 0) {
		    G_warning(_("zero area size"));
		    check_poly = 0;
		}
		if (cPoints->x[0] != cPoints->x[cPoints->n_points - 1] ||
		    cPoints->y[0] != cPoints->y[cPoints->n_points - 1]) {

		    G_warning(_("Line was not closed"));
		    check_poly = 0;
		}

		/* we need to check if the area is in the buffer.
		   I've simplfied convolution_line(), so that it runs faster,
		   however that leads to occasional problems */
		if (check_poly && Vect_point_in_poly
		    (cPoints->x[0], cPoints->y[0], area_isles[i])) {
		    if (Vect_get_point_in_poly(cPoints, &px, &py) == 0) {
			if (!point_in_buf(area_isles[i], px, py, da, db, dalpha)) {
			    add_line_to_array(cPoints, &arrPoints, &count,
					      &allocated, more);
			    cPoints = Vect_new_line_struct();
			}
		    }
		    else {
			G_warning(_("Vect_get_point_in_poly() failed"));
		    }
		}
	    }
	    res = extract_inner_contour(pg2, &winding, cPoints);
	}
	pg_destroy_struct(pg2);
    }

    arrPoints = G_realloc(arrPoints, count * sizeof(struct line_pnts *));
    *inner_count = count;
    *iPoints = arrPoints;

    Vect_destroy_line_struct(sPoints);
    Vect_destroy_line_struct(cPoints);

    G_debug(3, "buffer_lines() ... done");

    return;
}


/*!
   \brief Creates buffer around line.

   See also Vect_line_buffer().
   
   Shape of buffer endings is managed by two parameters - round and cap.
   Setting round=1, cap=1 gives "classical" buffer, while
   round=0, cap=1 gives square end, but cap=0 – butt.
   See v.buffer manual or SVG stroke-linecap for examples.
   
   To get "classical" buffer, set db equal to da, and dalpha to 0.

   \param Points input line geometry
   \param da distance along major axis
   \param db distance along minor axis
   \param dalpha angle between 0x and major axis
   \param round make corners round (0 - square, not 0 - round)
   \param caps add caps at line ends (0 - butt, not 0 - caps)
   \param tol maximum distance between theoretical arc and output segments
   \param[out] oPoints output polygon outer border (ccw order)
   \param[out] iPoints array of output polygon's holes (cw order)
   \param[out] inner_count number of holes
 */
void Vect_line_buffer2(const struct line_pnts *Points, double da, double db,
		       double dalpha, int round, int caps, double tol,
		       struct line_pnts **oPoints,
		       struct line_pnts ***iPoints, int *inner_count)
{
    struct planar_graph *pg;
    struct line_pnts *tPoints, *outer;
    struct line_pnts **isles;
    int isles_count = 0;
    int res, winding;
    int more = 8;
    int isles_allocated = 0;

    G_debug(2, "Vect_line_buffer()");

    Vect_line_prune((struct line_pnts *)Points);

    if (Points->n_points == 1)
	return Vect_point_buffer2(Points->x[0], Points->y[0], da, db,
			dalpha, round, tol, oPoints);

    /* initializations */
    tPoints = Vect_new_line_struct();
    isles = NULL;
    pg = pg_create(Points);

    /* outer contour */
    outer = Vect_new_line_struct();
    extract_outer_contour(pg, 0, outer);

    /* inner contours */
    res = extract_inner_contour(pg, &winding, tPoints);
    while (res != 0) {
	add_line_to_array(tPoints, &isles, &isles_count, &isles_allocated,
			  more);
	tPoints = Vect_new_line_struct();
	res = extract_inner_contour(pg, &winding, tPoints);
    }

    buffer_lines(outer, isles, isles_count, RIGHT_SIDE, da, db, dalpha, round,
		 caps, tol, oPoints, iPoints, inner_count);

    Vect_destroy_line_struct(tPoints);
    Vect_destroy_line_struct(outer);
    destroy_lines_array(isles, isles_count);
    pg_destroy_struct(pg);

    return;
}

/*!
   \brief Creates buffer around area.

   \param Map vector map
   \param area area id
   \param da distance along major axis
   \param db distance along minor axis
   \param dalpha angle between 0x and major axis
   \param round make corners round
   \param caps add caps at line ends
   \param tol maximum distance between theoretical arc and output segments
   \param[out] oPoints output polygon outer border (ccw order)
   \param[out] inner_count number of holes
   \param[out] iPoints array of output polygon's holes (cw order)
 */
void Vect_area_buffer2(const struct Map_info *Map, int area, double da, double db,
		       double dalpha, int round, int caps, double tol,
		       struct line_pnts **oPoints,
		       struct line_pnts ***iPoints, int *inner_count)
{
    struct line_pnts *tPoints, *outer;
    struct line_pnts **isles;
    int isles_count = 0, n_isles;
    int i, isle;
    int more = 8;
    int isles_allocated = 0;

    G_debug(2, "Vect_area_buffer()");

    /* initializations */
    tPoints = Vect_new_line_struct();
    n_isles = Vect_get_area_num_isles(Map, area);
    isles_allocated = n_isles;
    isles = G_malloc(isles_allocated * sizeof(struct line_pnts *));

    /* outer contour */
    outer = Vect_new_line_struct();
    Vect_get_area_points(Map, area, outer);
    /* does not work with zero length line segments */
    Vect_line_prune(outer);

    /* inner contours */
    for (i = 0; i < n_isles; i++) {
	isle = Vect_get_area_isle(Map, area, i);
	Vect_get_isle_points(Map, isle, tPoints);

	/* Check if the isle is big enough */
	/*
	   if (Vect_line_length(tPoints) < 2*PI*max)
	   continue;
	 */
	/* does not work with zero length line segments */
	Vect_line_prune(tPoints);
	add_line_to_array(tPoints, &isles, &isles_count, &isles_allocated,
			  more);
	tPoints = Vect_new_line_struct();
    }

    buffer_lines(outer, isles, isles_count, 0, da, db, dalpha, round, caps,
		 tol, oPoints, iPoints, inner_count);

    Vect_destroy_line_struct(tPoints);
    Vect_destroy_line_struct(outer);
    destroy_lines_array(isles, isles_count);

    return;
}

/*!
   \brief Creates buffer around the point (px, py).

   \param px input point x-coordinate
   \param py input point y-coordinate
   \param da distance along major axis
   \param db distance along minor axis
   \param dalpha angle between 0x and major axis
   \param round make corners round
   \param tol maximum distance between theoretical arc and output segments
   \param[out] oPoints output polygon outer border (ccw order)
 */
void Vect_point_buffer2(double px, double py, double da, double db,
			double dalpha, int round, double tol,
			struct line_pnts **oPoints)
{
    double tx, ty;
    double angular_tol, angular_step, phi1;
    int j, nsegments;

    G_debug(2, "Vect_point_buffer()");

    *oPoints = Vect_new_line_struct();

    dalpha *= PI / 180;		/* convert dalpha from degrees to radians */

    if (round || (!round)) {
	angular_tol = angular_tolerance(tol, da, db);

	nsegments = (int)(2 * PI / angular_tol) + 1;
	angular_step = 2 * PI / nsegments;

	phi1 = 0;
	for (j = 0; j < nsegments; j++) {
	    elliptic_transform(cos(phi1), sin(phi1), da, db, dalpha, &tx,
			       &ty);
	    Vect_append_point(*oPoints, px + tx, py + ty, 0);
	    phi1 += angular_step;
	}
    }
    else {

    }

    /* close the output line */
    Vect_append_point(*oPoints, (*oPoints)->x[0], (*oPoints)->y[0],
		      (*oPoints)->z[0]);

    return;
}


/*
   \brief Create parallel line

   See also Vect_line_parallel().
   
   \param InPoints input line geometry
   \param da distance along major axis
   \param da distance along minor axis
   \param dalpha angle between 0x and major axis
   \param round make corners round
   \param tol maximum distance between theoretical arc and output segments
   \param[out] OutPoints output line
 */
void Vect_line_parallel2(struct line_pnts *InPoints, double da, double db,
			 double dalpha, int side, int round, double tol,
			 struct line_pnts *OutPoints)
{
    G_debug(2, "Vect_line_parallel(): npoints = %d, da = %f, "
	    "db = %f, dalpha = %f, side = %d, round_corners = %d, tol = %f",
	    InPoints->n_points, da, db, dalpha, side, round, tol);

    parallel_line(InPoints, da, db, dalpha, side, round, 1, NON_LOOPED_LINE,
		  tol, OutPoints);

    /*    if (!loops)
       clean_parallel(OutPoints, InPoints, distance, rm_end);
     */
    
    return;
}
