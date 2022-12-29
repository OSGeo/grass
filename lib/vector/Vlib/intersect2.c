/*!
   \file lib/vector/Vlib/intersect2.c

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

   (C) 2001-2014 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2).  Read the file COPYING that comes with GRASS for details.

   \author Original author CERL, probably Dave Gerdes or Mike Higgins.
   \author Update to GRASS 5.7 Radim Blazek.
   \author Update to GRASS 7 Markus Metz.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <grass/vector.h>
#include <grass/rbtree.h>
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
static int snap_cross(int asegment, double *adistance, int bsegment,
		      double *bdistance, double *xc, double *yc);
static int cross_seg(int i, int j, int b);
static int find_cross(int i, int j, int b);


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

static double rethresh = 0.000001;	/* TODO */

static double d_ulp(double a, double b)
{
    double fa = fabs(a);
    double fb = fabs(b);
    double dmax, result;
    int exp;
    
    dmax = fa;
    if (dmax < fb)
	dmax = fb;

    /* unit in the last place (ULP):
     * smallest representable difference
     * shift of the exponent
     * float: 23, double: 52, middle: 37.5 */
    result = frexp(dmax, &exp);
    exp -= 38;
    result = ldexp(result, exp);

    return result;
}

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
static struct line_pnts *APnts, *BPnts, *ABPnts[2], *IPnts;

/* Snap breaks to nearest vertices within RE threshold */
/* Calculate distances along segments */
static int snap_cross(int asegment, double *adistance, int bsegment,
		      double *bdistance, double *xc, double *yc)
{
    int seg;
    double x, y;
    double dist, curdist, dthresh;

    /* 1. of A seg */
    seg = asegment;
    curdist = dist2(*xc, *yc, APnts->x[seg], APnts->y[seg]);
    x = APnts->x[seg];
    y = APnts->y[seg];

    *adistance = curdist;

    /* 2. of A seg */
    dist = dist2(*xc, *yc, APnts->x[seg + 1], APnts->y[seg + 1]);
    if (dist < curdist) {
	curdist = dist;
	x = APnts->x[seg + 1];
	y = APnts->y[seg + 1];
    }

    /* 1. of B seg */
    seg = bsegment;
    dist = dist2(*xc, *yc, BPnts->x[seg], BPnts->y[seg]);
    *bdistance = dist;

    if (dist < curdist) {
	curdist = dist;
	x = BPnts->x[seg];
	y = BPnts->y[seg];
    }
    /* 2. of B seg */
    dist = dist2(*xc, *yc, BPnts->x[seg + 1], BPnts->y[seg + 1]);
    if (dist < curdist) {
	curdist = dist;
	x = BPnts->x[seg + 1];
	y = BPnts->y[seg + 1];
    }

    /* the threshold should not be too small, otherwise we get 
     * too many tiny new segments
     * the threshold should not be too large, otherwise we might 
     * introduce new crossings 
     * the smallest difference representable with 
     * single precision floating point works well with pathological input
     * regular input is not affected */
    dthresh = d_ulp(x, y);
    if (curdist < dthresh * dthresh) {	/* was rethresh * rethresh */
	*xc = x;
	*yc = y;

	/* Update distances along segments */
	seg = asegment;
	*adistance = dist2(*xc, *yc, APnts->x[seg], APnts->y[seg]);
	seg = bsegment;
	*bdistance = dist2(*xc, *yc, BPnts->x[seg], BPnts->y[seg]);

	return 1;
    }

    return 0;
}

