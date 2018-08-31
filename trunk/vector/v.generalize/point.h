
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

#ifndef POINT_H
#define POINT_H

#include <grass/vector.h>

typedef struct
{
    double x, y, z;
} POINT;

typedef struct Point_list
{
    POINT p;
    struct Point_list *next;
} POINT_LIST;

/* res = a - b */
extern inline void point_subtract(POINT a, POINT b, POINT * res);

/* res = a + b */
extern inline void point_add(POINT a, POINT b, POINT * res);

/* dot product of two vectors: ax * bx + ay * by + az * bz */
extern double point_dot(POINT a, POINT b);

/* squared distance from the origin */
extern inline double point_dist2(POINT a);

/* assign point Points[index] to the res
 * if with z = 0 then res.z = 0  
 */
extern inline void point_assign(struct line_pnts *Points, int index,
				int with_z, POINT * res, int is_loop);
/* assign point Points[index] to the res
 * if with z = 0 then res.z = 0  
 * loop to infinite
 */

/* res = k * a */
extern inline void point_scalar(POINT a, double k, POINT * res);

/* copy the last point of Points to Points[pos] */
extern inline void points_copy_last(struct line_pnts *Points, int pos);

/* distance between two points */
extern inline double point_dist(POINT a, POINT b);

/* squared distance between two points */
extern inline double point_dist_square(POINT a, POINT b);

/* angle in radians between vectors ab and bc */
extern inline double point_angle_between(POINT a, POINT b, POINT c);

/* distance squared between a and segment bc */
extern inline double point_dist_segment_square(POINT a, POINT b, POINT c,
					       int with_z);
/* creates empty list of points */
extern POINT_LIST *point_list_new(POINT p);

/* insert new value to the list just after the l. i.e l->next.p = p */
extern void point_list_add(POINT_LIST * l, POINT p);

/* copy POINT_LIST structure into line_pnts structure 
 * return 0 on success, -1 on out of memory 
 */
extern int point_list_copy_to_line_pnts(POINT_LIST l,
					struct line_pnts *Points);
/*free the momory occupied by the list at l.next */
extern void point_list_free(POINT_LIST l);

/*delete the p->next element and set the pointers appropriatelly */
extern void point_list_delete_next(POINT_LIST * p);
#endif
