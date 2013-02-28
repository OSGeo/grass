/*!
   \file lib/vector/Vlib/intersect.c

   \brief Vector library - intersection

   Higher level functions for reading/writing/manipulating vectors.

   Some parts of code taken from grass50 v.spag/linecros.c

   Based on the following:

   <code>
   (ax2-ax1)r1 - (bx2-bx1)r2 = ax2 - ax1
   (ay2-ay1)r1 - (by2-by1)r2 = ay2 - ay1
   </code>

   Solving for r1 and r2, if r1 and r2 are between 0 and 1, then line
   segments (ax1,ay1)(ax2,ay2) and (bx1,by1)(bx2,by2) intersect.

   Intersect 2 line segments.

   Returns: 0 - do not intersect
   1 - intersect at one point
   <pre>
   \  /    \  /  \  /
   \/      \/    \/
   /\             \
   /  \             \
   2 - partial overlap         ( \/                      )
   ------      a          (    distance < threshold )
   ------   b          (                         )
   3 - a contains b            ( /\                      )
   ----------  a    ----------- a
   ----     b          ----- b
   4 - b contains a
   ----     a          ----- a
   ----------  b    ----------- b
   5 - identical
   ----------  a
   ----------  b
   </pre>
   Intersection points: 
   <pre>
   return  point1 breakes: point2 breaks:    distance1 on:   distance2 on:
   0        -              -                  -              -  
   1        a,b            -                  a              b
   2        a              b                  a              b
   3        a              a                  a              a
   4        b              b                  b              b
   5        -              -                  -              -
   </pre>
   Sometimes (often) is important to get the same coordinates for a x
   b and b x a.  To reach this, the segments a,b are 'sorted' at the
   beginning, so that for the same switched segments, results are
   identical. (reason is that double values are always rounded because
   of limited number of decimal places and for different order of
   coordinates, the results would be different)

   (C) 2001-2009 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2).  Read the file COPYING that comes with GRASS for details.

   \author Original author CERL, probably Dave Gerdes or Mike Higgins.
   \author Update to GRASS 5.7 Radim Blazek.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <grass/vector.h>
#include <grass/glocale.h>

/* function prototypes */
static int cmp_cross(const void *pa, const void *pb);
static void add_cross(int asegment, double adistance, int bsegment,
		      double bdistance, double x, double y);
static double dist2(double x1, double y1, double x2, double y2);

static int debug_level = -1;

#if 0
static int ident(double x1, double y1, double x2, double y2, double thresh);
#endif
static int cross_seg(int id, const struct RTree_Rect *rect, int *arg);
static int find_cross(int id, const struct RTree_Rect *rect, int *arg);

#define D  ((ax2-ax1)*(by1-by2) - (ay2-ay1)*(bx1-bx2))
#define D1 ((bx1-ax1)*(by1-by2) - (by1-ay1)*(bx1-bx2))
#define D2 ((ax2-ax1)*(by1-ay1) - (ay2-ay1)*(bx1-ax1))

/*!
 * \brief Check for intersect of 2 line segments.
 *
 * \param ax1,ay1,az1,ax2,ay2,az2 input line a
 * \param bx1,by1,bz1,bx2,by2,bz2 input line b
 * \param[out] x1,y1,z1 intersection point1 (case 2-4)
 * \param[out] x2,y2,z2 intersection point2 (case 2-4)
 * \param with_z use z coordinate (3D) (TODO)
 *
 * \return 0 - do not intersect,
 * \return 1 - intersect at one point,
 * \return 2 - partial overlap,
 * \return 3 - a contains b,
 * \return 4 - b contains a,
 * \return 5 - identical
 */