/* break segments */
static int cross_seg(int i, int j, int b)
{
    double x1, y1, z1, x2, y2, z2;
    double y1min, y1max, y2min, y2max;
    double adist, bdist;
    int ret;

    y1min = APnts->y[i];
    y1max = APnts->y[i + 1];
    if (APnts->y[i] > APnts->y[i + 1]) {
	y1min = APnts->y[i + 1];
	y1max = APnts->y[i];
    }

    y2min = BPnts->y[j];
    y2max = BPnts->y[j + 1];
    if (BPnts->y[j] > BPnts->y[j + 1]) {
	y2min = BPnts->y[j + 1];
	y2max = BPnts->y[j];
    }

    if (y1min > y2max || y1max < y2min)
	return 0;

    if (b) {
	ret = Vect_segment_intersection(APnts->x[i], APnts->y[i],
	                                APnts->z[i],
					APnts->x[i + 1], APnts->y[i + 1],
					APnts->z[i + 1],
					BPnts->x[j], BPnts->y[j], 
					BPnts->z[j],
					BPnts->x[j + 1], BPnts->y[j + 1],
					BPnts->z[j + 1],
					&x1, &y1, &z1, &x2, &y2, &z2, 0);
    }
    else {
	ret = Vect_segment_intersection(BPnts->x[j], BPnts->y[j],
					BPnts->z[j],
					BPnts->x[j + 1], BPnts->y[j + 1],
					BPnts->z[j + 1], 
					APnts->x[i], APnts->y[i],
					APnts->z[i],
					APnts->x[i + 1], APnts->y[i + 1],
					APnts->z[i + 1],
					&x1, &y1, &z1, &x2, &y2, &z2, 0);
    }

    /* add ALL (including end points and duplicates), clean later */
    if (ret > 0) {
	G_debug(2, "  -> %d x %d: intersection type = %d", i, j, ret);
	if (ret == 1) {		/* one intersection on segment A */
	    G_debug(3, "    in %f, %f ", x1, y1);
	    /* snap intersection only once */
	    snap_cross(i, &adist, j, &bdist, &x1, &y1);
	    add_cross(i, adist, j, bdist, x1, y1);
	    if (APnts == BPnts)
		add_cross(j, bdist, i, adist, x1, y1);
	}
	else if (ret == 2 || ret == 3 || ret == 4 || ret == 5) {
	    /*  partial overlap; a broken in one, b broken in one
	     *  or a contains b; a is broken in 2 points (but 1 may be end)
	     *  or b contains a; b is broken in 2 points (but 1 may be end) 
	     *  or identical */
	    G_debug(3, "    in %f, %f; %f, %f", x1, y1, x2, y2);
	    snap_cross(i, &adist, j, &bdist, &x1, &y1);
	    add_cross(i, adist, j, bdist, x1, y1);
	    if (APnts == BPnts)
		add_cross(j, bdist, i, adist, x1, y1);
	    snap_cross(i, &adist, j, &bdist, &x2, &y2);
	    add_cross(i, adist, j, bdist, x2, y2);
	    if (APnts == BPnts)
		add_cross(j, bdist, i, adist, x2, y2);
	}
    }
    return 1;			/* keep going */
}

/* event queue for Bentley-Ottmann */
#define QEVT_IN 1
#define QEVT_OUT 2
#define QEVT_CRS 3

#define GET_PARENT(p, c) ((p) = (int) (((c) - 2) / 3 + 1))
#define GET_CHILD(c, p) ((c) = (int) (((p) * 3) - 1))

struct qitem
{
    int l;	/* line 0 - A line , 1 - B line */
    int s;	/* segment index */
    int p;	/* point index */
    int e;	/* event type */
};

struct boq
{
    int count;
    int alloc;
    struct qitem *i;
};

/* compare two queue points */
/* return 1 if a < b else 0 */
static int cmp_q_x(struct qitem *a, struct qitem *b)
{
    double x1, y1, z1, x2, y2, z2;

    x1 = ABPnts[a->l]->x[a->p];
    y1 = ABPnts[a->l]->y[a->p];
    z1 = ABPnts[a->l]->z[a->p];

    x2 = ABPnts[b->l]->x[b->p];
    y2 = ABPnts[b->l]->y[b->p];
    z2 = ABPnts[b->l]->z[b->p];

    if (x1 < x2)
	return 1;
    if (x1 > x2)
	return 0;

    if (y1 < y2)
	return 1;
    if (y1 > y2)
	return 0;

    if (z1 < z2)
	return 1;
    if (z1 > z2)
	return 0;

    if (a->e < b->e)
	return 1;

    if (a->l < b->l)
	return 1;

    if (a->s < b->s)
	return 1;

    return 0;
}

