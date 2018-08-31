
/****************************************************************
 *
 * MODULE:     v.generalize
 *
 * AUTHOR(S):  Daniel Bundala
 *
 * PURPOSE:    Module for line simplification and smoothing
 *
 * COPYRIGHT:  (C) 2002-2005 by the GRASS Development Team
 *
 *             This program is free software under the
 *             GNU General Public License (>=v2).
 *             Read the file COPYING that comes with GRASS
 *             for details.
 *
 ****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>
#include "point.h"
#include "matrix.h"

/* TODO: add loop support where possible */

/* boyle's forward looking algorithm
 * return the number of points in the result = Points->n_points
 */
int boyle(struct line_pnts *Points, int look_ahead, int loop_support, int with_z)
{
    POINT last, ppoint;
    POINT *res;
    int next, n, i, p;
    double c1, c2;
    int is_loop, count;

    n = Points->n_points;

    /* if look_ahead is too small or line too short, there's nothing
     * to smooth */
    if (look_ahead < 2 || look_ahead >= n) {
	return n;
    }

    count = n - 2;
    is_loop = 0;

    /* is it loop ? */
    if (Points->x[0] == Points->x[n - 1] &&
        Points->y[0] == Points->y[n - 1] &&
        (Points->z[0] == Points->z[n - 1] || with_z == 0) && loop_support){
        is_loop = 1;
	count = n;
    }

    res = G_malloc(sizeof(POINT) * n);

    point_assign(Points, 0, with_z, &last, 0);
    res[0] = last;
    c1 = (double)1 / (double)(look_ahead - 1);
    c2 = (double)1 - c1;
    next = 1;

    for (i = 0; i < count; i++) {
	p = i + look_ahead;
	if (!is_loop && p >= n)
	    p = n - 1;
	point_assign(Points, p, with_z, &ppoint, is_loop);
	point_scalar(ppoint, c1, &ppoint);
	point_scalar(last, c2, &last);
	point_add(last, ppoint, &res[next]);
	/* original: use smoothed point as new last point */
	/* last = res[next]; */
	next++;
	if (is_loop) {
	    while (next >= n - 1)
		next -= n - 1;
	}
	/* new: smooth with original points */
	point_assign(Points, p, with_z, &last, is_loop);
    }

    for (i = 1; i < n - 1; i++) {
	Points->x[i] = res[i].x;
	Points->y[i] = res[i].y;
	Points->z[i] = res[i].z;
    }
    if (is_loop) {
	Points->x[0] = res[0].x;
	Points->y[0] = res[0].y;
	Points->z[0] = res[0].z;

	Points->x[n - 1] = res[0].x;
	Points->y[n - 1] = res[0].y;
	Points->z[n - 1] = res[0].z;
    }
    
    G_free(res);

    return Points->n_points;
}

/* mcmaster's sliding averaging algorithm. Return the number of points
 * in the output line. This equals to the number of points in the
 * input line */

int sliding_averaging(struct line_pnts *Points, double slide, int look_ahead, 
                      int loop_support, int with_z)
{
    int n, half, i;
    double sc;
    POINT p, tmp, s;
    POINT *res;
    int is_loop, count;

    is_loop = 0;
    n = Points->n_points;
    half = look_ahead / 2;
    
    count = n - half;

    /* is it loop ? */
    if (Points->x[0] == Points->x[n - 1] &&
        Points->y[0] == Points->y[n - 1] && 
        (Points->z[0] == Points->z[n - 1] || with_z == 0) && loop_support){
        is_loop = 1;
        count = n + half;
    }
     
    if (look_ahead % 2 == 0) {
	G_fatal_error(_("Look ahead parameter must be odd"));
	return n;
    }

    if (look_ahead >= n || look_ahead < 2)
	return n;

    res = G_malloc(sizeof(POINT) * (n + half));
    if (!res) {
	G_fatal_error(_("Out of memory"));
	return n;
    }

    sc = (double)1.0 / (double)look_ahead;

    point_assign(Points, 0, with_z, &p, 0);
    for (i = 1; i < look_ahead; i++) {
	point_assign(Points, i, with_z, &tmp, 0);
	point_add(p, tmp, &p);
    }

    /* and calculate the average of remaining points */
    for (i = half; i < count; i++) {
	point_assign(Points, i, with_z, &s, is_loop);
	point_scalar(s, 1.0 - slide, &s);
	point_scalar(p, sc * slide, &tmp);
	point_add(tmp, s, &res[i]);
	if ((i + half + 1 < n) || is_loop) {
	    point_assign(Points, i - half, with_z, &tmp, is_loop);
	    point_subtract(p, tmp, &p);
	    point_assign(Points, i + half + 1, with_z, &tmp, is_loop);
	    point_add(p, tmp, &p);
	}
    }


    if (is_loop) {
        for (i = 0; i < half; i++) {
	    Points->x[i] = res[n + i - 1].x;
	    Points->y[i] = res[n + i - 1].y;
	    Points->z[i] = res[n + i - 1].z;
        }
	for (i = half; i < n; i++) {
	    Points->x[i] = res[i].x;
	    Points->y[i] = res[i].y;
	    Points->z[i] = res[i].z;
	}
    }
    else {
	for (i = half; i < n - half; i++) {
	    Points->x[i] = res[i].x;
	    Points->y[i] = res[i].y;
	    Points->z[i] = res[i].z;
	}
    }

    G_free(res);

    return Points->n_points;
}

