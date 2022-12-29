
/**************************************************************
 * find intersection between two lines defined by points on the lines
 * line segment A is (ax1,ay1) to (ax2,ay2)
 * line segment B is (bx1,by1) to (bx2,by2)
 * returns
 *   -1 segment A and B do not intersect (parallel without overlap)
 *    0 segment A and B do not intersect but extensions do intersect
 *    1 intersection is a single point
 *    2 intersection is a line segment (colinear with overlap)
 * x,y intersection point
 * ra - ratio that the intersection divides A 
 * rb - ratio that the intersection divides B
 *
 *                              B2
 *                              /
 *                             /
 *   r=p/(p+q) : A1---p-------*--q------A2
 *                           /
 *                          /
 *                         B1
 *
 **************************************************************/

/**************************************************************
*
* A point P which lies on line defined by points A1=(x1,y1) and A2=(x2,y2)
* is given by the equation r * (x2,y2) + (1-r) * (x1,y1).
* if r is between 0 and 1, p lies between A1 and A2.
* 
* Suppose points on line (A1, A2) has equation 
*     (x,y) = ra * (ax2,ay2) + (1-ra) * (ax1,ay1)
* or for x and y separately
*     x = ra * ax2 - ra * ax1 + ax1
*     y = ra * ay2 - ra * ay1 + ay1
* and the points on line (B1, B2) are represented by
*     (x,y) = rb * (bx2,by2) + (1-rb) * (bx1,by1)
* or for x and y separately
*     x = rb * bx2 - rb * bx1 + bx1
*     y = rb * by2 - rb * by1 + by1
* 
* when the lines intersect, the point (x,y) has to
* satisfy a system of 2 equations:
*     ra * ax2 - ra * ax1 + ax1 = rb * bx2 - rb * bx1 + bx1
*     ra * ay2 - ra * ay1 + ay1 = rb * by2 - rb * by1 + by1
* 
* or
* 
*     (ax2 - ax1) * ra - (bx2 - bx1) * rb = bx1 - ax1
*     (ay2 - ay1) * ra - (by2 - by1) * rb = by1 - ay1
* 
* by Cramer's method, one can solve this by computing 3
* determinants of matrices:
* 
*    M  = (ax2-ax1)  (bx1-bx2)
*         (ay2-ay1)  (by1-by2)
* 
*    M1 = (bx1-ax1)  (bx1-bx2)
*         (by1-ay1)  (by1-by2)
* 
*    M2 = (ax2-ax1)  (bx1-ax1)
*         (ay2-ay1)  (by1-ay1)
* 
* Which are exactly the determinants D, D2, D1 below:
* 
*   D  ((ax2-ax1)*(by1-by2) - (ay2-ay1)*(bx1-bx2))
* 
*   D1 ((bx1-ax1)*(by1-by2) - (by1-ay1)*(bx1-bx2))
* 
*   D2 ((ax2-ax1)*(by1-ay1) - (ay2-ay1)*(bx1-ax1))
***********************************************************************/


#define D  ((ax2-ax1)*(by1-by2) - (ay2-ay1)*(bx1-bx2))
#define D1 ((bx1-ax1)*(by1-by2) - (by1-ay1)*(bx1-bx2))
#define D2 ((ax2-ax1)*(by1-ay1) - (ay2-ay1)*(bx1-ax1))

#define SWAP(x,y) {double t; t=x; x=y; y=t;}

int G_intersect_line_segments(double ax1, double ay1, double ax2, double ay2,
			      double bx1, double by1, double bx2, double by2,
			      double *ra, double *rb, double *x, double *y)
{
    double d;

    if (ax1 > ax2 || (ax1 == ax2 && ay1 > ay2)) {
	SWAP(ax1, ax2)
	SWAP(ay1, ay2)
    }

    if (bx1 > bx2 || (bx1 == bx2 && by1 > by2)) {
	SWAP(bx1, bx2)
	SWAP(by1, by2)
    }

    d = D;

    if (d) {			/* lines are not parallel */
	*ra = D1 / d;
	*rb = D2 / d;

	*x = ax1 + (*ra) * (ax2 - ax1);
	*y = ay1 + (*ra) * (ay2 - ay1);
	return (*ra >= 0.0 && *ra <= 1.0 && *rb >= 0.0 && *rb <= 1.0);
    }

    /* lines are parallel */
    if (D1 || D2) {
	return -1;
    }

    /* segments are colinear. check for overlap */

    /* Collinear vertical */
    if (ax1 == ax2) {
	if (ay1 > by2) {
	    *x = ax1;
	    *y = ay1;
	    return 0;		/* extensions overlap */
	}
	if (ay2 < by1) {
	    *x = ax2;
	    *y = ay2;
	    return 0;		/* extensions overlap */
	}

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
	return 2;
    }
    else {
	if (ax1 > bx2) {
	    *x = ax1;
	    *y = ay1;
	    return 0;		/* extensions overlap */
	}
	if (ax2 < bx1) {
	    *x = ax2;
	    *y = ay2;
	    return 0;		/* extensions overlap */
	}

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
	return 2;
    }

    return 0; /* should not be reached */
}