int Vect_segment_intersection(double ax1, double ay1, double az1, double ax2,
			      double ay2, double az2, double bx1, double by1,
			      double bz1, double bx2, double by2, double bz2,
			      double *x1, double *y1, double *z1, double *x2,
			      double *y2, double *z2, int with_z)
{
    static int first_3d = 1;
    double d, d1, d2, r1, r2, dtol, t;
    int switched = 0;

    /* TODO: Works for points ? */

    G_debug(4, "Vect_segment_intersection()");
    G_debug(4, "    %.15g , %.15g  - %.15g , %.15g", ax1, ay1, ax2, ay2);
    G_debug(4, "    %.15g , %.15g  - %.15g , %.15g", bx1, by1, bx2, by2);

    /* TODO 3D */
    if (with_z && first_3d) {
	G_warning(_("3D not supported by Vect_segment_intersection()"));
	first_3d = 0;
    }

    /* Check identical lines */
    if ((ax1 == bx1 && ay1 == by1 && ax2 == bx2 && ay2 == by2) ||
	(ax1 == bx2 && ay1 == by2 && ax2 == bx1 && ay2 == by1)) {
	G_debug(2, " -> identical segments");
	*x1 = ax1;
	*y1 = ay1;
	*z1 = az1;
	*x2 = ax2;
	*y2 = ay2;
	*z2 = az2;
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

    d = D;
    d1 = D1;
    d2 = D2;

    G_debug(2, "Vect_segment_intersection(): d = %f, d1 = %f, d2 = %f", d, d1,
	    d2);

    /* TODO: dtol was originaly set to 1.0e-10, which was usualy working but not always. 
     *       Can it be a problem to set the tolerance to 0.0 ? */
    dtol = 0.0;
    if (fabs(d) > dtol) {
	r1 = d1 / d;
	r2 = d2 / d;

	G_debug(2, " -> not parallel/collinear: r1 = %f, r2 = %f", r1, r2);

	if (r1 < 0 || r1 > 1 || r2 < 0 || r2 > 1) {
	    G_debug(2, "  -> no intersection");
	    return 0;
	}

	*x1 = ax1 + r1 * (ax2 - ax1);
	*y1 = ay1 + r1 * (ay2 - ay1);
	*z1 = 0;

	G_debug(2, "  -> intersection %f, %f", *x1, *y1);
	return 1;
    }

    /* segments are parallel or collinear */
    G_debug(3, " -> parallel/collinear");

    if (d1 || d2) {		/* lines are parallel */
	G_debug(2, "  -> parallel");
	return 0;
    }

    /* segments are colinear. check for overlap */

    /* Collinear vertical */
    /* original code assumed lines were not both vertical
     *  so there is a special case if they are */
    if (ax1 == ax2 && bx1 == bx2 && ax1 == bx1) {
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
	if (ay1 == by2) {
	    *x1 = ax1;
	    *y1 = ay1;
	    *z1 = 0;
	    G_debug(2, "   -> connected by end points");
	    return 1;		/* endpoints only */
	}
	if (ay2 == by1) {
	    *x1 = ax2;
	    *y1 = ay2;
	    *z1 = 0;
	    G_debug(2, "    -> connected by end points");
	    return 1;		/* endpoints only */
	}

	/* general overlap */
	G_debug(3, "   -> vertical overlap");
	/* a contains b */
	if (ay1 <= by1 && ay2 >= by2) {
	    G_debug(2, "    -> a contains b");
	    *x1 = bx1;
	    *y1 = by1;
	    *z1 = 0;
	    *x2 = bx2;
	    *y2 = by2;
	    *z2 = 0;
	    if (!switched)
		return 3;

	    return 4;
	}
	/* b contains a */
	if (ay1 >= by1 && ay2 <= by2) {
	    G_debug(2, "    -> b contains a");
	    *x1 = ax1;
	    *y1 = ay1;
	    *z1 = 0;
	    *x2 = ax2;
	    *y2 = ay2;
	    *z2 = 0;
	    if (!switched)
		return 4;

	    return 3;
	}

	/* general overlap, 2 intersection points */
	G_debug(2, "    -> partial overlap");
	if (by1 > ay1 && by1 < ay2) {	/* b1 in a */
	    if (!switched) {
		*x1 = bx1;
		*y1 = by1;
		*z1 = 0;
		*x2 = ax2;
		*y2 = ay2;
		*z2 = 0;
	    }
	    else {
		*x1 = ax2;
		*y1 = ay2;
		*z1 = 0;
		*x2 = bx1;
		*y2 = by1;
		*z2 = 0;
	    }
	    return 2;
	}
	if (by2 > ay1 && by2 < ay2) {	/* b2 in a */
	    if (!switched) {
		*x1 = bx2;
		*y1 = by2;
		*z1 = 0;
		*x2 = ax1;
		*y2 = ay1;
		*z2 = 0;
	    }
	    else {
		*x1 = ax1;
		*y1 = ay1;
		*z1 = 0;
		*x2 = bx2;
		*y2 = by2;
		*z2 = 0;
	    }
	    return 2;
	}

	/* should not be reached */
	G_warning(_("Vect_segment_intersection() ERROR (collinear vertical segments)"));
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
	*z1 = 0;
	G_debug(2, "    -> connected by end points");
	return 1;
    }
    if ((ax2 == bx1 && ay2 == by1) || (ax2 == bx2 && ay2 == by2)) {
	*x1 = ax2;
	*y1 = ay2;
	*z1 = 0;
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
	*z1 = 0;
	*x2 = bx2;
	*y2 = by2;
	*z2 = 0;
	if (!switched)
	    return 3;

	return 4;
    }
    /* b contains a */
    if (ax1 >= bx1 && ax2 <= bx2) {
	G_debug(2, "    -> b contains a");
	*x1 = ax1;
	*y1 = ay1;
	*z1 = 0;
	*x2 = ax2;
	*y2 = ay2;
	*z2 = 0;
	if (!switched)
	    return 4;

	return 3;
    }

    /* general overlap, 2 intersection points (lines are not vertical) */
    G_debug(2, "    -> partial overlap");
    if (bx1 > ax1 && bx1 < ax2) {	/* b1 is in a */
	if (!switched) {
	    *x1 = bx1;
	    *y1 = by1;
	    *z1 = 0;
	    *x2 = ax2;
	    *y2 = ay2;
	    *z2 = 0;
	}
	else {
	    *x1 = ax2;
	    *y1 = ay2;
	    *z1 = 0;
	    *x2 = bx1;
	    *y2 = by1;
	    *z2 = 0;
	}
	return 2;
    }
    if (bx2 > ax1 && bx2 < ax2) {	/* b2 is in a */
	if (!switched) {
	    *x1 = bx2;
	    *y1 = by2;
	    *z1 = 0;
	    *x2 = ax1;
	    *y2 = ay1;
	    *z2 = 0;
	}
	else {
	    *x1 = ax1;
	    *y1 = ay1;
	    *z1 = 0;
	    *x2 = bx2;
	    *y2 = by2;
	    *z2 = 0;
	}
	return 2;
    }

    /* should not be reached */
    G_warning(_("Vect_segment_intersection() ERROR (collinear non vertical segments)"));
    G_warning("%.15g %.15g", ax1, ay1);
    G_warning("%.15g %.15g", ax2, ay2);
    G_warning("x");
    G_warning("%.15g %.15g", bx1, by1);
    G_warning("%.15g %.15g", bx2, by2);

    return 0;
}

typedef struct
{				/* in arrays 0 - A line , 1 - B line */
    int segment[2];		/* segment number, start from 0 for first */
    double distance[2];
    double x, y, z;
} CROSS;

/* Current line in arrays is for some functions like cmp() set by: */
static int current;
static int second;		/* line which is not current */

static int a_cross = 0;
static int n_cross;
static CROSS *cross = NULL;
static int *use_cross = NULL;

static void add_cross(int asegment, double adistance, int bsegment,
		      double bdistance, double x, double y)
{
    if (n_cross == a_cross) {
	/* Must be space + 1, used later for last line point, do it better */
	cross =
	    (CROSS *) G_realloc((void *)cross,
				(a_cross + 101) * sizeof(CROSS));
	use_cross =
	    (int *)G_realloc((void *)use_cross,
			     (a_cross + 101) * sizeof(int));
	a_cross += 100;
    }

    G_debug(5,
	    "  add new cross: aseg/dist = %d/%f bseg/dist = %d/%f, x = %f y = %f",
	    asegment, adistance, bsegment, bdistance, x, y);
    cross[n_cross].segment[0] = asegment;
    cross[n_cross].distance[0] = adistance;
    cross[n_cross].segment[1] = bsegment;
    cross[n_cross].distance[1] = bdistance;
    cross[n_cross].x = x;
    cross[n_cross].y = y;
    n_cross++;
}

static int cmp_cross(const void *pa, const void *pb)
{
    CROSS *p1 = (CROSS *) pa;
    CROSS *p2 = (CROSS *) pb;

    if (p1->segment[current] < p2->segment[current])
	return -1;
    if (p1->segment[current] > p2->segment[current])
	return 1;
    /* the same segment */
    if (p1->distance[current] < p2->distance[current])
	return -1;
    if (p1->distance[current] > p2->distance[current])
	return 1;
    return 0;
}

static double dist2(double x1, double y1, double x2, double y2)
{
    double dx, dy;

    dx = x2 - x1;
    dy = y2 - y1;
    return (dx * dx + dy * dy);
}

#if 0
/* returns 1 if points are identical */
static int ident(double x1, double y1, double x2, double y2, double thresh)
{
    double dx, dy;

    dx = x2 - x1;
    dy = y2 - y1;
    if ((dx * dx + dy * dy) <= thresh * thresh)
	return 1;

    return 0;
}
#endif

/* shared by Vect_line_intersection, Vect_line_check_intersection, cross_seg, find_cross */
static struct line_pnts *APnts, *BPnts;

/* break segments (called by rtree search) */
static int cross_seg(int id, const struct RTree_Rect *rect, int *arg)
{
    double x1, y1, z1, x2, y2, z2;
    int i, j, ret;

    /* !!! segment number for B lines is returned as +1 */
    i = *arg;
    j = id - 1;
    /* Note: -1 to make up for the +1 when data was inserted */

    ret = Vect_segment_intersection(APnts->x[i], APnts->y[i], APnts->z[i],
				    APnts->x[i + 1], APnts->y[i + 1],
				    APnts->z[i + 1], BPnts->x[j], BPnts->y[j],
				    BPnts->z[j], BPnts->x[j + 1],
				    BPnts->y[j + 1], BPnts->z[j + 1], &x1,
				    &y1, &z1, &x2, &y2, &z2, 0);

    /* add ALL (including end points and duplicates), clean later */
    if (ret > 0) {
	G_debug(2, "  -> %d x %d: intersection type = %d", i, j, ret);
	if (ret == 1) {		/* one intersection on segment A */
	    G_debug(3, "    in %f, %f ", x1, y1);
	    add_cross(i, 0.0, j, 0.0, x1, y1);
	}
	else if (ret == 2 || ret == 3 || ret == 4 || ret == 5) {
	    /*  partial overlap; a broken in one, b broken in one
	     *  or a contains b; a is broken in 2 points (but 1 may be end)
	     *  or b contains a; b is broken in 2 points (but 1 may be end) 
	     *  or identical */
	    G_debug(3, "    in %f, %f; %f, %f", x1, y1, x2, y2);
	    add_cross(i, 0.0, j, 0.0, x1, y1);
	    add_cross(i, 0.0, j, 0.0, x2, y2);
	}
    }
    return 1;			/* keep going */
}

/*!
 * \brief Intersect 2 lines.
 *
 * Creates array of new lines created from original A line, by
 * intersection with B line. Points (Points->n_points == 1) are not
 * supported.
 *
 * \param APoints first input line 
 * \param BPoints second input line 
 * \param[out] ALines array of new lines created from original A line
 * \param[out] BLines array of new lines created from original B line
 * \param[out] nalines number of new lines (ALines)
 * \param[out] nblines number of new lines (BLines)
 * \param with_z 3D, not supported!
 *
 * \return 0 no intersection 
 * \return 1 intersection found
 */
int
Vect_line_intersection(struct line_pnts *APoints,
		       struct line_pnts *BPoints,
		       struct bound_box *ABox,
		       struct bound_box *BBox,
		       struct line_pnts ***ALines,
		       struct line_pnts ***BLines,
		       int *nalines, int *nblines, int with_z)
{
    int i, j, k, l, last_seg, seg, last;
    int n_alive_cross;
    double dist, curdist, last_x, last_y, last_z;
    double x, y, rethresh;
    struct line_pnts **XLines, *Points;
    struct RTree *MyRTree;
    struct line_pnts *Points1, *Points2;	/* first, second points */
    int seg1, seg2, vert1, vert2;
    static struct RTree_Rect rect;
    static int rect_init = 0;
    struct bound_box box, abbox;

    if (debug_level == -1) {
	const char *dstr = G__getenv("DEBUG");

	if (dstr != NULL)
	    debug_level = atoi(dstr);
	else
	    debug_level = 0;
    }

    if (!rect_init) {
	rect.boundary = G_malloc(6 * sizeof(RectReal));
	rect_init = 6;
    }

    n_cross = 0;
    rethresh = 0.000001;	/* TODO */
    APnts = APoints;
    BPnts = BPoints;

    /* RE (representation error).
     *  RE thresh above is nonsense of course, the RE threshold should be based on
     *  number of significant digits for double (IEEE-754) which is 15 or 16 and exponent. 
     *  The number above is in fact not required threshold, and will not work
     *  for example: equator length is 40.075,695 km (8 digits), units are m (+3) 
     *  and we want precision in mm (+ 3) = 14 -> minimum rethresh may be around 0.001
     *  ?Maybe all nonsense? */

    /* Warning: This function is also used to intersect the line by itself i.e. APoints and
     * BPoints are identical. I am not sure if it is clever, but it seems to work, but
     * we have to keep this in mind and handle some special cases (maybe) */

    /* TODO: 3D, RE threshold, GV_POINTS (line x point) */

    /* Take each segment from A and intersect by each segment from B.
     *  
     *  All intersections are found first and saved to array, then sorted by a distance along the line,
     *  and then the line is split to pieces.
     *
     *  Note: If segments are collinear, check if previous/next segments are also collinear, 
     *  in that case do not break:
     *  +----------+  
     *  +----+-----+  etc.
     *  doesn't need to be broken 
     *
     *  Note: If 2 adjacent segments of line B have common vertex exactly (or within thresh) on line A,
     *  intersection points for these B segments may differ due to RE:
     *  ------------ a       ----+--+----            ----+--+----
     *      /\         =>       /    \     or maybe       \/
     *  b0 /  \ b1             /      \      even:        /\     
     *
     *  -> solution: snap all breaks to nearest vertices first within RE threshold
     *  
     *  Question: Snap all breaks to each other within RE threshold?
     *
     *  Note: If a break is snapped to end point or two breaks are snapped to the same vertex
     *        resulting new line is degenerated => before line is added to array, it must be checked
     *        if line is not degenerated
     *
     *  Note: to snap to vertices is important for cases where line A is broken by B and C line
     *  at the same point:
     *   \  /  b   no snap     \    /
     *    \/       could    ----+--+----
     *  ------ a   result   
     *    /\       in ?:         /\
     *   /  \  c                /  \
     * 
     *  Note: once we snap breaks to vertices, we have to do that for both lines A and B in the same way
     *  and because we cannot be sure that A childrens will not change a bit by break(s) 
     *  we have to break both A and B  at once i.e. in one Vect_line_intersection () call.
     */

    /* Spatial index: lines may be very long (thousands of segments) and check each segment 
     *  with each from second line takes a long time (n*m). Because of that, spatial index
     *  is build first for the second line and segments from the first line are broken by segments
     *  in bound box */

    if (!Vect_box_overlap(ABox, BBox)) {
	*nalines = 0;
	*nblines = 0;
	return 0;
    }
    
    abbox = *BBox;
    if (abbox.N > ABox->N)
	abbox.N = ABox->N;
    if (abbox.S < ABox->S)
	abbox.S = ABox->S;
    if (abbox.E > ABox->E)
	abbox.E = ABox->E;
    if (abbox.W < ABox->W)
	abbox.W = ABox->W;
    if (abbox.T > ABox->T)
	abbox.T = ABox->T;
    if (abbox.B < ABox->B)
	abbox.B = ABox->B;

    abbox.N += rethresh;
    abbox.S -= rethresh;
    abbox.E += rethresh;
    abbox.W -= rethresh;
    abbox.T += rethresh;
    abbox.B -= rethresh;
    
    /* Create rtree for B line */
    MyRTree = RTreeCreateTree(-1, 0, 2);
    RTreeSetOverflow(MyRTree, 0);
    for (i = 0; i < BPoints->n_points - 1; i++) {
	if (BPoints->x[i] <= BPoints->x[i + 1]) {
	    rect.boundary[0] = BPoints->x[i];
	    rect.boundary[3] = BPoints->x[i + 1];

	    box.W = BPoints->x[i];
	    box.E = BPoints->x[i + 1];
	}
	else {
	    rect.boundary[0] = BPoints->x[i + 1];
	    rect.boundary[3] = BPoints->x[i];

	    box.W = BPoints->x[i + 1];
	    box.E = BPoints->x[i];
	}

	if (BPoints->y[i] <= BPoints->y[i + 1]) {
	    rect.boundary[1] = BPoints->y[i];
	    rect.boundary[4] = BPoints->y[i + 1];

	    box.S = BPoints->y[i];
	    box.N = BPoints->y[i + 1];
	}
	else {
	    rect.boundary[1] = BPoints->y[i + 1];
	    rect.boundary[4] = BPoints->y[i];

	    box.S = BPoints->y[i + 1];
	    box.N = BPoints->y[i];
	}

	if (BPoints->z[i] <= BPoints->z[i + 1]) {
	    rect.boundary[2] = BPoints->z[i];
	    rect.boundary[5] = BPoints->z[i + 1];

	    box.B = BPoints->z[i];
	    box.T = BPoints->z[i + 1];
	}
	else {
	    rect.boundary[2] = BPoints->z[i + 1];
	    rect.boundary[5] = BPoints->z[i];

	    box.B = BPoints->z[i + 1];
	    box.T = BPoints->z[i];
	}
	box.N += rethresh;
	box.S -= rethresh;
	box.E += rethresh;
	box.W -= rethresh;
	box.T += rethresh;
	box.B -= rethresh;

	if (Vect_box_overlap(&abbox, &box)) {
	    RTreeInsertRect(&rect, i + 1, MyRTree);	/* B line segment numbers in rtree start from 1 */
	}
    }

    /* Break segments in A by segments in B */
    for (i = 0; i < APoints->n_points - 1; i++) {
	if (APoints->x[i] <= APoints->x[i + 1]) {
	    rect.boundary[0] = APoints->x[i];
	    rect.boundary[3] = APoints->x[i + 1];
	}
	else {
	    rect.boundary[0] = APoints->x[i + 1];
	    rect.boundary[3] = APoints->x[i];
	}

	if (APoints->y[i] <= APoints->y[i + 1]) {
	    rect.boundary[1] = APoints->y[i];
	    rect.boundary[4] = APoints->y[i + 1];
	}
	else {
	    rect.boundary[1] = APoints->y[i + 1];
	    rect.boundary[4] = APoints->y[i];
	}
	if (APoints->z[i] <= APoints->z[i + 1]) {
	    rect.boundary[2] = APoints->z[i];
	    rect.boundary[5] = APoints->z[i + 1];
	}
	else {
	    rect.boundary[2] = APoints->z[i + 1];
	    rect.boundary[5] = APoints->z[i];
	}

	j = RTreeSearch(MyRTree, &rect, (void *)cross_seg, &i);	/* A segment number from 0 */
    }

    /* Free RTree */
    RTreeDestroyTree(MyRTree);

    G_debug(2, "n_cross = %d", n_cross);
    /* Lines do not cross each other */
    if (n_cross == 0) {
	*nalines = 0;
	*nblines = 0;
	return 0;
    }

    /* Snap breaks to nearest vertices within RE threshold */
    /* Calculate distances along segments */
    for (i = 0; i < n_cross; i++) {
	/* 1. of A seg */
	seg = cross[i].segment[0];
	curdist =
	    dist2(cross[i].x, cross[i].y, APoints->x[seg], APoints->y[seg]);
	x = APoints->x[seg];
	y = APoints->y[seg];

	cross[i].distance[0] = curdist;

	/* 2. of A seg */
	dist =
	    dist2(cross[i].x, cross[i].y, APoints->x[seg + 1],
		  APoints->y[seg + 1]);
	if (dist < curdist) {
	    curdist = dist;
	    x = APoints->x[seg + 1];
	    y = APoints->y[seg + 1];
	}

	/* 1. of B seg */
	seg = cross[i].segment[1];
	dist =
	    dist2(cross[i].x, cross[i].y, BPoints->x[seg], BPoints->y[seg]);
	cross[i].distance[1] = dist;

	if (dist < curdist) {
	    curdist = dist;
	    x = BPoints->x[seg];
	    y = BPoints->y[seg];
	}
	/* 2. of B seg */
	dist = dist2(cross[i].x, cross[i].y, BPoints->x[seg + 1], BPoints->y[seg + 1]);
	if (dist < curdist) {
	    curdist = dist;
	    x = BPoints->x[seg + 1];
	    y = BPoints->y[seg + 1];
	}
	if (curdist < rethresh * rethresh) {
	    cross[i].x = x;
	    cross[i].y = y;

	    /* Update distances along segments */
	    seg = cross[i].segment[0];
	    cross[i].distance[0] =
		dist2(APoints->x[seg], APoints->y[seg], cross[i].x, cross[i].y);
	    seg = cross[i].segment[1];
	    cross[i].distance[1] =
		dist2(BPoints->x[seg], BPoints->y[seg], cross[i].x, cross[i].y);
	}
    }

    /* l = 1 ~ line A, l = 2 ~ line B */
    for (l = 1; l < 3; l++) {
	for (i = 0; i < n_cross; i++)
	    use_cross[i] = 1;

	/* Create array of lines */
	XLines = G_malloc((n_cross + 1) * sizeof(struct line_pnts *));

	if (l == 1) {
	    G_debug(2, "Clean and create array for line A");
	    Points = APoints;
	    Points1 = APoints;
	    Points2 = BPoints;
	    current = 0;
	    second = 1;
	}
	else {
	    G_debug(2, "Clean and create array for line B");
	    Points = BPoints;
	    Points1 = BPoints;
	    Points2 = APoints;
	    current = 1;
	    second = 0;
	}

	/* Sort points along lines */
	qsort((void *)cross, sizeof(char) * n_cross, sizeof(CROSS),
	      cmp_cross);

	/* Print all (raw) breaks */
	    /* avoid loop when not debugging */
	if (debug_level > 2) {
	    for (i = 0; i < n_cross; i++) {
		G_debug(3,
			"  cross = %d seg1/dist1 = %d/%f seg2/dist2 = %d/%f x = %f y = %f",
			i, cross[i].segment[current],
			sqrt(cross[i].distance[current]),
			cross[i].segment[second], sqrt(cross[i].distance[second]),
			cross[i].x, cross[i].y);
	    }
	}

	/* Remove breaks on first/last line vertices */
	for (i = 0; i < n_cross; i++) {
	    if (use_cross[i] == 1) {
		j = Points1->n_points - 1;

		/* Note: */
		if ((cross[i].segment[current] == 0 &&
		     cross[i].x == Points1->x[0] &&
		     cross[i].y == Points1->y[0]) ||
		    (cross[i].segment[current] == j - 1 &&
		     cross[i].x == Points1->x[j] &&
		     cross[i].y == Points1->y[j])) {
		    use_cross[i] = 0;	/* first/last */
		    G_debug(3, "cross %d deleted (first/last point)", i);
		}
	    }
	}

	/* Remove breaks with collinear previous and next segments on 1 and 2 */
	/* Note: breaks with collinear previous and nex must be remove duplicates,
	 *        otherwise some cross may be lost. Example (+ is vertex):
	 *             B          first cross intersections: A/B  segment:
	 *             |               0/0, 0/1, 1/0, 1/1 - collinear previous and next
	 *     AB -----+----+--- A     0/4, 0/5, 1/4, 1/5 - OK        
	 *              \___|                   
	 *                B                    
	 *  This should not inluence that break is always on first segment, see below (I hope)
	 */
	/* TODO: this doesn't find identical with breaks on revious/next */
	for (i = 0; i < n_cross; i++) {
	    if (use_cross[i] == 0)
		continue;
	    G_debug(3, "  is %d between colinear?", i);

	    seg1 = cross[i].segment[current];
	    seg2 = cross[i].segment[second];

	    /* Is it vertex on 1, which? */
	    if (cross[i].x == Points1->x[seg1] &&
		cross[i].y == Points1->y[seg1]) {
		vert1 = seg1;
	    }
	    else if (cross[i].x == Points1->x[seg1 + 1] &&
		     cross[i].y == Points1->y[seg1 + 1]) {
		vert1 = seg1 + 1;
	    }
	    else {
		G_debug(3, "  -> is not vertex on 1. line");
		continue;
	    }

	    /* Is it vertex on 2, which? */
	    /* For 1. line it is easy, because breaks on vertex are always at end vertex
	     *  for 2. line we need to find which vertex is on break if any (vert2 starts from 0) */
	    if (cross[i].x == Points2->x[seg2] &&
		cross[i].y == Points2->y[seg2]) {
		vert2 = seg2;
	    }
	    else if (cross[i].x == Points2->x[seg2 + 1] &&
		     cross[i].y == Points2->y[seg2 + 1]) {
		vert2 = seg2 + 1;
	    }
	    else {
		G_debug(3, "  -> is not vertex on 2. line");
		continue;
	    }
	    G_debug(3, "    seg1/vert1 = %d/%d  seg2/ver2 = %d/%d", seg1,
		    vert1, seg2, vert2);

	    /* Check if the second vertex is not first/last */
	    if (vert2 == 0 || vert2 == Points2->n_points - 1) {
		G_debug(3, "  -> vertex 2 (%d) is first/last", vert2);
		continue;
	    }

	    /* Are there first vertices of this segment identical */
	    if (!((Points1->x[vert1 - 1] == Points2->x[vert2 - 1] &&
		   Points1->y[vert1 - 1] == Points2->y[vert2 - 1] &&
		   Points1->x[vert1 + 1] == Points2->x[vert2 + 1] &&
		   Points1->y[vert1 + 1] == Points2->y[vert2 + 1]) ||
		  (Points1->x[vert1 - 1] == Points2->x[vert2 + 1] &&
		   Points1->y[vert1 - 1] == Points2->y[vert2 + 1] &&
		   Points1->x[vert1 + 1] == Points2->x[vert2 - 1] &&
		   Points1->y[vert1 + 1] == Points2->y[vert2 - 1])
		)
		) {
		G_debug(3, "  -> previous/next are not identical");
		continue;
	    }

	    use_cross[i] = 0;

	    G_debug(3, "    -> collinear -> remove");
	}

	/* Remove duplicates, i.e. merge all identical breaks to one.
	 *  We must be careful because two points with identical coordinates may be distant if measured along
	 *  the line:
	 *       |         Segments b0 and b1 overlap, b0 runs up, b1 down.
	 *       |         Two inersections may be merged for a, because they are identical,
	 *  -----+---- a   but cannot be merged for b, because both b0 and b1 must be broken. 
	 *       |         I.e. Breaks on b have identical coordinates, but there are not identical
	 *       b0 | b1      if measured along line b.
	 *                 
	 *      -> Breaks may be merged as identical if lay on the same segment, or on vertex connecting
	 *      2 adjacent segments the points lay on
	 *      
	 *  Note: if duplicate is on a vertex, the break is removed from next segment =>
	 *        break on vertex is always on first segment of this vertex (used below) 
	 */
	last = -1;
	for (i = 1; i < n_cross; i++) {
	    if (use_cross[i] == 0)
		continue;
	    if (last == -1) {	/* set first alive */
		last = i;
		continue;
	    }
	    seg = cross[i].segment[current];
	    /* compare with last */
	    G_debug(3, "  duplicate ?: cross = %d seg = %d dist = %f", i,
		    cross[i].segment[current], cross[i].distance[current]);
	    if ((cross[i].segment[current] == cross[last].segment[current] &&
		 cross[i].distance[current] == cross[last].distance[current])
		|| (cross[i].segment[current] ==
		    cross[last].segment[current] + 1 &&
		    cross[i].distance[current] == 0 &&
		    cross[i].x == cross[last].x &&
		    cross[i].y == cross[last].y)) {
		G_debug(3, "  cross %d identical to last -> removed", i);
		use_cross[i] = 0;	/* identical */
	    }
	    else {
		last = i;
	    }
	}

	/* Create array of new lines */
	/* Count alive crosses */
	n_alive_cross = 0;
	G_debug(3, "  alive crosses:");
	for (i = 0; i < n_cross; i++) {
	    if (use_cross[i] == 1) {
		G_debug(3, "  %d", i);
		n_alive_cross++;
	    }
	}

	k = 0;
	if (n_alive_cross > 0) {
	    /* Add last line point at the end of cross array (cross alley) */
	    use_cross[n_cross] = 1;
	    j = Points->n_points - 1;
	    cross[n_cross].x = Points->x[j];
	    cross[n_cross].y = Points->y[j];
	    cross[n_cross].segment[current] = Points->n_points - 2;

	    last_seg = 0;
	    last_x = Points->x[0];
	    last_y = Points->y[0];
	    last_z = Points->z[0];
	    /* Go through all cross (+last line point) and create for each new line 
	     *  starting at last_* and ending at cross (last point) */
	    for (i = 0; i <= n_cross; i++) {	/* i.e. n_cross + 1 new lines */
		seg = cross[i].segment[current];
		G_debug(2, "%d seg = %d dist = %f", i, seg,
			cross[i].distance[current]);
		if (use_cross[i] == 0) {
		    G_debug(3, "   removed -> next");
		    continue;
		}

		G_debug(2, " New line:");
		XLines[k] = Vect_new_line_struct();
		/* add last intersection or first point first */
		Vect_append_point(XLines[k], last_x, last_y, last_z);
		G_debug(2, "   append last vert: %f %f", last_x, last_y);

		/* add first points of segments between last and current seg */
		for (j = last_seg + 1; j <= seg; j++) {
		    G_debug(2, "  segment j = %d", j);
		    /* skipp vertex identical to last break */
		    if ((j == last_seg + 1) && Points->x[j] == last_x &&
			Points->y[j] == last_y) {
			G_debug(2, "   -> skip (identical to last break)");
			continue;
		    }
		    Vect_append_point(XLines[k], Points->x[j], Points->y[j],
				      Points->z[j]);
		    G_debug(2, "   append first of seg: %f %f", Points->x[j],
			    Points->y[j]);
		}

		/* add current cross or end point */
		Vect_append_point(XLines[k], cross[i].x, cross[i].y, 0.0);
		G_debug(2, "   append cross / last point: %f %f", cross[i].x,
			cross[i].y);
		last_seg = seg;
		last_x = cross[i].x;
		last_y = cross[i].y, last_z = 0;

		/* Check if line is degenerate */
		if (dig_line_degenerate(XLines[k]) > 0) {
		    G_debug(2, "   line is degenerate -> skipped");
		    Vect_destroy_line_struct(XLines[k]);
		}
		else {
		    k++;
		}
	    }
	}
	if (l == 1) {
	    *nalines = k;
	    *ALines = XLines;
	}
	else {
	    *nblines = k;
	    *BLines = XLines;
	}
    }
    
    /* clean up */

    return 1;
}

static struct line_pnts *APnts, *BPnts, *IPnts;

static int cross_found;		/* set by find_cross() */

/* break segments (called by rtree search) */
static int find_cross(int id, const struct RTree_Rect *rect, int *arg)
{
    double x1, y1, z1, x2, y2, z2;
    int i, j, ret;

    /* !!! segment number for B lines is returned as +1 */
    i = *arg;
    j = id - 1;
    /* Note: -1 to make up for the +1 when data was inserted */

    ret = Vect_segment_intersection(APnts->x[i], APnts->y[i], APnts->z[i],
				    APnts->x[i + 1], APnts->y[i + 1],
				    APnts->z[i + 1], BPnts->x[j], BPnts->y[j],
				    BPnts->z[j], BPnts->x[j + 1],
				    BPnts->y[j + 1], BPnts->z[j + 1], &x1,
				    &y1, &z1, &x2, &y2, &z2, 0);

    if (!IPnts)
	IPnts = Vect_new_line_struct();

    switch (ret) {
    case 0:
    case 5:
	break;
    case 1:
	if (0 > Vect_copy_xyz_to_pnts(IPnts, &x1, &y1, &z1, 1))
	    G_warning(_("Error while adding point to array. Out of memory"));
	break;
    case 2:
    case 3:
    case 4:
	if (0 > Vect_copy_xyz_to_pnts(IPnts, &x1, &y1, &z1, 1))
	    G_warning(_("Error while adding point to array. Out of memory"));
	if (0 > Vect_copy_xyz_to_pnts(IPnts, &x2, &y2, &z2, 1))
	    G_warning(_("Error while adding point to array. Out of memory"));
	break;
    }
    /* add ALL (including end points and duplicates), clean later */
    if (ret > 0) {
	cross_found = 1;
	return 0;
    }
    return 1;			/* keep going */
}

/*!
 * \brief Check if 2 lines intersect.
 *
 * Points (Points->n_points == 1) are also supported.
 *
 * \param APoints first input line 
 * \param BPoints second input line 
 * \param with_z 3D, not supported (only if one or both are points)!
 *
 * \return 0 no intersection 
 * \return 1 intersection found
 */
int
Vect_line_check_intersection(struct line_pnts *APoints,
			     struct line_pnts *BPoints, int with_z)
{
    int i;
    double dist, rethresh;
    struct RTree *MyRTree;
    static struct RTree_Rect rect;
    static int rect_init = 0;

    if (!rect_init) {
	rect.boundary = G_malloc(6 * sizeof(RectReal));
	rect_init = 6;
    }

    rethresh = 0.000001;	/* TODO */
    APnts = APoints;
    BPnts = BPoints;

    /* TODO: 3D, RE (representation error) threshold, GV_POINTS (line x point) */

    if (!IPnts)
	IPnts = Vect_new_line_struct();

    /* If one or both are point (Points->n_points == 1) */
    if (APoints->n_points == 1 && BPoints->n_points == 1) {
	if (APoints->x[0] == BPoints->x[0] && APoints->y[0] == BPoints->y[0]) {
	    if (!with_z) {
		if (0 >
		    Vect_copy_xyz_to_pnts(IPnts, &APoints->x[0],
					  &APoints->y[0], NULL, 1))
		    G_warning(_("Error while adding point to array. Out of memory"));
		return 1;
	    }
	    else {
		if (APoints->z[0] == BPoints->z[0]) {
		    if (0 >
			Vect_copy_xyz_to_pnts(IPnts, &APoints->x[0],
					      &APoints->y[0], &APoints->z[0],
					      1))
			G_warning(_("Error while adding point to array. Out of memory"));
		    return 1;
		}
		else
		    return 0;
	    }
	}
	else {
	    return 0;
	}
    }

    if (APoints->n_points == 1) {
	Vect_line_distance(BPoints, APoints->x[0], APoints->y[0],
			   APoints->z[0], with_z, NULL, NULL, NULL, &dist,
			   NULL, NULL);

	if (dist <= rethresh) {
	    if (0 >
		Vect_copy_xyz_to_pnts(IPnts, &APoints->x[0], &APoints->y[0],
				      &APoints->z[0], 1))
		G_warning(_("Error while adding point to array. Out of memory"));
	    return 1;
	}
	else {
	    return 0;
	}
    }

    if (BPoints->n_points == 1) {
	Vect_line_distance(APoints, BPoints->x[0], BPoints->y[0],
			   BPoints->z[0], with_z, NULL, NULL, NULL, &dist,
			   NULL, NULL);

	if (dist <= rethresh) {
	    if (0 >
		Vect_copy_xyz_to_pnts(IPnts, &BPoints->x[0], &BPoints->y[0],
				      &BPoints->z[0], 1))
		G_warning(_("Error while adding point to array. Out of memory"));
	    return 1;
	}
	else
	    return 0;
    }

    /* Take each segment from A and find if intersects any segment from B. */

    /* Spatial index: lines may be very long (thousands of segments) and check each segment 
     *  with each from second line takes a long time (n*m). Because of that, spatial index
     *  is build first for the second line and segments from the first line are broken by segments
     *  in bound box */

    /* Create rtree for B line */
    MyRTree = RTreeCreateTree(-1, 0, 2);
    RTreeSetOverflow(MyRTree, 0);
    for (i = 0; i < BPoints->n_points - 1; i++) {
	if (BPoints->x[i] <= BPoints->x[i + 1]) {
	    rect.boundary[0] = BPoints->x[i];
	    rect.boundary[3] = BPoints->x[i + 1];
	}
	else {
	    rect.boundary[0] = BPoints->x[i + 1];
	    rect.boundary[3] = BPoints->x[i];
	}

	if (BPoints->y[i] <= BPoints->y[i + 1]) {
	    rect.boundary[1] = BPoints->y[i];
	    rect.boundary[4] = BPoints->y[i + 1];
	}
	else {
	    rect.boundary[1] = BPoints->y[i + 1];
	    rect.boundary[4] = BPoints->y[i];
	}

	if (BPoints->z[i] <= BPoints->z[i + 1]) {
	    rect.boundary[2] = BPoints->z[i];
	    rect.boundary[5] = BPoints->z[i + 1];
	}
	else {
	    rect.boundary[2] = BPoints->z[i + 1];
	    rect.boundary[5] = BPoints->z[i];
	}

	RTreeInsertRect(&rect, i + 1, MyRTree);	/* B line segment numbers in rtree start from 1 */
    }

    /* Find intersection */
    cross_found = 0;

    for (i = 0; i < APoints->n_points - 1; i++) {
	if (APoints->x[i] <= APoints->x[i + 1]) {
	    rect.boundary[0] = APoints->x[i];
	    rect.boundary[3] = APoints->x[i + 1];
	}
	else {
	    rect.boundary[0] = APoints->x[i + 1];
	    rect.boundary[3] = APoints->x[i];
	}

	if (APoints->y[i] <= APoints->y[i + 1]) {
	    rect.boundary[1] = APoints->y[i];
	    rect.boundary[4] = APoints->y[i + 1];
	}
	else {
	    rect.boundary[1] = APoints->y[i + 1];
	    rect.boundary[4] = APoints->y[i];
	}
	if (APoints->z[i] <= APoints->z[i + 1]) {
	    rect.boundary[2] = APoints->z[i];
	    rect.boundary[5] = APoints->z[i + 1];
	}
	else {
	    rect.boundary[2] = APoints->z[i + 1];
	    rect.boundary[5] = APoints->z[i];
	}

	RTreeSearch(MyRTree, &rect, (void *)find_cross, &i);	/* A segment number from 0 */

	if (cross_found) {
	    break;
	}
    }

    /* Free RTree */
    RTreeDestroyTree(MyRTree);

    return cross_found;
}

/*!
 * \brief Get 2 lines intersection points.
 * 
 * A wrapper around Vect_line_check_intersection() function.
 *
 * \param APoints first input line 
 * \param BPoints second input line 
 * \param[out] IPoints output with intersection points
 * \param with_z 3D, not supported (only if one or both are points)!
 *
 * \return 0 no intersection 
 * \return 1 intersection found
 */
int
Vect_line_get_intersections(struct line_pnts *APoints,
			    struct line_pnts *BPoints,
			    struct line_pnts *IPoints, int with_z)
{
    int ret;

    IPnts = IPoints;
    ret = Vect_line_check_intersection(APoints, BPoints, with_z);

    return ret;
}
