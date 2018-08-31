/*!
   \file lib/vector/Vlib/e_intersect.c

   \brief Vector library - intersection (lower level functions)

   Higher level functions for reading/writing/manipulating vectors.

   (C) 2008-2009 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Rewritten by Rosen Matev (Google Summer of Code 2008)
*/

#include <stdlib.h>
#include <math.h>

#include <grass/gis.h>

#include "e_intersect.h"

#define SWAP(a,b) {double t = a; a = b; b = t;}
#ifndef MIN
#define MIN(X,Y) ((X<Y)?X:Y)
#endif
#ifndef MAX
#define MAX(X,Y) ((X>Y)?X:Y)
#endif
#define D ((ax2-ax1)*(by1-by2) - (ay2-ay1)*(bx1-bx2))
#define DA ((bx1-ax1)*(by1-by2) - (by1-ay1)*(bx1-bx2))
#define DB ((ax2-ax1)*(by1-ay1) - (ay2-ay1)*(bx1-ax1))


#ifdef ASDASDASFDSAFFDAS
mpf_t p11, p12, p21, p22, t1, t2;
mpf_t dd, dda, ddb, delta;
mpf_t rra, rrb;

int initialized = 0;

void initialize_mpf_vars()
{
    mpf_set_default_prec(2048);

    mpf_init(p11);
    mpf_init(p12);
    mpf_init(p21);
    mpf_init(p22);

    mpf_init(t1);
    mpf_init(t2);

    mpf_init(dd);
    mpf_init(dda);
    mpf_init(ddb);
    mpf_init(delta);

    mpf_init(rra);
    mpf_init(rrb);

    initialized = 1;
}

/*
   Caclulates:
   |a11-b11  a12-b12|
   |a21-b21  a22-b22|
 */
void det22(mpf_t rop, double a11, double b11, double a12, double b12,
	   double a21, double b21, double a22, double b22)
{
    mpf_set_d(t1, a11);
    mpf_set_d(t2, b11);
    mpf_sub(p11, t1, t2);
    mpf_set_d(t1, a12);
    mpf_set_d(t2, b12);
    mpf_sub(p12, t1, t2);
    mpf_set_d(t1, a21);
    mpf_set_d(t2, b21);
    mpf_sub(p21, t1, t2);
    mpf_set_d(t1, a22);
    mpf_set_d(t2, b22);
    mpf_sub(p22, t1, t2);

    mpf_mul(t1, p11, p22);
    mpf_mul(t2, p12, p21);
    mpf_sub(rop, t1, t2);

    return;
}

void swap(double *a, double *b)
{
    double t = *a;

    *a = *b;
    *b = t;
    return;
}