/* sift up routine for min heap */
static int sift_up(struct boq *q, int start)
{
    register int parent, child;
    struct qitem a, *b;

    child = start;
    a = q->i[child];

    while (child > 1) {
	GET_PARENT(parent, child);

	b = &q->i[parent];
	/* child smaller */
	if (cmp_q_x(&a, b)) {
	    /* push parent point down */
	    q->i[child] = q->i[parent];
	    child = parent;
	}
	else
	    /* no more sifting up, found new slot for child */
	    break;
    }

    /* put point in new slot */
    if (child < start) {
	q->i[child] = a;
    }

    return child;
}

static int boq_add(struct boq *q, struct qitem *i)
{
    if (q->count + 2 >= q->alloc) {
	q->alloc = q->count + 100;
	q->i = G_realloc(q->i, q->alloc * sizeof(struct qitem));
    }
    q->i[q->count + 1] = *i;
    sift_up(q, q->count + 1);
    
    q->count++;

    return 1;
}

/* drop point routine for min heap */
static int boq_drop(struct boq *q, struct qitem *qi)
{
    register int child, childr, parent;
    register int i;
    struct qitem *a, *b;

    if (q->count == 0)
	return 0;

    *qi = q->i[1];

    if (q->count == 1) {
	q->count = 0;
	return 1;
    }

    /* start with root */
    parent = 1;

    /* sift down: move hole back towards bottom of heap */

    while (GET_CHILD(child, parent) <= q->count) {
	a = &q->i[child];
	i = child + 3;
	for (childr = child + 1; childr <= q->count && childr < i; childr++) {
	    b = &q->i[childr];
	    if (cmp_q_x(b, a)) {
		child = childr;
		a = &q->i[child];
	    }
	}

	/* move hole down */
	q->i[parent] = q->i[child];
	parent = child;
    }

    /* hole is in lowest layer, move to heap end */
    if (parent < q->count) {
	q->i[parent] = q->i[q->count];

	/* sift up last swapped point, only necessary if hole moved to heap end */
	sift_up(q, parent);
    }

    /* the actual drop */
    q->count--;

    return 1;
}

/* compare two tree points */
/* return -1 if a < b, 1 if a > b, 0 if a == b */
static int cmp_t_y(const void *aa, const void *bb)
{
    double x1, y1, z1, x2, y2, z2;
    struct qitem *a = (struct qitem *) aa;
    struct qitem *b = (struct qitem *) bb;

    x1 = ABPnts[a->l]->x[a->p];
    y1 = ABPnts[a->l]->y[a->p];
    z1 = ABPnts[a->l]->z[a->p];

    x2 = ABPnts[b->l]->x[b->p];
    y2 = ABPnts[b->l]->y[b->p];
    z2 = ABPnts[b->l]->z[b->p];

    if (y1 < y2)
	return -1;
    if (y1 > y2)
	return 1;

    if (x1 < x2)
	return -1;
    if (x1 > x2)
	return 1;

    if (z1 < z2)
	return -1;
    if (z1 > z2)
	return 1;

    if (a->s < b->s)
	return -1;
    if (a->s > b->s)
	return 1;

    return 0;
}

static int boq_load(struct boq *q, struct line_pnts *Pnts,
             struct bound_box *abbox, int l, int with_z)
{
    int i, loaded;
    int vi, vo;
    double x1, y1, z1, x2, y2, z2;
    struct bound_box box;
    struct qitem qi;

    /* load Pnts to queue */
    qi.l = l;
    loaded = 0;

