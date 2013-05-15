
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
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>
#include "point.h"
#include "pq.h"
#include "misc.h"

int douglas_peucker(struct line_pnts *Points, double thresh, int with_z)
{
    int *stack = G_malloc(sizeof(int) * Points->n_points * 2);

    if (!stack) {
	G_fatal_error(_("Out of memory"));
	return Points->n_points;
    }

    int *index = G_malloc(sizeof(int) * Points->n_points);	/* Indices of points in output line */

    if (!index) {
	G_fatal_error(_("Out of memory"));
	G_free(stack);
	return Points->n_points;
    }

    int top = 2;		/* first free slot in the stack */
    int icount = 1;		/* number of indices stored */
    int i;

    thresh *= thresh;

    index[0] = 0;		/* first point is always in output line */

    /* stack contains pairs of elements: (beginning, end) */

    stack[0] = 0;
    stack[1] = Points->n_points - 1;

    while (top > 0) {		/*there are still segments to consider */
	/*Pop indices of the segment from the stack */
	int last = stack[--top];
	int first = stack[--top];

	double x1 = Points->x[first];
	double y1 = Points->y[first];
	double z1 = Points->z[first];

	double x2 = Points->x[last];
	double y2 = Points->y[last];
	double z2 = Points->z[last];

	int maxindex = -1;
	double maxdist = -1;
	int i;

	for (i = first + 1; i <= last - 1; i++) {	/* Find the furthermost point between first, last */
	    double px, py, pz, pdist;
	    int status;
	    double dist =
		dig_distance2_point_to_line(Points->x[i], Points->y[i],
					    Points->z[i],
					    x1, y1, z1, x2, y2, z2, with_z,
					    &px, &py, &pz, &pdist, &status);

	    if (maxindex == -1 || dist > maxdist) {	/* update the furthermost point so far seen */
		maxindex = i;
		maxdist = dist;
	    }
	}


	if (maxindex == -1 || maxdist <= thresh) {	/* no points between or all point are inside the threshold */
	    index[icount++] = last;
	}
	else {
	    /* break line into two parts, the order of pushing is crucial! It gurantees, that we are going to the left */
	    stack[top++] = maxindex;
	    stack[top++] = last;
	    stack[top++] = first;
	    stack[top++] = maxindex;
	}


    }


    Points->n_points = icount;

    /* finally, select only points marked in the algorithm */
    for (i = 0; i < icount; i++) {
	Points->x[i] = Points->x[index[i]];
	Points->y[i] = Points->y[index[i]];
	Points->z[i] = Points->z[index[i]];
    }

    G_free(stack);
    G_free(index);
    return (Points->n_points);
}

int lang(struct line_pnts *Points, double thresh, int look_ahead, int with_z)
{

    int count = 1;		/* place where the next point will be added. i.e after the last point */

    int from = 0;
    int to = look_ahead;

    thresh *= thresh;

    while (from < Points->n_points - 1) {	/* repeat until we reach the last point */
	int i;
	int found = 0;		/* whether we have found the point outside the threshold */

	double x1 = Points->x[from];
	double y1 = Points->y[from];
	double z1 = Points->z[from];

	if (Points->n_points - 1 < to) {	/* check that we are always in the line */
	    to = Points->n_points - 1;
	}

	double x2 = Points->x[to];
	double y2 = Points->y[to];
	double z2 = Points->z[to];

	for (i = from + 1; i < to; i++) {
	    double px, py, pz, pdist;
	    int status;

	    if (dig_distance2_point_to_line
		(Points->x[i], Points->y[i], Points->z[i], x1, y1, z1, x2, y2,
		 z2, with_z, &px, &py, &pz, &pdist, &status) > thresh) {
		found = 1;
		break;
	    }
	}

	if (found) {
	    to--;
	}
	else {
	    Points->x[count] = Points->x[to];
	    Points->y[count] = Points->y[to];
	    Points->z[count] = Points->z[to];
	    count++;
	    from = to;
	    to += look_ahead;
	}

    }
    Points->n_points = count;
    return Points->n_points;
}

/* Eliminates all vertices which are close(r than eps) to each other */
int vertex_reduction(struct line_pnts *Points, double eps, int with_z)
{
    int start, i, count, n;

    n = Points->n_points;

    /* there's almost nothing to remove */
    if (n <= 2)
	return Points->n_points;

    count = 0;
    start = 0;

    eps *= eps;
    count = 1;			/*we never remove the first point */

    for (i = 0; i < n - 1; i++) {
	double dx = Points->x[i] - Points->x[start];
	double dy = Points->y[i] - Points->y[start];
	double dz = Points->z[i] - Points->z[start];

	double dst = dx * dx + dy * dy;

	if (with_z) {
	    dst += dz * dz;
	}

	if (dst > eps) {
	    Points->x[count] = Points->x[i];
	    Points->y[count] = Points->y[i];
	    Points->z[count] = Points->z[i];
	    count++;
	    start = i;
	}

    }

    /* last point is also always preserved */
    Points->x[count] = Points->x[n - 1];
    Points->y[count] = Points->y[n - 1];
    Points->z[count] = Points->z[n - 1];
    count++;
    Points->n_points = count;

    return Points->n_points;
}