/* multi-precision version */
int segment_intersection_2d_e(double ax1, double ay1, double ax2, double ay2,
			      double bx1, double by1, double bx2, double by2,
			      double *x1, double *y1, double *x2, double *y2)
{
    double t;

    double max_ax, min_ax, max_ay, min_ay;

    double max_bx, min_bx, max_by, min_by;

    int sgn_d, sgn_da, sgn_db;

    int vertical;

    int f11, f12, f21, f22;

    mp_exp_t exp;

    char *s;

    if (!initialized)
	initialize_mpf_vars();

    /* TODO: Works for points ? */
    G_debug(3, "segment_intersection_2d_e()");
    G_debug(4, "    ax1  = %.18f, ay1  = %.18f", ax1, ay1);
    G_debug(4, "    ax2  = %.18f, ay2  = %.18f", ax2, ay2);
    G_debug(4, "    bx1  = %.18f, by1  = %.18f", bx1, by1);
    G_debug(4, "    bx2  = %.18f, by2  = %.18f", bx2, by2);

    f11 = ((ax1 == bx1) && (ay1 == by1));
    f12 = ((ax1 == bx2) && (ay1 == by2));
    f21 = ((ax2 == bx1) && (ay2 == by1));
    f22 = ((ax2 == bx2) && (ay2 == by2));

    /* Check for identical segments */
    if ((f11 && f22) || (f12 && f21)) {
	G_debug(3, "    identical segments");
	*x1 = ax1;
	*y1 = ay1;
	*x2 = ax2;
	*y2 = ay2;
	return 5;
    }
    /* Check for identical endpoints */
    if (f11 || f12) {
	G_debug(3, "    connected by endpoints");
	*x1 = ax1;
	*y1 = ay1;
	return 1;
    }
    if (f21 || f22) {
	G_debug(3, "    connected by endpoints");
	*x1 = ax2;
	*y1 = ay2;
	return 1;
    }

    if ((MAX(ax1, ax2) < MIN(bx1, bx2)) || (MAX(bx1, bx2) < MIN(ax1, ax2))) {
	G_debug(3, "    no intersection (disjoint bounding boxes)");
	return 0;
    }
    if ((MAX(ay1, ay2) < MIN(by1, by2)) || (MAX(by1, by2) < MIN(ay1, ay2))) {
	G_debug(3, "    no intersection (disjoint bounding boxes)");
	return 0;
    }

    det22(dd, ax2, ax1, bx1, bx2, ay2, ay1, by1, by2);
    sgn_d = mpf_sgn(dd);
    if (sgn_d != 0) {
	G_debug(3, "    general position");

	det22(dda, bx1, ax1, bx1, bx2, by1, ay1, by1, by2);
	sgn_da = mpf_sgn(dda);

	/*mpf_div(rra, dda, dd);
	   mpf_div(rrb, ddb, dd);
	   s = mpf_get_str(NULL, &exp, 10, 40, rra);
	   G_debug(4, "        ra = %sE%d", (s[0]==0)?"0":s, exp);
	   s = mpf_get_str(NULL, &exp, 10, 24, rrb);
	   G_debug(4, "        rb = %sE%d", (s[0]==0)?"0":s, exp);
	 */

	if (sgn_d > 0) {
	    if ((sgn_da < 0) || (mpf_cmp(dda, dd) > 0)) {
		G_debug(3, "        no intersection");
		return 0;
	    }

	    det22(ddb, ax2, ax1, bx1, ax1, ay2, ay1, by1, ay1);
	    sgn_db = mpf_sgn(ddb);
	    if ((sgn_db < 0) || (mpf_cmp(ddb, dd) > 0)) {
		G_debug(3, "        no intersection");
		return 0;
	    }
	}
	else {			/* if sgn_d < 0 */
	    if ((sgn_da > 0) || (mpf_cmp(dda, dd) < 0)) {
		G_debug(3, "        no intersection");
		return 0;
	    }

	    det22(ddb, ax2, ax1, bx1, ax1, ay2, ay1, by1, ay1);
	    sgn_db = mpf_sgn(ddb);
	    if ((sgn_db > 0) || (mpf_cmp(ddb, dd) < 0)) {
		G_debug(3, "        no intersection");
		return 0;
	    }
	}

	/*G_debug(3, "        ra=%.17g rb=%.17g", mpf_get_d(dda)/mpf_get_d(dd), mpf_get_d(ddb)/mpf_get_d(dd)); */
	/*G_debug(3, "        sgn_d=%d sgn_da=%d sgn_db=%d cmp(dda,dd)=%d cmp(ddb,dd)=%d", sgn_d, sgn_da, sgn_db, mpf_cmp(dda, dd), mpf_cmp(ddb, dd)); */

	mpf_set_d(delta, ax2 - ax1);
	mpf_mul(t1, dda, delta);
	mpf_div(t2, t1, dd);
	*x1 = ax1 + mpf_get_d(t2);

	mpf_set_d(delta, ay2 - ay1);
	mpf_mul(t1, dda, delta);
	mpf_div(t2, t1, dd);
	*y1 = ay1 + mpf_get_d(t2);

	G_debug(3, "        intersection %.16g, %.16g", *x1, *y1);
	return 1;
    }

    /* segments are parallel or collinear */
    det22(dda, bx1, ax1, bx1, bx2, by1, ay1, by1, by2);
    sgn_da = mpf_sgn(dda);
    if (sgn_da != 0) {
	/* segments are parallel */
	G_debug(3, "    parallel segments");
	return 0;
    }

    /* segments are colinear. check for overlap */

    /* swap endpoints if needed */
    /* if segments are vertical, we swap x-coords with y-coords */
    vertical = 0;
    if (ax1 > ax2) {
	SWAP(ax1, ax2);
	SWAP(ay1, ay2);
    }
    else if (ax1 == ax2) {
	vertical = 1;
	if (ay1 > ay2)
	    SWAP(ay1, ay2);
	SWAP(ax1, ay1);
	SWAP(ax2, ay2);
    }
    if (bx1 > bx2) {
	SWAP(bx1, bx2);
	SWAP(by1, by2);
    }
    else if (bx1 == bx2) {
	if (by1 > by2)
	    SWAP(by1, by2);
	SWAP(bx1, by1);
	SWAP(bx2, by2);
    }

    G_debug(3, "    collinear segments");

    if ((bx2 < ax1) || (bx1 > ax2)) {
	G_debug(3, "        no intersection");
	return 0;
    }

    /* there is overlap or connected end points */
    G_debug(3, "        overlap");

    /* a contains b */
    if ((ax1 < bx1) && (ax2 > bx2)) {
	G_debug(3, "            a contains b");
	if (!vertical) {
	    *x1 = bx1;
	    *y1 = by1;
	    *x2 = bx2;
	    *y2 = by2;
	}
	else {
	    *x1 = by1;
	    *y1 = bx1;
	    *x2 = by2;
	    *y2 = bx2;
	}
	return 3;
    }

    /* b contains a */
    if ((ax1 > bx1) && (ax2 < bx2)) {
	G_debug(3, "            b contains a");
	if (!vertical) {
	    *x1 = bx1;
	    *y1 = by1;
	    *x2 = bx2;
	    *y2 = by2;
	}
	else {
	    *x1 = by1;
	    *y1 = bx1;
	    *x2 = by2;
	    *y2 = bx2;
	}
	return 4;
    }

    /* general overlap, 2 intersection points */
    G_debug(3, "        partial overlap");
    if ((bx1 > ax1) && (bx1 < ax2)) {	/* b1 is in a */
	if (!vertical) {
	    *x1 = bx1;
	    *y1 = by1;
	    *x2 = ax2;
	    *y2 = ay2;
	}
	else {
	    *x1 = by1;
	    *y1 = bx1;
	    *x2 = ay2;
	    *y2 = ax2;
	}
	return 2;
    }
    if ((bx2 > ax1) && (bx2 < ax2)) {	/* b2 is in a */
	if (!vertical) {
	    *x1 = bx2;
	    *y1 = by2;
	    *x2 = ax1;
	    *y2 = ay1;
	}
	else {
	    *x1 = by2;
	    *y1 = bx2;
	    *x2 = ay1;
	    *y2 = ax1;
	}
	return 2;
    }

    /* should not be reached */
    G_warning(("segment_intersection_2d() ERROR (should not be reached)"));
    G_warning("%.16g %.16g", ax1, ay1);
    G_warning("%.16g %.16g", ax2, ay2);
    G_warning("x");
    G_warning("%.16g %.16g", bx1, by1);
    G_warning("%.16g %.16g", bx2, by2);

    return 0;
}
#endif