    for (i = 0; i < Pnts->n_points - 1; i++) {
	x1 = Pnts->x[i];
	y1 = Pnts->y[i];
	z1 = Pnts->z[i];

	x2 = Pnts->x[i + 1];
	y2 = Pnts->y[i + 1];
	z2 = Pnts->z[i + 1];

	if (x1 == x2 && y1 == y2 && (!with_z || z1 == z2))
	    continue;

	if (x1 < x2) {
	    box.W = x1;
	    box.E = x2;
	}
	else {
	    box.E = x1;
	    box.W = x2;
	}
	if (y1 < y2) {
	    box.S = y1;
	    box.N = y2;
	}
	else {
	    box.N = y1;
	    box.S = y2;
	}
	if (z1 < z2) {
	    box.B = z1;
	    box.T = z2;
	}
	else {
	    box.T = z1;
	    box.B = z2;
	}
	box.W -= d_ulp(box.W, box.W);
	box.S -= d_ulp(box.S, box.S);
	box.B -= d_ulp(box.B, box.B);
	box.E += d_ulp(box.E, box.E);
	box.N += d_ulp(box.N, box.N);
	box.T += d_ulp(box.T, box.T);

	if (!Vect_box_overlap(abbox, &box))
	    continue;

	vi = i;
	vo = i + 1;
	
	if (x1 < x2) {
	    vi = i;
	    vo = i + 1;
	}
	else if (x1 > x2) {
	    vi = i + 1;
	    vo = i;
	}
	else {
	    if (y1 < y2) {
		vi = i;
		vo = i + 1;
	    }
	    else if (y1 > y2) {
		vi = i + 1;
		vo = i;
	    }
	    else {
		if (z1 < z2) {
		    vi = i;
		    vo = i + 1;
		}
		else if (z1 > z2) {
		    vi = i + 1;
		    vo = i;
		}
		else {
		    G_fatal_error("Identical points");
		}
	    }
	}

	qi.s = i;

	/* event in */
	qi.e = QEVT_IN;
	qi.p = vi;
	boq_add(q, &qi);
	
	/* event out */
	qi.e = QEVT_OUT;
	qi.p = vo;
	boq_add(q, &qi);

	loaded += 2;
    }

    return loaded;
}