/* mcmaster's distance weighting algorithm. Return the number
 * of points in the output line which equals to Points->n_points */
int distance_weighting(struct line_pnts *Points, double slide, int look_ahead,
		       int loop_support, int with_z)
{
    POINT p, c, s, tmp;
    int n, i, half, j;
    double dists, d;
    POINT *res;
    int is_loop, count;

    n = Points->n_points;

    if (look_ahead >= n || look_ahead < 1)
	return n;

    half = look_ahead / 2;
    count = n - half;

    /* is it loop ? */
    is_loop = 0;
    if (Points->x[0] == Points->x[n - 1] &&
        Points->y[0] == Points->y[n - 1] &&
        (Points->z[0] == Points->z[n - 1] || with_z == 0) && loop_support){
        is_loop = 1;
        count = n + half - 1;
    }

    if ((look_ahead & 1) == 0) {
	G_fatal_error(_("Look ahead parameter must be odd"));
	return n;
    }

    res = (POINT *) G_malloc(sizeof(POINT) * (n + half));
    if (!res) {
	G_fatal_error(_("Out of memory"));
	return n;
    }

    point_assign(Points, 0, with_z, &res[0], 0);

    for (i = half; i < count; i++) {
	point_assign(Points, i, with_z, &c, is_loop);
	s.x = s.y = s.z = 0;
	dists = 0;

	for (j = i - half; j <= i + half; j++) {
	    if (j == i)
		continue;
	    point_assign(Points, j, with_z, &p, is_loop);
	    d = point_dist(p, c);
	    if (d < GRASS_EPSILON)
		continue;
	    d = (double)1.0 / d;
	    dists += d;
	    point_scalar(p, d, &tmp);
	    s.x += tmp.x;
	    s.y += tmp.y;
	    s.z += tmp.z;
	}
	if (dists < GRASS_EPSILON) {
	    point_add(c, s, &res[i]);
	}
	else {
	    point_scalar(s, slide / dists, &tmp);
	    point_scalar(c, (double)1.0 - slide, &s);
	    point_add(s, tmp, &res[i]);
	}
    }

    if (is_loop) {
        for (i = 0; i < half; i++) {
	    Points->x[i] = res[n + i - 1].x;
	    Points->y[i] = res[n + i - 1].y;
	    Points->z[i] = res[n + i - 1].z;
        }
	for (i = half; i < n; i++) {
	    Points->x[i] = res[i].x;
	    Points->y[i] = res[i].y;
	    Points->z[i] = res[i].z;
	}
    }
    else {
	for (i = half; i < n - half; i++) {
	    Points->x[i] = res[i].x;
	    Points->y[i] = res[i].y;
	    Points->z[i] = res[i].z;
	}
    }

    G_free(res);

    return Points->n_points;
}


/* Chaiken's algorithm. Return the number of points in smoothed line 
 */