/* OLD */
/* tollerance aware version */
/* TODO: fix all ==s left */
int segment_intersection_2d_tol(double ax1, double ay1, double ax2,
				double ay2, double bx1, double by1,
				double bx2, double by2, double *x1,
				double *y1, double *x2, double *y2,
				double tol)
{
    double tola, tolb;

    double d, d1, d2, ra, rb, t;

    int switched = 0;

    /* TODO: Works for points ? */
    G_debug(4, "segment_intersection_2d()");
    G_debug(4, "    ax1  = %.18f, ay1  = %.18f", ax1, ay1);
    G_debug(4, "    ax2  = %.18f, ay2  = %.18f", ax2, ay2);
    G_debug(4, "    bx1  = %.18f, by1  = %.18f", bx1, by1);
    G_debug(4, "    bx2  = %.18f, by2  = %.18f", bx2, by2);


    /* Check identical lines */
    if ((FEQUAL(ax1, bx1, tol) && FEQUAL(ay1, by1, tol) &&
	 FEQUAL(ax2, bx2, tol) && FEQUAL(ay2, by2, tol)) ||
	(FEQUAL(ax1, bx2, tol) && FEQUAL(ay1, by2, tol) &&
	 FEQUAL(ax2, bx1, tol) && FEQUAL(ay2, by1, tol))) {
	G_debug(2, " -> identical segments");
	*x1 = ax1;
	*y1 = ay1;
	*x2 = ax2;
	*y2 = ay2;
	return 5;
    }

    /*  'Sort' lines by x1, x2, y1, y2 */
    if (bx1 < ax1)
	switched = 1;
    else if (bx1 == ax1) {
	if (bx2 < ax2)
	    switched = 1;
	else if (bx2 == ax2) {
	    if (by1 < ay1)
		switched = 1;
	    else if (by1 == ay1) {
		if (by2 < ay2)
		    switched = 1;	/* by2 != ay2 (would be identical */
	    }
	}
    }

    if (switched) {
	t = ax1;
	ax1 = bx1;
	bx1 = t;
	t = ay1;
	ay1 = by1;
	by1 = t;
	t = ax2;
	ax2 = bx2;
	bx2 = t;
	t = ay2;
	ay2 = by2;
	by2 = t;
    }

    d = (ax2 - ax1) * (by1 - by2) - (ay2 - ay1) * (bx1 - bx2);
    d1 = (bx1 - ax1) * (by1 - by2) - (by1 - ay1) * (bx1 - bx2);
    d2 = (ax2 - ax1) * (by1 - ay1) - (ay2 - ay1) * (bx1 - ax1);

    G_debug(2, "    d  = %.18g", d);
    G_debug(2, "    d1 = %.18g", d1);
    G_debug(2, "    d2 = %.18g", d2);

    tola = tol / MAX(fabs(ax2 - ax1), fabs(ay2 - ay1));
    tolb = tol / MAX(fabs(bx2 - bx1), fabs(by2 - by1));
    G_debug(2, "    tol  = %.18g", tol);
    G_debug(2, "    tola = %.18g", tola);
    G_debug(2, "    tolb = %.18g", tolb);
    if (!FZERO(d, tol)) {
	ra = d1 / d;
	rb = d2 / d;

	G_debug(2, "    not parallel/collinear: ra = %.18g", ra);
	G_debug(2, "                            rb = %.18g", rb);

	if ((ra <= -tola) || (ra >= 1 + tola) || (rb <= -tolb) ||
	    (rb >= 1 + tolb)) {
	    G_debug(2, "        no intersection");
	    return 0;
	}

	ra = MIN(MAX(ra, 0), 1);
	*x1 = ax1 + ra * (ax2 - ax1);
	*y1 = ay1 + ra * (ay2 - ay1);

	G_debug(2, "        intersection %.18f, %.18f", *x1, *y1);
	return 1;
    }

    /* segments are parallel or collinear */
    G_debug(3, " -> parallel/collinear");

    if ((!FZERO(d1, tol)) || (!FZERO(d2, tol))) {	/* lines are parallel */
	G_debug(2, "  -> parallel");
	return 0;
    }

    /* segments are colinear. check for overlap */

    /*    aa = adx*adx + ady*ady;
       bb = bdx*bdx + bdy*bdy;

       t = (ax1-bx1)*bdx + (ay1-by1)*bdy; */


    /* Collinear vertical */
    /* original code assumed lines were not both vertical
     *  so there is a special case if they are */
    if (FEQUAL(ax1, ax2, tol) && FEQUAL(bx1, bx2, tol) &&
	FEQUAL(ax1, bx1, tol)) {
	G_debug(2, "  -> collinear vertical");
	if (ay1 > ay2) {
	    t = ay1;
	    ay1 = ay2;
	    ay2 = t;
	}			/* to be sure that ay1 < ay2 */
	if (by1 > by2) {
	    t = by1;
	    by1 = by2;
	    by2 = t;
	}			/* to be sure that by1 < by2 */
	if (ay1 > by2 || ay2 < by1) {
	    G_debug(2, "   -> no intersection");
	    return 0;
	}

	/* end points */
	if (FEQUAL(ay1, by2, tol)) {
	    *x1 = ax1;
	    *y1 = ay1;
	    G_debug(2, "   -> connected by end points");
	    return 1;		/* endpoints only */
	}
	if (FEQUAL(ay2, by1, tol)) {
	    *x1 = ax2;
	    *y1 = ay2;
	    G_debug(2, "  -> connected by end points");
	    return 1;		/* endpoints only */
	}

	/* general overlap */
	G_debug(3, "   -> vertical overlap");
	/* a contains b */
	if (ay1 <= by1 && ay2 >= by2) {
	    G_debug(2, "    -> a contains b");
	    *x1 = bx1;
	    *y1 = by1;
	    *x2 = bx2;
	    *y2 = by2;
	    if (!switched)
		return 3;
	    else
		return 4;
	}
	/* b contains a */
	if (ay1 >= by1 && ay2 <= by2) {
	    G_debug(2, "    -> b contains a");
	    *x1 = ax1;
	    *y1 = ay1;
	    *x2 = ax2;
	    *y2 = ay2;
	    if (!switched)
		return 4;
	    else
		return 3;
	}

	/* general overlap, 2 intersection points */
	G_debug(2, "    -> partial overlap");
	if (by1 > ay1 && by1 < ay2) {	/* b1 in a */
	    if (!switched) {
		*x1 = bx1;
		*y1 = by1;
		*x2 = ax2;
		*y2 = ay2;
	    }
	    else {
		*x1 = ax2;
		*y1 = ay2;
		*x2 = bx1;
		*y2 = by1;
	    }
	    return 2;
	}

	if (by2 > ay1 && by2 < ay2) {	/* b2 in a */
	    if (!switched) {
		*x1 = bx2;
		*y1 = by2;
		*x2 = ax1;
		*y2 = ay1;
	    }
	    else {
		*x1 = ax1;
		*y1 = ay1;
		*x2 = bx2;
		*y2 = by2;
	    }
	    return 2;
	}

	/* should not be reached */
	G_warning(("Vect_segment_intersection() ERROR (collinear vertical segments)"));
	G_warning("%.15g %.15g", ax1, ay1);
	G_warning("%.15g %.15g", ax2, ay2);
	G_warning("x");
	G_warning("%.15g %.15g", bx1, by1);
	G_warning("%.15g %.15g", bx2, by2);
	return 0;
    }

    G_debug(2, "   -> collinear non vertical");

    /* Collinear non vertical */
    if ((bx1 > ax1 && bx2 > ax1 && bx1 > ax2 && bx2 > ax2) ||
	(bx1 < ax1 && bx2 < ax1 && bx1 < ax2 && bx2 < ax2)) {
	G_debug(2, "   -> no intersection");
	return 0;
    }

    /* there is overlap or connected end points */
    G_debug(2, "   -> overlap/connected end points");

    /* end points */
    if ((ax1 == bx1 && ay1 == by1) || (ax1 == bx2 && ay1 == by2)) {
	*x1 = ax1;
	*y1 = ay1;
	G_debug(2, "    -> connected by end points");
	return 1;
    }
    if ((ax2 == bx1 && ay2 == by1) || (ax2 == bx2 && ay2 == by2)) {
	*x1 = ax2;
	*y1 = ay2;
	G_debug(2, "    -> connected by end points");
	return 1;
    }

    if (ax1 > ax2) {
	t = ax1;
	ax1 = ax2;
	ax2 = t;
	t = ay1;
	ay1 = ay2;
	ay2 = t;
    }				/* to be sure that ax1 < ax2 */
    if (bx1 > bx2) {
	t = bx1;
	bx1 = bx2;
	bx2 = t;
	t = by1;
	by1 = by2;
	by2 = t;
    }				/* to be sure that bx1 < bx2 */

    /* a contains b */
    if (ax1 <= bx1 && ax2 >= bx2) {
	G_debug(2, "    -> a contains b");
	*x1 = bx1;
	*y1 = by1;
	*x2 = bx2;
	*y2 = by2;
	if (!switched)
	    return 3;
	else
	    return 4;
    }
    /* b contains a */
    if (ax1 >= bx1 && ax2 <= bx2) {
	G_debug(2, "    -> b contains a");
	*x1 = ax1;
	*y1 = ay1;
	*x2 = ax2;
	*y2 = ay2;
	if (!switched)
	    return 4;
	else
	    return 3;
    }

    /* general overlap, 2 intersection points (lines are not vertical) */
    G_debug(2, "    -> partial overlap");
    if (bx1 > ax1 && bx1 < ax2) {	/* b1 is in a */
	if (!switched) {
	    *x1 = bx1;
	    *y1 = by1;
	    *x2 = ax2;
	    *y2 = ay2;
	}
	else {
	    *x1 = ax2;
	    *y1 = ay2;
	    *x2 = bx1;
	    *y2 = by1;
	}
	return 2;
    }
    if (bx2 > ax1 && bx2 < ax2) {	/* b2 is in a */
	if (!switched) {
	    *x1 = bx2;
	    *y1 = by2;
	    *x2 = ax1;
	    *y2 = ay1;
	}
	else {
	    *x1 = ax1;
	    *y1 = ay1;
	    *x2 = bx2;
	    *y2 = by2;
	}
	return 2;
    }

    /* should not be reached */
    G_warning(("segment_intersection_2d() ERROR (collinear non vertical segments)"));
    G_warning("%.15g %.15g", ax1, ay1);
    G_warning("%.15g %.15g", ax2, ay2);
    G_warning("x");
    G_warning("%.15g %.15g", bx1, by1);
    G_warning("%.15g %.15g", bx2, by2);

    return 0;
}