/*!
 * \brief Intersect 2 lines.
 *
 * Creates array of new lines created from original A line, by
 * intersection with B line. Points (Points->n_points == 1) are not
 * supported. If B line is NULL, A line is intersected with itself.
 * 
 * simplified Bentley–Ottmann Algorithm
 *
 * \param APoints first input line 
 * \param BPoints second input line or NULL
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
Vect_line_intersection2(struct line_pnts *APoints,
		        struct line_pnts *BPoints,
		        struct bound_box *pABox,
		        struct bound_box *pBBox,
		        struct line_pnts ***ALines,
		        struct line_pnts ***BLines,
		        int *nalines, int *nblines, int with_z)
{
    int i, j, k, l, nl, last_seg, seg, last;
    int n_alive_cross;
    double dist, last_x, last_y, last_z;
    struct line_pnts **XLines, *Points;
    struct line_pnts *Points1, *Points2;	/* first, second points */
    int seg1, seg2, vert1, vert2;
    struct bound_box ABox, BBox, abbox;
    struct boq bo_queue;
    struct qitem qi, *found;
    struct RB_TREE *bo_ta, *bo_tb;
    struct RB_TRAV bo_t_trav;
    int same = 0;

    if (debug_level == -1) {
	const char *dstr = G_getenv_nofatal("DEBUG");

	if (dstr != NULL)
	    debug_level = atoi(dstr);
	else
	    debug_level = 0;
    }

    n_cross = 0;
    APnts = APoints;
    BPnts = BPoints;

    same = 0;
    if (!BPoints) {
	BPnts = APoints;
	same = 1;
    }

    ABPnts[0] = APnts;
    ABPnts[1] = BPnts;

    *nalines = 0;
    *nblines = 0;

    /* RE (representation error).
     *  RE thresh above is nonsense of course, the RE threshold should be based on
     *  number of significant digits for double (IEEE-754) which is 15 or 16 and exponent. 
     *  The number above is in fact not required threshold, and will not work
     *  for example: equator length is 40.075,695 km (8 digits), units are m (+3) 
     *  and we want precision in mm (+ 3) = 14 -> minimum rethresh may be around 0.001
     *  ?Maybe all nonsense? 
     *  Use rounding error of the unit in the last place ? 
     *  max of fabs(x), fabs(y)
     *  rethresh = pow(2, log2(max) - 53) */

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

    /* don't modify original bboxes: make a copy of the bboxes */
    ABox = *pABox;
    BBox = *pBBox;
    if (!with_z) {
	ABox.T = BBox.T = PORT_DOUBLE_MAX;
	ABox.B = BBox.B = -PORT_DOUBLE_MAX;
    }

    if (!same && !Vect_box_overlap(&ABox, &BBox)) {
	return 0;
    }

    /* overlap box of A line and B line */
    abbox = BBox;
    if (!same) {
	if (abbox.N > ABox.N)
	    abbox.N = ABox.N;
	if (abbox.S < ABox.S)
	    abbox.S = ABox.S;
	if (abbox.E > ABox.E)
	    abbox.E = ABox.E;
	if (abbox.W < ABox.W)
	    abbox.W = ABox.W;

	if (with_z) {
	    if (abbox.T > BBox.T)
		abbox.T = BBox.T;
	    if (abbox.B < BBox.B)
		abbox.B = BBox.B;
	}
    }

    abbox.N += d_ulp(abbox.N, abbox.N);
    abbox.S -= d_ulp(abbox.S, abbox.S);
    abbox.E += d_ulp(abbox.E, abbox.E);
    abbox.W -= d_ulp(abbox.W, abbox.W);
    if (with_z) {
	abbox.T += d_ulp(abbox.T, abbox.T);
	abbox.B -= d_ulp(abbox.B, abbox.B);
    }

    if (APnts->n_points < 2 || BPnts->n_points < 2) {
	G_fatal_error("Intersection with points is not yet supported");
	return 0;
    }

    /* initialize queue */
    bo_queue.count = 0;
    bo_queue.alloc = 2 * (APnts->n_points + BPnts->n_points);
    bo_queue.i = G_malloc(bo_queue.alloc * sizeof(struct qitem));

    /* load APnts to queue */
    boq_load(&bo_queue, APnts, &abbox, 0, with_z);

    if (!same) {
	/* load BPnts to queue */
	boq_load(&bo_queue, BPnts, &abbox, 1, with_z);
    }

    /* initialize search tree */
    bo_ta = rbtree_create(cmp_t_y, sizeof(struct qitem));
    if (same)
	bo_tb = bo_ta;
    else
	bo_tb = rbtree_create(cmp_t_y, sizeof(struct qitem));

    /* find intersections */
    while (boq_drop(&bo_queue, &qi)) {

	if (qi.e == QEVT_IN) {
	    /* not the original Bentley-Ottmann algorithm */
	    if (qi.l == 0) {
		/* test for intersection of s with all segments in T */
		rbtree_init_trav(&bo_t_trav, bo_tb);
		while ((found = rbtree_traverse(&bo_t_trav))) {
		    cross_seg(qi.s, found->s, 0);
		}
		
		/* insert s in T */
		rbtree_insert(bo_ta, &qi);
	    }
	    else {
		/* test for intersection of s with all segments in T */
		rbtree_init_trav(&bo_t_trav, bo_ta);
		while ((found = rbtree_traverse(&bo_t_trav))) {
		    cross_seg(found->s, qi.s, 1);
		}
		
		/* insert s in T */
		rbtree_insert(bo_tb, &qi);
	    }
	}
	else if (qi.e == QEVT_OUT) {
	    /* remove from T */
	    
	    /* adjust p */
	    if (qi.p == qi.s)
		qi.p++;
	    else
		qi.p--;
	    
	    if (qi.l == 0) {
		if (!rbtree_remove(bo_ta, &qi))
		    G_fatal_error("RB tree error!");
	    }
	    else {
		if (!rbtree_remove(bo_tb, &qi))
		    G_fatal_error("RB tree error!");
	    }
	}
    }
    G_free(bo_queue.i);
    rbtree_destroy(bo_ta);
    if (!same)
	rbtree_destroy(bo_tb);

    G_debug(2, "n_cross = %d", n_cross);
    /* Lines do not cross each other */
    if (n_cross == 0) {
	return 0;
    }

    /* l = 1 ~ line A, l = 2 ~ line B */
    nl = 3;
    if (same)
	nl = 2;
    for (l = 1; l < nl; l++) {
	for (i = 0; i < n_cross; i++)
	    use_cross[i] = 1;

	/* Create array of lines */
	XLines = G_malloc((n_cross + 1) * sizeof(struct line_pnts *));

	if (l == 1) {
	    G_debug(2, "Clean and create array for line A");
	    Points = APnts;
	    Points1 = APnts;
	    Points2 = BPnts;
	    current = 0;
	    second = 1;
	}
	else {
	    G_debug(2, "Clean and create array for line B");
	    Points = BPnts;
	    Points1 = BPnts;
	    Points2 = APnts;
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
	for (i = 0; i < n_cross; i++) {
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

		last_seg = seg;
		last_x = cross[i].x;
		last_y = cross[i].y;
		last_z = 0;
		/* calculate last_z */
		if (Points->z[last_seg] == Points->z[last_seg + 1]) {
		    last_z = Points->z[last_seg + 1];
		}
		else if (last_x == Points->x[last_seg] &&
		    last_y == Points->y[last_seg]) {
		    last_z = Points->z[last_seg];
		}
		else if (last_x == Points->x[last_seg + 1] &&
		    last_y == Points->y[last_seg + 1]) {
		    last_z = Points->z[last_seg + 1];
		}
		else {
		    dist = dist2(Points->x[last_seg],
		                 Points->x[last_seg + 1],
				 Points->y[last_seg],
				 Points->y[last_seg + 1]);
		    if (with_z) {
			last_z = (Points->z[last_seg] * sqrt(cross[i].distance[current]) +
				  Points->z[last_seg + 1] * 
				  (sqrt(dist) - sqrt(cross[i].distance[current]))) /
				  sqrt(dist);
		    }
		}

		/* add current cross or end point */
		Vect_append_point(XLines[k], cross[i].x, cross[i].y, last_z);
		G_debug(2, "   append cross / last point: %f %f", cross[i].x,
			cross[i].y);

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

    return 1;
}

static int cross_found;		/* set by find_cross() */

/* find segment intersection, used by Vect_line_check_intersection2 */
static int find_cross(int i, int j, int b)
{
    double x1, y1, z1, x2, y2, z2;
    double y1min, y1max, y2min, y2max;
    int ret;

    y1min = APnts->y[i];
    y1max = APnts->y[i + 1];
    if (APnts->y[i] > APnts->y[i + 1]) {
	y1min = APnts->y[i + 1];
	y1max = APnts->y[i];
    }

    y2min = BPnts->y[j];
    y2max = BPnts->y[j + 1];
    if (BPnts->y[j] > BPnts->y[j + 1]) {
	y2min = BPnts->y[j + 1];
	y2max = BPnts->y[j];
    }

    if (y1min > y2max || y1max < y2min)
	return 0;

    if (b) {
	ret = Vect_segment_intersection(APnts->x[i], APnts->y[i],
	                                APnts->z[i],
					APnts->x[i + 1], APnts->y[i + 1],
					APnts->z[i + 1],
					BPnts->x[j], BPnts->y[j], 
					BPnts->z[j],
					BPnts->x[j + 1], BPnts->y[j + 1],
					BPnts->z[j + 1],
					&x1, &y1, &z1, &x2, &y2, &z2, 0);
    }
    else {
	ret = Vect_segment_intersection(BPnts->x[j], BPnts->y[j],
					BPnts->z[j],
					BPnts->x[j + 1], BPnts->y[j + 1],
					BPnts->z[j + 1], 
					APnts->x[i], APnts->y[i],
					APnts->z[i],
					APnts->x[i + 1], APnts->y[i + 1],
					APnts->z[i + 1],
					&x1, &y1, &z1, &x2, &y2, &z2, 0);
    }

    if (!IPnts)
	IPnts = Vect_new_line_struct();

    /* add ALL (including end points and duplicates), clean later */
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

    return ret;
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
 * \return 1 intersection
 * \return 2 end points only
 */
int
Vect_line_check_intersection2(struct line_pnts *APoints,
			      struct line_pnts *BPoints, int with_z)
{
    double dist;
    struct bound_box ABox, BBox, abbox;
    struct boq bo_queue;
    struct qitem qi, *found;
    struct RB_TREE *bo_ta, *bo_tb;
    struct RB_TRAV bo_t_trav;
    int ret, intersect;
    double xa1, ya1, xa2, ya2, xb1, yb1, xb2, yb2, xi, yi;

    APnts = APoints;
    BPnts = BPoints;

    ABPnts[0] = APnts;
    ABPnts[1] = BPnts;

    /* TODO: 3D, RE (representation error) threshold, GV_POINTS (line x point) */

    if (!IPnts)
	IPnts = Vect_new_line_struct();
    Vect_reset_line(IPnts);

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

	if (dist <= d_ulp(APoints->x[0], APoints->y[0])) {
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

	if (dist <= d_ulp(BPoints->x[0], BPoints->y[0])) {
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

    dig_line_box(APoints, &ABox);
    dig_line_box(BPoints, &BBox);
    if (!with_z) {
	ABox.T = BBox.T = PORT_DOUBLE_MAX;
	ABox.B = BBox.B = -PORT_DOUBLE_MAX;
    }

    if (!Vect_box_overlap(&ABox, &BBox)) {
	return 0;
    }

    /* overlap box */
    abbox = BBox;
    if (abbox.N > ABox.N)
	abbox.N = ABox.N;
    if (abbox.S < ABox.S)
	abbox.S = ABox.S;
    if (abbox.E > ABox.E)
	abbox.E = ABox.E;
    if (abbox.W < ABox.W)
	abbox.W = ABox.W;

    abbox.N += d_ulp(abbox.N, abbox.N);
    abbox.S -= d_ulp(abbox.S, abbox.S);
    abbox.E += d_ulp(abbox.E, abbox.E);
    abbox.W -= d_ulp(abbox.W, abbox.W);

    if (with_z) {
	if (abbox.T > ABox.T)
	    abbox.T = ABox.T;
	if (abbox.B < ABox.B)
	    abbox.B = ABox.B;

	abbox.T += d_ulp(abbox.T, abbox.T);
	abbox.B -= d_ulp(abbox.B, abbox.B);
    }

    /* initialize queue */
    bo_queue.count = 0;
    bo_queue.alloc = 2 * (APnts->n_points + BPnts->n_points);
    bo_queue.i = G_malloc(bo_queue.alloc * sizeof(struct qitem));

    /* load APnts to queue */
    if (!boq_load(&bo_queue, APnts, &abbox, 0, with_z)) {
	G_free(bo_queue.i);
	return 0;
    }

    /* load BPnts to queue */
    if (!boq_load(&bo_queue, BPnts, &abbox, 1, with_z)) {
	G_free(bo_queue.i);
	return 0;
    }

    /* initialize search tree */
    bo_ta = rbtree_create(cmp_t_y, sizeof(struct qitem));
    bo_tb = rbtree_create(cmp_t_y, sizeof(struct qitem));

    /* find intersection */
    xa1 = APnts->x[0];
    ya1 = APnts->y[0];
    xa2 = APnts->x[APnts->n_points - 1];
    ya2 = APnts->y[APnts->n_points - 1];
    xb1 = BPnts->x[0];
    yb1 = BPnts->y[0];
    xb2 = BPnts->x[BPnts->n_points - 1];
    yb2 = BPnts->y[BPnts->n_points - 1];
    intersect = 0;
    while (boq_drop(&bo_queue, &qi)) {

	if (qi.e == QEVT_IN) {
	    /* not the original Bentley-Ottmann algorithm */
	    if (qi.l == 0) {
		/* test for intersection of s with all segments in T */
		rbtree_init_trav(&bo_t_trav, bo_tb);
		while ((found = rbtree_traverse(&bo_t_trav))) {
		    ret = find_cross(qi.s, found->s, 0);

		    if (ret > 0) {
			if (ret != 1) {
			    intersect = 1;
			}
			/* intersect at one point */
			else if (intersect != 1) {
			    intersect = 1;
			    xi = IPnts->x[IPnts->n_points - 1];
			    yi = IPnts->y[IPnts->n_points - 1];
			    if (xi == xa1 && yi == ya1) {
				if ((xi == xb1 && yi == yb1) ||
				    (xi == xb2 && yi == yb2)) {
				    intersect = 2;
				}
			    }
			    else if (xi == xa2 && yi == ya2) {
				if ((xi == xb1 && yi == yb1) ||
				    (xi == xb2 && yi == yb2)) {
				    intersect = 2;
				}
			    }
			}
		    }

		    if (intersect == 1) {
			break;
		    }
		}
		
		/* insert s in T */
		rbtree_insert(bo_ta, &qi);
	    }
	    else {
		/* test for intersection of s with all segments in T */
		rbtree_init_trav(&bo_t_trav, bo_ta);
		while ((found = rbtree_traverse(&bo_t_trav))) {
		    ret = find_cross(found->s, qi.s, 1);

		    if (ret > 0) {
			if (ret != 1) {
			    intersect = 1;
			}
			/* intersect at one point */
			else if (intersect != 1) {
			    intersect = 1;
			    xi = IPnts->x[IPnts->n_points - 1];
			    yi = IPnts->y[IPnts->n_points - 1];
			    if (xi == xa1 && yi == ya1) {
				if ((xi == xb1 && yi == yb1) ||
				    (xi == xb2 && yi == yb2)) {
				    intersect = 2;
				}
			    }
			    else if (xi == xa2 && yi == ya2) {
				if ((xi == xb1 && yi == yb1) ||
				    (xi == xb2 && yi == yb2)) {
				    intersect = 2;
				}
			    }
			}
		    }

		    if (intersect == 1) {
			break;
		    }
		}
		
		/* insert s in T */
		rbtree_insert(bo_tb, &qi);
	    }
	}
	else if (qi.e == QEVT_OUT) {
	    /* remove from T */
	    
	    /* adjust p */
	    if (qi.p == qi.s)
		qi.p++;
	    else
		qi.p--;
	    
	    if (qi.l == 0) {
		if (!rbtree_remove(bo_ta, &qi))
		    G_fatal_error("RB tree error!");
	    }
	    else {
		if (!rbtree_remove(bo_tb, &qi))
		    G_fatal_error("RB tree error!");
	    }
	}
	if (intersect == 1) {
	    break;
	}
    }
    G_free(bo_queue.i);
    rbtree_destroy(bo_ta);
    rbtree_destroy(bo_tb);

    return intersect;
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
Vect_line_get_intersections2(struct line_pnts *APoints,
			     struct line_pnts *BPoints,
			     struct line_pnts *IPoints, int with_z)
{
    int ret;

    IPnts = IPoints;
    ret = Vect_line_check_intersection2(APoints, BPoints, with_z);

    return ret;
}