int chaiken(struct line_pnts *Points, double thresh, int loop_support, int with_z)
{

    int n, i;
    POINT_LIST head, *cur;
    POINT p0, p1, p2, m1, tmp;
    int is_loop;

    n = Points->n_points;

    /* line is too short */
    if (n < 3)
	return n;

    is_loop = 0;
    /* is it loop ? */
    if (Points->x[0] == Points->x[n - 1] &&
        Points->y[0] == Points->y[n - 1] && 
        (Points->z[0] == Points->z[n - 1] || with_z == 0) && loop_support){
        is_loop = 1;
    }

    thresh *= thresh;

    head.next = NULL;
    cur = &head;
    if (!is_loop) {
	/* always keep first point */
	point_assign(Points, 0, with_z, &p0, 0);
    }
    else {
	point_assign(Points, 0, with_z, &p1, 0);
	point_assign(Points, 1, with_z, &p2, 0);
	point_add(p1, p2, &tmp);
	point_scalar(tmp, 0.5, &p0);
    }

    point_list_add(cur, p0); 
    cur = cur->next;

    for (i = 2; i <= n; i++) {
	if (!is_loop) {
	    if (i == n)
		point_assign(Points, i - 1, with_z, &p2, 0);
	    else
		point_assign(Points, i, with_z, &p2, 0);
	}
	else
	    point_assign(Points, i, with_z, &p2, 1);
	point_assign(Points, i - 1, with_z, &p1, 0);

	while (1) {
	    point_add(p1, p2, &tmp);
	    point_scalar(tmp, 0.5, &m1);

	    point_list_add(cur, m1);

	    if (point_dist_square(p0, m1) > thresh) {
		point_add(p1, m1, &tmp);	/* need to refine the partition */
		point_scalar(tmp, 0.5, &p2);
		point_add(p1, p0, &tmp);
		point_scalar(tmp, 0.5, &p1);
	    }
	    else {
		break;		/* good approximation */
	    }
	}

	while (cur->next != NULL)
	    cur = cur->next;

	p0 = cur->p;
    }

    if (!is_loop) {
	point_assign(Points, n - 1, with_z, &p0, 0);
	point_list_add(cur, p0); /* always keep last point */
    }

    if (point_list_copy_to_line_pnts(head, Points) == -1) {
	G_fatal_error(_("Out of memory"));
    }
    point_list_free(head);

    return Points->n_points;
}


/* use for refining tangent in hermite interpolation */
void refine_tangent(POINT * p)
{
    double l = point_dist2(*p);

    if (l < GRASS_EPSILON) {
	point_scalar(*p, 0.0, p);
    }
    else {
	point_scalar(*p, (double)1.0 / sqrt(sqrt(sqrt(l))), p);
    }
    return;
}

/* approximates given line using hermite cubic spline
 * interpolates by steps of length step
 * returns the number of point in result
 */