int segment_intersection_2d(double ax1, double ay1, double ax2, double ay2,
			    double bx1, double by1, double bx2, double by2,
			    double *x1, double *y1, double *x2, double *y2)
{
    const int DLEVEL = 4;

    int vertical;

    int f11, f12, f21, f22;

    double d, da, db;

    /* TODO: Works for points ? */
    G_debug(DLEVEL, "segment_intersection_2d()");
    G_debug(4, "    ax1  = %.18f, ay1  = %.18f", ax1, ay1);
    G_debug(4, "    ax2  = %.18f, ay2  = %.18f", ax2, ay2);
    G_debug(4, "    bx1  = %.18f, by1  = %.18f", bx1, by1);
    G_debug(4, "    bx2  = %.18f, by2  = %.18f", bx2, by2);

    f11 = ((ax1 == bx1) && (ay1 == by1));
    f12 = ((ax1 == bx2) && (ay1 == by2));
    f21 = ((ax2 == bx1) && (ay2 == by1));
    f22 = ((ax2 == bx2) && (ay2 == by2));

    /* Check for identical segments */
    if ((f11 && f22) || (f12 && f21)) {
	G_debug(DLEVEL, "    identical segments");
	*x1 = ax1;
	*y1 = ay1;
	*x2 = ax2;
	*y2 = ay2;
	return 5;
    }
    /* Check for identical endpoints */
    if (f11 || f12) {
	G_debug(DLEVEL, "    connected by endpoints");
	*x1 = ax1;
	*y1 = ay1;
	return 1;
    }
    if (f21 || f22) {
	G_debug(DLEVEL, "    connected by endpoints");
	*x1 = ax2;
	*y1 = ay2;
	return 1;
    }

    if ((MAX(ax1, ax2) < MIN(bx1, bx2)) || (MAX(bx1, bx2) < MIN(ax1, ax2))) {
	G_debug(DLEVEL, "    no intersection (disjoint bounding boxes)");
	return 0;
    }
    if ((MAX(ay1, ay2) < MIN(by1, by2)) || (MAX(by1, by2) < MIN(ay1, ay2))) {
	G_debug(DLEVEL, "    no intersection (disjoint bounding boxes)");
	return 0;
    }

    /* swap endpoints if needed */
    /* if segments are vertical, we swap x-coords with y-coords */
    vertical = 0;
    if (ax1 > ax2) {
	SWAP(ax1, ax2);
	SWAP(ay1, ay2);
    }
    else if (ax1 == ax2) {
	vertical = 1;
	if (ay1 > ay2)
	    SWAP(ay1, ay2);
	SWAP(ax1, ay1);
	SWAP(ax2, ay2);
    }
    if (bx1 > bx2) {
	SWAP(bx1, bx2);
	SWAP(by1, by2);
    }
    else if (bx1 == bx2) {
	if (by1 > by2)
	    SWAP(by1, by2);
	SWAP(bx1, by1);
	SWAP(bx2, by2);
    }

    d = D;
    if (d != 0) {
	G_debug(DLEVEL, "    general position");

	da = DA;

	/*mpf_div(rra, dda, dd);
	   mpf_div(rrb, ddb, dd);
	   s = mpf_get_str(NULL, &exp, 10, 40, rra);
	   G_debug(4, "        ra = %sE%d", (s[0]==0)?"0":s, exp);
	   s = mpf_get_str(NULL, &exp, 10, 24, rrb);
	   G_debug(4, "        rb = %sE%d", (s[0]==0)?"0":s, exp);
	 */

	if (d > 0) {
	    if ((da < 0) || (da > d)) {
		G_debug(DLEVEL, "        no intersection");
		return 0;
	    }

	    db = DB;
	    if ((db < 0) || (db > d)) {
		G_debug(DLEVEL, "        no intersection");
		return 0;
	    }
	}
	else {			/* if d < 0 */
	    if ((da > 0) || (da < d)) {
		G_debug(DLEVEL, "        no intersection");
		return 0;
	    }

	    db = DB;
	    if ((db > 0) || (db < d)) {
		G_debug(DLEVEL, "        no intersection");
		return 0;
	    }
	}

	/*G_debug(DLEVEL, "        ra=%.17g rb=%.17g", mpf_get_d(dda)/mpf_get_d(dd), mpf_get_d(ddb)/mpf_get_d(dd)); */
	/*G_debug(DLEVEL, "        sgn_d=%d sgn_da=%d sgn_db=%d cmp(dda,dd)=%d cmp(ddb,dd)=%d", sgn_d, sgn_da, sgn_db, mpf_cmp(dda, dd), mpf_cmp(ddb, dd)); */

	*x1 = ax1 + (ax2 - ax1) * da / d;
	*y1 = ay1 + (ay2 - ay1) * da / d;

	G_debug(DLEVEL, "        intersection %.16g, %.16g", *x1, *y1);
	return 1;
    }

    /* segments are parallel or collinear */
    da = DA;
    db = DB;
    if ((da != 0) || (db != 0)) {
	/* segments are parallel */
	G_debug(DLEVEL, "    parallel segments");
	return 0;
    }

    /* segments are colinear. check for overlap */

    G_debug(DLEVEL, "    collinear segments");

    if ((bx2 < ax1) || (bx1 > ax2)) {
	G_debug(DLEVEL, "        no intersection");
	return 0;
    }

    /* there is overlap or connected end points */
    G_debug(DLEVEL, "        overlap");

    /* a contains b */
    if ((ax1 < bx1) && (ax2 > bx2)) {
	G_debug(DLEVEL, "            a contains b");
	if (!vertical) {
	    *x1 = bx1;
	    *y1 = by1;
	    *x2 = bx2;
	    *y2 = by2;
	}
	else {
	    *x1 = by1;
	    *y1 = bx1;
	    *x2 = by2;
	    *y2 = bx2;
	}
	return 3;
    }

    /* b contains a */
    if ((ax1 > bx1) && (ax2 < bx2)) {
	G_debug(DLEVEL, "            b contains a");
	if (!vertical) {
	    *x1 = bx1;
	    *y1 = by1;
	    *x2 = bx2;
	    *y2 = by2;
	}
	else {
	    *x1 = by1;
	    *y1 = bx1;
	    *x2 = by2;
	    *y2 = bx2;
	}
	return 4;
    }

    /* general overlap, 2 intersection points */
    G_debug(DLEVEL, "        partial overlap");
    if ((bx1 > ax1) && (bx1 < ax2)) {	/* b1 is in a */
	if (!vertical) {
	    *x1 = bx1;
	    *y1 = by1;
	    *x2 = ax2;
	    *y2 = ay2;
	}
	else {
	    *x1 = by1;
	    *y1 = bx1;
	    *x2 = ay2;
	    *y2 = ax2;
	}
	return 2;
    }
    if ((bx2 > ax1) && (bx2 < ax2)) {	/* b2 is in a */
	if (!vertical) {
	    *x1 = bx2;
	    *y1 = by2;
	    *x2 = ax1;
	    *y2 = ay1;
	}
	else {
	    *x1 = by2;
	    *y1 = bx2;
	    *x2 = ay1;
	    *y2 = ax1;
	}
	return 2;
    }

    /* should not be reached */
    G_warning(("segment_intersection_2d() ERROR (should not be reached)"));
    G_warning("%.16g %.16g", ax1, ay1);
    G_warning("%.16g %.16g", ax2, ay2);
    G_warning("x");
    G_warning("%.16g %.16g", bx1, by1);
    G_warning("%.16g %.16g", bx2, by2);

    return 0;
}

