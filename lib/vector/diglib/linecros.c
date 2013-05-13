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
#include <stdio.h>

/***************************************************************
* test_for_intersection (ax1,ay1,ax2,ay2,bx1,by1,bx2,by2)
*   double ax1,ax2,ay1,ay2;
*   double bx1,bx2,by1,by2;
*
* returns
*   0 no intersection at all
*   1 the line segments intersect at only one point
*  -1 the line segments intersect at many points, i.e., overlapping
*     segments from the same line
*
* find_intersection (ax1,ay1,ax2,ay2,bx1,by1,bx2,by2,x,y)
*   double ax1,ax2,ay1,ay2;
*   double bx1,bx2,by1,by2;
*   double *x,*y;
*
* returns
*   0 no intersection
*   1 x,y set to (unique) intersection
*  -1 lines overlap, no unique intersection
*
* Based on the following:
*
*    (ax2-ax1)r1 - (bx2-bx1)r2 = ax2 - ax1
*    (ay2-ay1)r1 - (by2-by1)r2 = ay2 - ay1
*
* Solving for r1 and r2, if r1 and r2 are between 0 and 1,
* then line segments (ax1,ay1)(ax2,ay2) and (bx1,by1)(bx2,by2)
* intersect
****************************************************************/

#define D  ((ax2-ax1)*(by1-by2) - (ay2-ay1)*(bx1-bx2))

#define D1 ((bx1-ax1)*(by1-by2) - (by1-ay1)*(bx1-bx2))

#define D2 ((ax2-ax1)*(by1-ay1) - (ay2-ay1)*(bx1-ax1))

int
dig_test_for_intersection(double ax1, double ay1,
			  double ax2, double ay2,
			  double bx1, double by1, double bx2, double by2)
{
    register double d, d1, d2;
    double t;

    d = D;
    d1 = D1;
    d2 = D2;

    if (d > 0)
	return (d1 >= 0 && d2 >= 0 && d >= d1 && d >= d2);
    if (d < 0)
	return (d1 <= 0 && d2 <= 0 && d <= d1 && d <= d2);

    /* lines are parallel */
    if (d1 || d2)
	return 0;

    /* segments are colinear. check for overlap */

    /* Collinear vertical */
    if (ax1 == ax2) {
	if (ay1 > ay2) {
	    t = ay1;
	    ay1 = ay2;
	    ay2 = t;
	}
	if (by1 > by2) {
	    t = by1;
	    by1 = by2;
	    by2 = t;
	}
	if (ay1 > by2)
	    return 0;
	if (ay2 < by1)
	    return 0;

	/* there is overlap */

	if (ay1 == by2 || ay2 == by1)
	    return 1;		/* endpoints only */

	return -1;			/* true overlap   */
    }
    else {
	if (ax1 > ax2) {
	    t = ax1;
	    ax1 = ax2;
	    ax2 = t;
	}
	if (bx1 > bx2) {
	    t = bx1;
	    bx1 = bx2;
	    bx2 = t;
	}
	if (ax1 > bx2)
	    return 0;
	if (ax2 < bx1)
	    return 0;

	/* there is overlap */

	if (ax1 == bx2 || ax2 == bx1)
	    return 1;		/* endpoints only */

	return -1;			/* true overlap   */
    }    
    return 0; /* should not be reached */
}


int
dig_find_intersection(double ax1, double ay1,
		      double ax2, double ay2,
		      double bx1, double by1,
		      double bx2, double by2, double *x, double *y)
{
    register double d, r1, r2;
    double t;

    if (ax1 > ax2 || (ax1 == ax2 && ay1 > ay2)) {
	t = ax1;
	ax1 = ax2;
	ax2 = t;

	t = ay1;
	ay1 = ay2;
	ay2 = t;
    }

    if (bx1 > bx2 || (bx1 == bx2 && by1 > by2)) {
	t = bx1;
	bx1 = bx2;
	bx2 = t;

	t = by1;
	by1 = by2;
	by2 = t;
    }

    d = D;

    if (d) {

	r1 = D1 / d;
	r2 = D2 / d;
	if (r1 < 0 || r1 > 1 || r2 < 0 || r2 > 1) {
	    return 0;
	}
	*x = ax1 + r1 * (ax2 - ax1);
	*y = ay1 + r1 * (ay2 - ay1);
	return 1;
    }

    /* lines are parallel */
    if (D1 || D2) {
	return 0;
    }

    /* segments are colinear. check for overlap */

    /* Collinear vertical */
    if (ax1 == ax2) {
	if (ay1 > by2)
	    return 0;
	if (ay2 < by1)
	    return 0;

	/* there is overlap */

	if (ay1 == by2) {
	    *x = ax1;
	    *y = ay1;
	    return 1;		/* endpoints only */
	}
	if (ay2 == by1) {
	    *x = ax2;
	    *y = ay2;
	    return 1;		/* endpoints only */
	}

	/* overlap, no single intersection point */
	if (ay1 > by1 && ay1 < by2) {
	    *x = ax1;
	    *y = ay1;
	}
	else {
	    *x = ax2;
	    *y = ay2;
	}
	return -1;
    }
    else {
	if (ax1 > bx2)
	    return 0;
	if (ax2 < bx1)
	    return 0;

	/* there is overlap */

	if (ax1 == bx2) {
	    *x = ax1;
	    *y = ay1;
	    return 1;		/* endpoints only */
	}
	if (ax2 == bx1) {
	    *x = ax2;
	    *y = ay2;
	    return 1;		/* endpoints only */
	}
	
	/* overlap, no single intersection point */
	if (ax1 > bx1 && ax1 < bx2) {
	    *x = ax1;
	    *y = ay1;
	}
	else {
	    *x = ax2;
	    *y = ay2;
	}
	return -1;
    }

    return 0; /* should not be reached */
}