int hermite(struct line_pnts *Points, double step, double angle_thresh,
	    int loop_support, int with_z)
{
    POINT_LIST head, *last, *point;
    POINT p0, p1, t0, t1, tmp, res;
    double length, next, length_begin, l;
    double s;
    double h1, h2, h3, h4;
    int n, i;
    int ni;
    int is_loop;

    n = Points->n_points;

    /* line is too short */
    if (n <= 2) {
	return n;
    }

    is_loop = 0;

    /* is it loop ? */
    if (Points->x[0] == Points->x[n - 1] &&
        Points->y[0] == Points->y[n - 1] && 
        (Points->z[0] == Points->z[n - 1] || with_z == 0) && loop_support){
        is_loop = 1;
    }

    /* convert degrees=>radians */
    angle_thresh *= M_PI / 180.0;

    head.next = NULL;
    point = last = &head;

    if (!is_loop) {
	point_assign(Points, 0, with_z, &p0, 0);
	point_assign(Points, 1, with_z, &p1, 0);
	/* length of line from point 0 to i+1 */
	length = point_dist(p0, p1);
	/* tangent at p0, p1 */
	point_subtract(p1, p0, &t0);
	refine_tangent(&t0);
	point_assign(Points, 2, with_z, &tmp, 0);
	point_subtract(tmp, p0, &t1);
	refine_tangent(&t1);
    }
    else {
	point_assign(Points, n - 2, with_z, &p0, 0);
	point_assign(Points, 0, with_z, &p1, 0);
	/* length of line from point n - 2 to 0 = n - 1 */
	length = point_dist(p0, p1);
	/* tangent at p0, p1 */
	point_assign(Points, 1, with_z, &tmp, 0);
	point_subtract(tmp, p0, &t0);
	refine_tangent(&t0);

	point_assign(Points, 0, with_z, &p0, 0);
	point_assign(Points, 1, with_z, &p1, 0);
	/* length of line from point 0 to 1 */
	length = point_dist(p0, p1);
	/* tangent at p0, p2 */
	point_assign(Points, 2, with_z, &tmp, 0);
	point_subtract(tmp, p0, &t1);
	refine_tangent(&t1);
    }

    /* length of line 0..i */
    length_begin = 0;
    next = 0;
    
    /* we always operate on the segment point[i]..point[i+1] */
    i = 0;
    while (i < n - 1) {
	if (next > length || (length - length_begin < GRASS_EPSILON)) {	/* segmet i..i+1 is finished or too short */
	    i++;
	    if (i >= n - 1)
		break;		/* we are already out of line */
	    point_assign(Points, i, with_z, &p0, is_loop);
	    point_assign(Points, i + 1, with_z, &p1, is_loop);
	    length_begin = length;
	    length += point_dist(p0, p1);
	    ni = i + 2;
	    if (!is_loop && ni > n - 1)
		ni = n - 1;	/* ensure that we are in the line */
	    t0 = t1;
	    point_assign(Points, ni, with_z, &tmp, is_loop);
	    point_subtract(tmp, p0, &t1);
	    refine_tangent(&t1);
	}
	else {
	    l = length - length_begin;	/* length of actual segment */
	    s = (next - length_begin) / l;	/* 0<=s<=1 where we want to add new point on the line */

	    /* then we need to calculate 4 control polynomials */
	    h1 = 2 * s * s * s - 3 * s * s + 1;
	    h2 = -2 * s * s * s + 3 * s * s;
	    h3 = s * s * s - 2 * s * s + s;
	    h4 = s * s * s - s * s;

	    point_scalar(p0, h1, &res);
	    point_scalar(p1, h2, &tmp);
	    point_add(res, tmp, &res);
	    point_scalar(t0, h3, &tmp);
	    point_add(res, tmp, &res);
	    point_scalar(t1, h4, &tmp);
	    point_add(res, tmp, &res);
	    point_list_add(last, res);
	    last = last->next;

	    next += step;
	}
	/* if the angle between 2 vectors is less then eps, remove the
	 * middle point */
	if (point->next && point->next->next && point->next->next->next) {
	    if (point_angle_between
		(point->next->p, point->next->next->p,
		 point->next->next->next->p) < angle_thresh) {
		point_list_delete_next(point->next);
	    }
	    else
		point = point->next;
	}
    }

    point_assign(Points, n - 1, with_z, &p0, 0);
    point_list_add(last, p0);

    if (point_list_copy_to_line_pnts(head, Points) == -1)
	G_fatal_error(_("Out of memory"));

    point_list_free(head);

    return Points->n_points;
}

/* snakes algorithm for line simplification/generalization 
 * returns the number of points in the output line. This is
 * always equal to the number of points in the original line
 * 
 * alpha, beta are 2 parameters which change the behaviour of the algorithm
 * 
 * TODO: Add parameter iterations, so the runnining time is O(N^3 * log iterations)
 * instead of O(N^3 * itearations). Probably not needed, for many iterations,
 * the result is almost straight line
 */
