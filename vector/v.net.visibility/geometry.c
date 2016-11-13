
/****************************************************************
 * MODULE:     v.path.obstacles
 *
 * AUTHOR(S):  Maximilian Maldacker
 *  
 *
 * COPYRIGHT:  (C) 2002-2005 by the GRASS Development Team
 *
 *             This program is free software under the
 *             GNU General Public License (>=v2).
 *             Read the file COPYING that comes with GRASS
 *             for details.
 *
 ****************************************************************/

#include "geometry.h"

/** true if q lies nearer to p than segment e, i.e. pq and e don't intersect
*/
int before(struct Point *p, struct Point *q, struct Line *e)
{
    double x1, y1, z1, x2, y2, z2;
    int status;

    if (e == NULL)
	return 1;

    status =
	Vect_segment_intersection(p->x, p->y, 0, q->x, q->y, 0, e->p1->x,
				  e->p1->y, 0, e->p2->x, e->p2->y, 0, &x1,
				  &y1, &z1, &x2, &y2, &z2, 0) == 0;

    return status;
}

/** returns true if p3 is left of the directed line p1p2
*/
int left_turn(struct Point *p1, struct Point *p2, struct Point *p3)
{
    double a, b, c, d;

    if (p3->y == PORT_DOUBLE_MAX) {
	return (p1->x < p2->x || (p1->x == p2->x && p1->y < p2->y));
    }
    else {
	a = p1->x - p2->x;
	b = p1->y - p2->y;
	c = p3->x - p2->x;
	d = p3->y - p2->y;

	return a * d - b * c < 0.0;
    }
}

/** returns true if p is in between the segment e along the x axis
*/
int in_between(struct Point *p, struct Line *e)
{
    int a = e->p1->x <= p->x && e->p2->x >= p->x;
    int b = e->p2->x <= p->x && e->p1->x >= p->x;

    return a || b;
}


/** tests if the point (x, y ) is inside the boundary of p 
*/
int point_inside(struct Point *p, double x, double y)
{
    int c = 0;
    struct Point *n1 = p;
    struct Point *n2 = other2(p);

    do {
	if ((((n1->y <= y) && (y < n2->y)) ||
	     ((n2->y <= y) && (y < n1->y))) &&
	    (x < (n2->x - n1->x) * (y - n1->y) / (n2->y - n1->y) + n1->x))
	    c = !c;

	n1 = other2(n1);
	n2 = other2(n2);

    } while (n1 != p);

    return c;
}

/** returns 1 if the segment intersect with the half line starting from p pointing downards
	x and y are the intersection point
*/
int segment_intersect(struct Line *line, struct Point *p, double *y)
{
    struct Point *p1 = line->p1;
    struct Point *p2 = line->p2;
    double t;

    if (in_between(p, line)) {
	if (p2->x != p1->x) {
	    t = (p->x - p1->x) / (p2->x - p1->x);

	    *y = p1->y + t * (p2->y - p1->y);
	}
	else {
	    if (p1->y > p->y || p2->y > p->y)
		return -1;

	    *y = p1->y > p2->y ? p1->y : p2->y;
	}

	return 1;
    }
    else
	return -1;

}