/*Reumann-Witkam algorithm 
 * Returns number of points in the output line
 */
int reumann_witkam(struct line_pnts *Points, double thresh, int with_z)
{
    int i, count;
    POINT x0, x1, x2, sub, diff;
    double subd, diffd, sp, dist;
    int n;

    n = Points->n_points;

    if (n < 3)
	return n;

    thresh *= thresh;

    count = 1;

    point_assign(Points, 0, with_z, &x1, 0);
    point_assign(Points, 1, with_z, &x2, 0);
    point_subtract(x2, x1, &sub);
    subd = point_dist2(sub);


    for (i = 2; i < n; i++) {

	point_assign(Points, i, with_z, &x0, 0);
	point_subtract(x1, x0, &diff);
	diffd = point_dist2(diff);
	sp = point_dot(diff, sub);
	dist = (diffd * subd - sp * sp) / subd;
	/* if the point is out of the threshlod-sausage, store it and calculate
	 * all variables which do not change for each line-point calculation */
	if (dist > thresh) {

	    point_assign(Points, i - 1, with_z, &x1, 0);
	    point_assign(Points, i, with_z, &x2, 0);
	    point_subtract(x2, x1, &sub);
	    subd = point_dist2(sub);

	    Points->x[count] = x0.x;
	    Points->y[count] = x0.y;
	    Points->z[count] = x0.z;
	    count++;
	}


    }

    Points->x[count] = Points->x[n - 1];
    Points->y[count] = Points->y[n - 1];
    Points->z[count] = Points->z[n - 1];
    Points->n_points = count + 1;

    return Points->n_points;

}

/* douglas-peucker algorithm which simplifies a line to a line with
 * at most reduction% of points.
 * returns the number of points in the output line. It is approx 
 * reduction/100 * Points->n_points.
 */
int douglas_peucker_reduction(struct line_pnts *Points, double thresh,
			      double reduction, int with_z)
{

    int i;
    int n = Points->n_points;

    /* the maximum number of points  which may be 
     * included in the output */
    int nexp = n * (reduction / (double)100.0);

    /* line too short */
    if (n < 3)
	return n;

    /* indicates which point were selected by the algorithm */
    int *sel;
    sel = G_calloc(sizeof(int), n);
    if (sel == NULL) {
	G_fatal_error(_("Out of memory"));
	return n;
    }

    /* array used for storing the indices of line segments+furthest point */
    int *index;
    index = G_malloc(sizeof(int) * 3 * n);
    if (index == NULL) {
	G_fatal_error(_("Out of memory"));
	G_free(sel);
	return n;
    }

    int indices;

    indices = 0;

    /* preserve first and last point */
    sel[0] = sel[n - 1] = 1;
    nexp -= 2;

    thresh *= thresh;

    double d;
    int mid = get_furthest(Points, 0, n - 1, with_z, &d);
    int em;

    /* priority queue of line segments,
     * key is the distance of the furthest point */
    binary_heap pq;

    if (!binary_heap_init(n, &pq)) {
	G_fatal_error(_("Out of memory"));
	G_free(sel);
	G_free(index);
	return n;
    }


    if (d > thresh) {
	index[0] = 0;
	index[1] = n - 1;
	index[2] = mid;
	binary_heap_push(d, 0, &pq);
	indices = 3;
    }

    /* while we can add new points and queue is non-empty */
    while (nexp > 0) {
	/* empty heap */
	if (!binary_heap_extract_max(&pq, &em))
	    break;
	int left = index[em];
	int right = index[em + 1];
	int furt = index[em + 2];

	/*mark the furthest point */
	sel[furt] = 1;
	nexp--;

	/* consider left and right segment */
	mid = get_furthest(Points, left, furt, with_z, &d);
	if (d > thresh) {
	    binary_heap_push(d, indices, &pq);
	    index[indices++] = left;
	    index[indices++] = furt;
	    index[indices++] = mid;
	}

	mid = get_furthest(Points, furt, right, with_z, &d);
	if (d > thresh) {
	    binary_heap_push(d, indices, &pq);
	    index[indices++] = furt;
	    index[indices++] = right;
	    index[indices++] = mid;
	}

    }

    /* copy selected points */
    int selected = 0;

    for (i = 0; i < n; i++) {
	if (sel[i]) {
	    Points->x[selected] = Points->x[i];
	    Points->y[selected] = Points->y[i];
	    Points->z[selected] = Points->z[i];
	    selected++;
	}
    }

    G_free(sel);
    G_free(index);
    binary_heap_free(&pq);
    Points->n_points = selected;
    return Points->n_points;
}