int snakes(struct line_pnts *Points, double alpha, double beta,
           int loop_support, int with_z)
{
    MATRIX g, ginv, xcoord, ycoord, zcoord, xout, yout, zout;

    int n = Points->n_points;
    int i, j;
    double x0, y0, z0;
    double a = 2.0 * alpha + 6.0 * beta;
    double b = -alpha - 4.0 * beta;
    double c = beta;
    double val[5] = { c, b, a, b, c };
    int plus = 4;
    int is_loop = 0;

    if (n < plus)
	return n;

    /* is it loop ? */
    if (Points->x[0] == Points->x[n - 1] &&
        Points->y[0] == Points->y[n - 1] && 
        (Points->z[0] == Points->z[n - 1] || with_z == 0) && loop_support){
        is_loop = 1;
	
	if (n < plus + 2)
	    return n;
    }

    if (!matrix_init(n + 2 * plus, n + 2 * plus, &g)) {
	G_fatal_error(_("Out of memory"));
	return n;
    }
    if (!matrix_init(n + 2 * plus, 1, &xcoord)) {
	G_fatal_error(_("Out of memory"));
	return n;
    }
    if (!matrix_init(n + 2 * plus, 1, &ycoord)) {
	G_fatal_error(_("Out of memory"));
	return n;
    }
    if (!matrix_init(n + 2 * plus, 1, &zcoord)) {
	G_fatal_error(_("Out of memory"));
	return n;
    }
    if (!matrix_init(n + 2 * plus, 1, &xout)) {
	G_fatal_error(_("Out of memory"));
	return n;
    }
    if (!matrix_init(n + 2 * plus, 1, &yout)) {
	G_fatal_error(_("Out of memory"));
	return n;
    }
    if (!matrix_init(n + 2 * plus, 1, &zout)) {
	G_fatal_error(_("Out of memory"));
	return n;
    }

    x0 = Points->x[0];
    y0 = Points->y[0];
    z0 = Points->z[0];

    /* store the coordinates in the column vectors */
    for (i = 0; i < n; i++) {
	xcoord.a[i + plus][0] = Points->x[i] - x0;
	ycoord.a[i + plus][0] = Points->y[i] - y0;
	zcoord.a[i + plus][0] = Points->z[i] - z0;
    }

    if (!is_loop) {
	/* repeat first and last point at the beginning and end
	 * of each vector respectively */
	for (i = 0; i < plus; i++) {
	    xcoord.a[i][0] = 0;
	    ycoord.a[i][0] = 0;
	    zcoord.a[i][0] = 0;
	}

	for (i = n + plus; i < n + 2 * plus; i++) {
	    xcoord.a[i][0] = Points->x[n - 1] - x0;
	    ycoord.a[i][0] = Points->y[n - 1] - y0;
	    zcoord.a[i][0] = Points->z[n - 1] - z0;
	}
    }
    else {
	/* loop: point 0 and point n - 1 are identical */

	/* repeat last and first points at the beginning and
	 * end of each vector respectively */

	/* add points from n - plus - 1 to n - 2 */
	for (i = 0, j = n - plus - 1; i < plus; i++, j++) {
	    xcoord.a[i][0] = Points->x[j] - x0;;
	    ycoord.a[i][0] = Points->y[j] - y0;;
	    zcoord.a[i][0] = Points->z[j] - z0;;
	}
	/* add points from 1 to plus + 1 */
	for (i = n + plus, j = 1; i < n + 2 * plus; i++, j++) {
	    xcoord.a[i][0] = Points->x[j] - x0;
	    ycoord.a[i][0] = Points->y[j] - y0;
	    zcoord.a[i][0] = Points->z[j] - z0;
	}
    }

    /* calculate the matrix */

    for (i = 0; i < n + 2 * plus; i++)
	for (j = 0; j < n + 2 * plus; j++) {
	    int index = j - i + 2;

	    if (index >= 0 && index <= 4)
		g.a[i][j] = val[index];
	    else
		g.a[i][j] = 0;
	}

    matrix_add_identity((double)1.0, &g);

    /* find its inverse */
    if (!matrix_inverse(&g, &ginv, 0)) {
	G_fatal_error(_("Unable to find the inverse matrix"));
	return n;
    }

    if (!matrix_mult(&ginv, &xcoord, &xout)
	|| !matrix_mult(&ginv, &ycoord, &yout)
	|| !matrix_mult(&ginv, &zcoord, &zout)) {
	G_fatal_error(_("Unable to calculate the output vectors"));
	return n;
    }

    if (!is_loop) {
	/* copy the new values of coordinates, but
	 * never move the last and first point */
	for (i = 1; i < n - 1; i++) {
	    Points->x[i] = xout.a[i + plus][0] + x0;
	    Points->y[i] = yout.a[i + plus][0] + y0;
	    if (with_z)
		Points->z[i] = zout.a[i + plus][0] + z0;
	}
    }
    else {
	/* copy the new values of coordinates,
	 * also move the last and first point */
	for (i = 0; i < n; i++) {
	    Points->x[i] = xout.a[i + plus][0] + x0;
	    Points->y[i] = yout.a[i + plus][0] + y0;
	    if (with_z)
		Points->z[i] = zout.a[i + plus][0] + z0;
	}

	Points->x[n - 1] = Points->x[0];
	Points->y[n - 1] = Points->y[0];
	Points->z[n - 1] = Points->z[0];
    }

    matrix_free(&g);
    matrix_free(&ginv);
    matrix_free(&xcoord);
    matrix_free(&ycoord);
    matrix_free(&zcoord);
    matrix_free(&xout);
    matrix_free(&yout);
    matrix_free(&zout);

    return Points->n_points;
}
