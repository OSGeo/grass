
/****************************************************************
 *
 * MODULE:     v.generalize
 *
 * AUTHOR(S):  Daniel Bundala
 *
 * PURPOSE:    Definition of a point in 3D and basic operations
 *             with points 
 *
 * COPYRIGHT:  (C) 2002-2005 by the GRASS Development Team
 *
 *             This program is free software under the
 *             GNU General Public License (>=v2).
 *             Read the file COPYING that comes with GRASS
 *             for details.
 *
 ****************************************************************/

#include <math.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>
#include "point.h"

inline void point_subtract(POINT a, POINT b, POINT * res)
{
    res->x = a.x - b.x;
    res->y = a.y - b.y;
    res->z = a.z - b.z;
    return;
}

inline void point_add(POINT a, POINT b, POINT * res)
{
    res->x = a.x + b.x;
    res->y = a.y + b.y;
    res->z = a.z + b.z;
    return;
}

double point_dot(POINT a, POINT b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline double point_dist2(POINT a)
{
    return a.x * a.x + a.y * a.y + a.z * a.z;
}

inline void point_assign(struct line_pnts *Points, int index, int with_z,
			 POINT * res)
{
    res->x = Points->x[index];
    res->y = Points->y[index];
    if (with_z) {
	res->z = Points->z[index];
    }
    else {
	res->z = 0;
    }
    return;
}

inline void point_assign_loop(struct line_pnts *Points, int index, int with_z,
			 POINT * res)
{
    index = index % Points->n_points;
    res->x = Points->x[index];
    res->y = Points->y[index];
    if (with_z) {
	res->z = Points->z[index];
    }
    else {
	res->z = 0;
    }
    return;
}

inline void point_scalar(POINT a, double k, POINT * res)
{
    res->x = a.x * k;
    res->y = a.y * k;
    res->z = a.z * k;
    return;
}

inline void points_copy_last(struct line_pnts *Points, int pos)
{
    int n = Points->n_points - 1;

    Points->x[pos] = Points->x[n];
    Points->y[pos] = Points->y[n];
    Points->z[pos] = Points->z[n];
    Points->n_points = pos + 1;
    return;
}

inline double point_dist(POINT a, POINT b)
{
    return sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y) +
		(a.z - b.z) * (a.z - b.z));
}

inline double point_dist_square(POINT a, POINT b)
{
    return (a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y) +
	(a.z - b.z) * (a.z - b.z);
}

inline double point_angle_between(POINT a, POINT b, POINT c)
{
    point_subtract(b, a, &a);
    point_subtract(c, b, &b);
    return acos(point_dot(a, b) / sqrt(point_dist2(a) * point_dist2(b)));
}

inline double point_dist_segment_square(POINT a, POINT b, POINT c, int with_z)
{
    double px, py, pz, pdist;
    int status;

    return dig_distance2_point_to_line(a.x, a.y, a.z, b.x, b.y, b.z,
				       c.x, c.y, c.z, with_z, &px, &py, &pz,
				       &pdist, &status);
}

POINT_LIST *point_list_new(POINT p)
{
    POINT_LIST *pl;

    pl = G_malloc(sizeof(POINT_LIST));

    pl->next = NULL;
    pl->p = p;
    return pl;
}

void point_list_add(POINT_LIST * l, POINT p)
{
    POINT_LIST *n;

    n = point_list_new(p);
    n->next = l->next;
    l->next = n;
    return;
}

int point_list_copy_to_line_pnts(POINT_LIST l, struct line_pnts *Points)
{

    int length, i;
    POINT_LIST *cur;

    cur = l.next;
    length = 0;

    while (cur != NULL) {
	length++;
	cur = cur->next;
    }

    if (length != Points->n_points)
	if (0 > dig_alloc_points(Points, length))
	    return (-1);

    Points->n_points = length;

    cur = l.next;
    for (i = 0; i < length; i++) {
	Points->x[i] = cur->p.x;
	Points->y[i] = cur->p.y;
	Points->z[i] = cur->p.z;
	cur = cur->next;
    }

    return 0;
}

void point_list_free(POINT_LIST l)
{
    POINT_LIST *p, *n;

    p = l.next;
    while (p != NULL) {
	n = p->next;
	G_free(p);
	p = n;
    }
}

extern void point_list_delete_next(POINT_LIST * p)
{
    POINT_LIST *t = p->next;

    p->next = p->next->next;
    G_free(t);
    return;
}