#define N 52			/* double's mantisa size in bits */
/* a and b are different in at most <bits> significant digits */
int almost_equal(double a, double b, int bits)
{
    int ea, eb, e;

    if (a == b)
	return 1;

    if (a == 0 || b == 0) {
	/* return (0 < -N+bits); */
	return (bits > N);
    }

    frexp(a, &ea);
    frexp(b, &eb);
    if (ea != eb)
	return (bits > N + abs(ea - eb));
    frexp(a - b, &e);
    return (e < ea - N + bits);
}

#ifdef ASDASDFASFEAS
int segment_intersection_2d_test(double ax1, double ay1, double ax2,
				 double ay2, double bx1, double by1,
				 double bx2, double by2, double *x1,
				 double *y1, double *x2, double *y2)
{
    double t;

    double max_ax, min_ax, max_ay, min_ay;

    double max_bx, min_bx, max_by, min_by;

    int sgn_d, sgn_da, sgn_db;

    int vertical;

    int f11, f12, f21, f22;

    mp_exp_t exp;

    char *s;

    double d, da, db, ra, rb;

    if (!initialized)
	initialize_mpf_vars();

    /* TODO: Works for points ? */
    G_debug(3, "segment_intersection_2d_test()");
    G_debug(3, "    ax1  = %.18e, ay1  = %.18e", ax1, ay1);
    G_debug(3, "    ax2  = %.18e, ay2  = %.18e", ax2, ay2);
    G_debug(3, "    bx1  = %.18e, by1  = %.18e", bx1, by1);
    G_debug(3, "    bx2  = %.18e, by2  = %.18e", bx2, by2);

    f11 = ((ax1 == bx1) && (ay1 == by1));
    f12 = ((ax1 == bx2) && (ay1 == by2));
    f21 = ((ax2 == bx1) && (ay2 == by1));
    f22 = ((ax2 == bx2) && (ay2 == by2));

    /* Check for identical segments */
    if ((f11 && f22) || (f12 && f21)) {
	G_debug(4, "    identical segments");
	*x1 = ax1;
	*y1 = ay1;
	*x2 = ax2;
	*y2 = ay2;
	return 5;
    }
    /* Check for identical endpoints */
    if (f11 || f12) {
	G_debug(4, "    connected by endpoints");
	*x1 = ax1;
	*y1 = ay1;
	return 1;
    }
    if (f21 || f22) {
	G_debug(4, "    connected by endpoints");
	*x1 = ax2;
	*y1 = ay2;
	return 1;
    }

    if ((MAX(ax1, ax2) < MIN(bx1, bx2)) || (MAX(bx1, bx2) < MIN(ax1, ax2))) {
	G_debug(4, "    no intersection (disjoint bounding boxes)");
	return 0;
    }
    if ((MAX(ay1, ay2) < MIN(by1, by2)) || (MAX(by1, by2) < MIN(ay1, ay2))) {
	G_debug(4, "    no intersection (disjoint bounding boxes)");
	return 0;
    }

    d = (ax2 - ax1) * (by1 - by2) - (ay2 - ay1) * (bx1 - bx2);
    da = (bx1 - ax1) * (by1 - by2) - (by1 - ay1) * (bx1 - bx2);
    db = (ax2 - ax1) * (by1 - ay1) - (ay2 - ay1) * (bx1 - ax1);

    det22(dd, ax2, ax1, bx1, bx2, ay2, ay1, by1, by2);
    sgn_d = mpf_sgn(dd);
    s = mpf_get_str(NULL, &exp, 10, 40, dd);
    G_debug(3, "    dd = %sE%d", (s[0] == 0) ? "0" : s, exp);
    G_debug(3, "    d = %.18E", d);

    if (sgn_d != 0) {
	G_debug(3, "    general position");

	det22(dda, bx1, ax1, bx1, bx2, by1, ay1, by1, by2);
	det22(ddb, ax2, ax1, bx1, ax1, ay2, ay1, by1, ay1);
	sgn_da = mpf_sgn(dda);
	sgn_db = mpf_sgn(ddb);

	ra = da / d;
	rb = db / d;
	mpf_div(rra, dda, dd);
	mpf_div(rrb, ddb, dd);

	s = mpf_get_str(NULL, &exp, 10, 40, rra);
	G_debug(4, "        rra = %sE%d", (s[0] == 0) ? "0" : s, exp);
	G_debug(4, "        ra = %.18E", ra);
	s = mpf_get_str(NULL, &exp, 10, 40, rrb);
	G_debug(4, "        rrb = %sE%d", (s[0] == 0) ? "0" : s, exp);
	G_debug(4, "        rb = %.18E", rb);

	if (sgn_d > 0) {
	    if ((sgn_da < 0) || (mpf_cmp(dda, dd) > 0)) {
		G_debug(DLEVEL, "        no intersection");
		return 0;
	    }

	    if ((sgn_db < 0) || (mpf_cmp(ddb, dd) > 0)) {
		G_debug(DLEVEL, "        no intersection");
		return 0;
	    }
	}
	else {			/* if sgn_d < 0 */
	    if ((sgn_da > 0) || (mpf_cmp(dda, dd) < 0)) {
		G_debug(DLEVEL, "        no intersection");
		return 0;
	    }

	    if ((sgn_db > 0) || (mpf_cmp(ddb, dd) < 0)) {
		G_debug(DLEVEL, "        no intersection");
		return 0;
	    }
	}

	mpf_set_d(delta, ax2 - ax1);
	mpf_mul(t1, dda, delta);
	mpf_div(t2, t1, dd);
	*x1 = ax1 + mpf_get_d(t2);

	mpf_set_d(delta, ay2 - ay1);
	mpf_mul(t1, dda, delta);
	mpf_div(t2, t1, dd);
	*y1 = ay1 + mpf_get_d(t2);

	G_debug(2, "        intersection at:");
	G_debug(2, "            xx = %.18e", *x1);
	G_debug(2, "             x = %.18e", ax1 + ra * (ax2 - ax1));
	G_debug(2, "            yy = %.18e", *y1);
	G_debug(2, "             y = %.18e", ay1 + ra * (ay2 - ay1));
	return 1;
    }

    G_debug(3, "    parallel/collinear...");
    return -1;
}
#endif
