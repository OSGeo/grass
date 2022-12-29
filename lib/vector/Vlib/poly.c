/*!
   \file lib/vector/Vlib/poly.c

   \brief Vector library - polygon related fns

   Higher level functions for reading/writing/manipulating vectors.

   (C) 2001-2009 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2).  Read the file COPYING that comes with GRASS for details.

   \author Original author CERL, probably Dave Gerdes or Mike Higgins.
   \author Update to GRASS 5.7 Radim Blazek and David D. Gray.
 */

#include <math.h>
#include <stdlib.h>
#include <grass/vector.h>
#include <grass/linkm.h>
#include <grass/glocale.h>

struct Slink
{
    struct Slink *next;
    double x;
};


/* function prototypes */
static int comp_double(double *, double *);
static int V__within(double, double, double);
int Vect__intersect_y_line_with_poly();
int Vect__intersect_x_line_with_poly();
static void destroy_links(struct link_head *, struct Slink *);
static int Vect__divide_and_conquer(struct Slink *, const struct line_pnts *,
				    struct link_head *, double *, double *,
				    int);

/*!
   \brief Get point inside area and outside all islands.

   Take a line and intersect it with the polygon and any islands.
   sort the list of X values from these intersections.  This will
   be a list of segments alternating  IN/OUT/IN/OUT of the polygon.
   Pick the largest IN segment and take the midpoint. 

   \param Map vector map
   \param area area id
   \param[out] X,Y point coordinateds

   \return 0 on success
   \return -1 on error
 */
int
Vect_get_point_in_area(const struct Map_info *Map, int area, double *X, double *Y)
{
    static struct line_pnts *Points;
    static struct line_pnts **IPoints;
    static int first_time = 1;
    static int isl_allocated = 0;
    int i, n_isles;

    G_debug(3, "Vect_get_point_in_area()");

    if (first_time) {
	Points = Vect_new_line_struct();
	IPoints = NULL;
	first_time = 0;
    }
    n_isles = Vect_get_area_num_isles(Map, area);
    if (n_isles > isl_allocated) {
	IPoints = (struct line_pnts **)
	    G_realloc(IPoints, (1 + n_isles) * sizeof(struct line_pnts *));
	for (i = isl_allocated; i < n_isles; i++)
	    IPoints[i] = Vect_new_line_struct();
	isl_allocated = n_isles;
    }

    if (0 > Vect_get_area_points(Map, area, Points))
	return -1;

    for (i = 0; i < n_isles; i++) {
	IPoints[i]->alloc_points = 0;
	if (0 >
	    Vect_get_isle_points(Map, Vect_get_area_isle(Map, area, i),
				 IPoints[i]))
	    return -1;
    }
    return (Vect_get_point_in_poly_isl((const struct line_pnts*) Points,
				       (const struct line_pnts**) IPoints, n_isles, X, Y));

    return -1;
}

static int comp_double(double *i, double *j)
{
    if (*i < *j)
	return -1;

    return (*i > *j);
}

static int V__within(double a, double x, double b)
{
    if (a < b)
	return (x >= a && x < b);

    return (x > b && x <= a);
}

/*
   \brief Intersects line with polygon

   For each intersection of a polygon w/ a line, stuff the X value in
   the Inter Points array.  I used line_pnts, just cuz the memory
   management was already there.  I am getting real tired of managing
   realloc stuff.

   \param Points line
   \param y y coordinate of horizontal line
   \param Inter intersections of horizontal line with points line

   \return 0 on success
   \return -1 on error 
 */
int
Vect__intersect_y_line_with_poly(const struct line_pnts *Points,
			       double y, struct line_pnts *Inter)
{
    int i;
    double a, b, c, d, x;
    double perc;

    for (i = 1; i < Points->n_points; i++) {
	a = Points->y[i - 1];
	b = Points->y[i];

	c = Points->x[i - 1];
	d = Points->x[i];

	if (V__within(a, y, b)) {
	    if (a == b)
		continue;

	    perc = (y - a) / (b - a);
	    x = perc * (d - c) + c;	/* interp X */

	    if (0 > Vect_append_point(Inter, x, y, 0))
		return -1;
	}
    }
    return 0;
}

/*
   \brief Intersects line with polygon

   For each intersection of a polygon w/ a line, stuff the Y value in
   the Inter Points array.  I used line_pnts, just cuz the memory
   management was already there.  I am getting real tired of managing
   realloc stuff.

   \param Points line
   \param x x coordinate of vertical line
   \param Inter intersections of horizontal line with points line

   \return 0 on success
   \return -1 on error 
 */
int
Vect__intersect_x_line_with_poly(const struct line_pnts *Points,
			       double x, struct line_pnts *Inter)
{
    int i;
    double a, b, c, d, y;
    double perc;

    for (i = 1; i < Points->n_points; i++) {
	a = Points->x[i - 1];
	b = Points->x[i];

	c = Points->y[i - 1];
	d = Points->y[i];

	if (V__within(a, x, b)) {
	    if (a == b)
		continue;

	    perc = (x - a) / (b - a);
	    y = perc * (d - c) + c;	/* interp Y */

	    if (0 > Vect_append_point(Inter, x, y, 0))
		return -1;
	}
    }
    return 0;
}

/*!
   \brief Get point inside polygon.

   This does NOT consider ISLANDS!

   \param Points polygon
   \param[out] X,Y point coordinates

   \return 0 on success
   \return -1 on error
 */
int Vect_get_point_in_poly(const struct line_pnts *Points, double *X, double *Y)
{
    double cent_x, cent_y;
    struct Slink *Head;
    static struct link_head *Token;
    struct Slink *tmp;
    static int first_time = 1;
    register int i;
    double x_max, x_min;
    int ret;

    /* get centroid */
    Vect_find_poly_centroid(Points, &cent_x, &cent_y);
    /* is it w/in poly? */
    if (Vect_point_in_poly(cent_x, cent_y, Points) == 1) {
	*X = cent_x;
	*Y = cent_y;
	return 0;
    }

    /* guess we have to do it the hard way... */
    G_debug(3, "Vect_get_point_in_poly(): divide and conquer");

    /* get min and max x values */
    x_max = x_min = Points->x[0];
    for (i = 0; i < Points->n_points; i++) {
	if (x_min > Points->x[i])
	    x_min = Points->x[i];
	if (x_max < Points->x[i])
	    x_max = Points->x[i];
    }


    /* init the linked list */
    if (first_time) {
	/* will never call link_cleanup ()  */
	link_exit_on_error(1);	/* kill program if out of memory */
	Token = (struct link_head *)link_init(sizeof(struct Slink));
	first_time = 0;
    }

    Head = (struct Slink *)link_new(Token);
    tmp = (struct Slink *)link_new(Token);

    Head->next = tmp;
    tmp->next = NULL;

    Head->x = x_min;
    tmp->x = x_max;

    *Y = cent_y;		/* pick line segment (x_min, cent_y) - (x_max, cent_y) */
    ret = Vect__divide_and_conquer(Head, Points, Token, X, Y, 10);

    destroy_links(Token, Head);

    if (ret < 0) {
	G_warning("Vect_get_point_in_poly(): %s",
		  _("Unable to find point in polygon"));
	return -1;
    }

    G_debug(3, "Found point in %d iterations", 10 - ret);

    return 0;
}


/*
   \brief Provide a breadth first binary division of real space along line segment.

   Looking for a point w/in the polygon.

   This routine walks along the list of points on line segment
   and divides each pair in half. It sticks that new point right into
   the list, and then checks to see if it is inside the poly. 

   After going through the whole list, it calls itself.  The list 
   now has a whole extra set of points to divide again.

   \param Head 
   \param Points
   \param Token
   \param X,Y
   \param levels

   \return # levels it took
   \return -1 if exceeded # of levels
 */
static int
Vect__divide_and_conquer(struct Slink *Head,
			 const struct line_pnts *Points,
			 struct link_head *Token,
			 double *X, double *Y, int levels)
{
    struct Slink *A, *B, *C;

    G_debug(3, "Vect__divide_and_conquer(): LEVEL %d", levels);
    A = Head;
    B = Head->next;

    do {
	C = (struct Slink *)link_new(Token);
	A->next = C;
	C->next = B;

	C->x = (A->x + B->x) / 2.;

	if (Vect_point_in_poly(C->x, *Y, Points) == 1) {
	    *X = C->x;
	    return levels;
	}

	A = B;
	B = B->next;
    }
    while (B != NULL);

    /*
     **  If it got through the entire loop and still no hits,
     **   then lets go a level deeper and divide again.
     */

    if (levels <= 0)
	return -1;

    return Vect__divide_and_conquer(Head, Points, Token, X, Y, --levels);
}

static void destroy_links(struct link_head *Token, struct Slink *Head)
{
    struct Slink *p, *tmp;

    p = Head;

    while (p != NULL) {
	tmp = p->next;
	link_dispose(Token, (VOID_T *) p);
	p = tmp;
    }
}


/*!
   \brief Get centroid of polygon

   \param points polygon
   \param[out] cent_x,cent_y centroid coordinates

   \return 0 on success
   \return -1 on error
 */
int
Vect_find_poly_centroid(const struct line_pnts *points,
			double *cent_x, double *cent_y)
{
    int i;
    double *xptr1, *yptr1;
    double *xptr2, *yptr2;
    double cent_weight_x, cent_weight_y;
    double len, tot_len;

    tot_len = 0.0;
    cent_weight_x = 0.0;
    cent_weight_y = 0.0;

    xptr1 = points->x;
    yptr1 = points->y;
    xptr2 = points->x + 1;
    yptr2 = points->y + 1;

    /* center of gravity of the polygon line, not area */
    for (i = 1; i < points->n_points; i++) {
	len = hypot(*xptr1 - *xptr2, *yptr1 - *yptr2);
	cent_weight_x += len * ((*xptr1 + *xptr2) / 2.);
	cent_weight_y += len * ((*yptr1 + *yptr2) / 2.);
	tot_len += len;
	xptr1++;
	xptr2++;
	yptr1++;
	yptr2++;
    }

    if (tot_len == 0.0)
	return -1;

    *cent_x = cent_weight_x / tot_len;
    *cent_y = cent_weight_y / tot_len;

    return 0;
}


/*
 ** returns true if point is in any of islands /w in area
 ** returns 0 if not
 ** returns -1 on error
 */
/*
   int 
   Vect_point_in_islands (
   struct Map_info *Map,
   int area,
   double cent_x, double cent_y)
   {
   struct P_area *Area;
   static struct line_pnts *TPoints;
   static int first_time = 1;
   int isle;

   if (first_time == 1)
   {
   TPoints = Vect_new_line_struct ();
   first_time = 0;
   }

   Area = &(Map->plus.Area[area]);

   for (isle = 0; isle < Area->n_isles; isle++)
   {
   if (0 > Vect_get_isle_points (Map, Area->isles[isle], TPoints))
   return -1;

   if ( Vect_point_in_poly (cent_x, cent_y, TPoints) == 1 )
   return 1;
   }

   return 0;
   }
 */

/*!
   \brief Get point inside polygon but outside the islands specifiled in IPoints.

   Take a line and intersect it with the polygon and any islands.
   sort the list of X values from these intersections. This will be a
   list of segments alternating IN/OUT/IN/OUT of the polygon. Pick the
   largest IN segment and take the midpoint.

   \param Points polygon (boundary)
   \param IPoints isles (list of isle boundaries)
   \param n_isles number of isles
   \param[out] att_x,att_y point coordinates

   \return 0 on success
   \return -1 on error
 */
int Vect_get_point_in_poly_isl(const struct line_pnts *Points,
			       const struct line_pnts **IPoints, int n_isles,
			       double *att_x, double *att_y)
{
    static struct line_pnts *Intersects;
    static int first_time = 1;
    double cent_x, cent_y;
    register int i, j;
    double max, hi_x, lo_x, hi_y, lo_y;
    double fa, fb, dmax;
    int exp;
    int maxpos;
    int point_in_sles = 0;
    double diff;
    int ret;

    G_debug(3, "Vect_get_point_in_poly_isl(): n_isles = %d", n_isles);

    if (first_time) {
	Intersects = Vect_new_line_struct();
	first_time = 0;
    }

    if (Points->n_points < 3) {	/* test */
	if (Points->n_points > 0) {
	    *att_x = Points->x[0];
	    *att_y = Points->y[0];
	    return 0;
	}
	return -1;
    }

    /* get centroid */
    Vect_find_poly_centroid(Points, &cent_x, &cent_y);
    /* is it w/in poly? */
    if (Vect_point_in_poly(cent_x, cent_y, Points) == 1) {
	/* if the point is inside the polygon */
	for (i = 0; i < n_isles; i++) {
	    if (Vect_point_in_poly(cent_x, cent_y, IPoints[i]) >= 1) {
		point_in_sles = 1;
		break;
	    }
	}
	if (!point_in_sles) {
	    *att_x = cent_x;
	    *att_y = cent_y;
	    return 0;
	}
    }
    /* guess we have to do it the hard way... */

    /* first find att_y close to cent_y so that no points lie on the line */
    /* find the point closest to line from below, and point close to line
       from above and take average of their y-coordinates */
    /* same for x */

    /* first initializing lo_x,hi_x and lo_y,hi_y 
     * to be any 2 pnts on either side of cent_x and cent_y */
    hi_y = cent_y - 1;
    lo_y = cent_y + 1;

    hi_x = cent_x - 1;
    lo_x = cent_x + 1;
    for (i = 0; i < Points->n_points; i++) {
	if ((lo_y < cent_y) && (hi_y >= cent_y) &&
	    (lo_x < cent_x) && (hi_x >= cent_x))
	    break;		/* already initialized */
	if (Points->y[i] < cent_y)
	    lo_y = Points->y[i];
	if (Points->y[i] >= cent_y)
	    hi_y = Points->y[i];

	if (Points->x[i] < cent_x)
	    lo_x = Points->x[i];
	if (Points->x[i] >= cent_x)
	    hi_x = Points->x[i];
    }
    /* first going through boundary points */
    for (i = 0; i < Points->n_points; i++) {
	if ((Points->y[i] < cent_y) &&
	    ((cent_y - Points->y[i]) < (cent_y - lo_y)))
	    lo_y = Points->y[i];
	if ((Points->y[i] >= cent_y) &&
	    ((Points->y[i] - cent_y) < (hi_y - cent_y)))
	    hi_y = Points->y[i];

	if ((Points->x[i] < cent_x) &&
	    ((cent_x - Points->x[i]) < (cent_x - lo_x)))
	    lo_x = Points->x[i];
	if ((Points->x[i] >= cent_x) &&
	    ((Points->x[i] - cent_x) < (hi_x - cent_x)))
	    hi_x = Points->x[i];
    }
    for (i = 0; i < n_isles; i++) {
	for (j = 0; j < IPoints[i]->n_points; j++) {
	    if ((IPoints[i]->y[j] < cent_y) &&
		((cent_y - IPoints[i]->y[j]) < (cent_y - lo_y)))
		lo_y = IPoints[i]->y[j];
	    if ((IPoints[i]->y[j] >= cent_y) &&
		((IPoints[i]->y[j] - cent_y) < (hi_y - cent_y)))
		hi_y = IPoints[i]->y[j];

	    if ((IPoints[i]->x[j] < cent_x) &&
		((cent_x - IPoints[i]->x[j]) < (cent_x - lo_x)))
		lo_x = IPoints[i]->x[j];
	    if ((IPoints[i]->x[j] >= cent_x) &&
		((IPoints[i]->x[j] - cent_x) < (hi_x - cent_x)))
		hi_x = IPoints[i]->x[j];
	}
    }

    if (lo_y == hi_y)
	return (-1);		/* area is empty */

    *att_y = (hi_y + lo_y) / 2.0;

    Intersects->n_points = 0;
    Vect__intersect_y_line_with_poly(Points, *att_y, Intersects);

    /* add in intersections w/ holes */
    for (i = 0; i < n_isles; i++) {
	if (0 >
	    Vect__intersect_y_line_with_poly(IPoints[i], *att_y, Intersects))
	    return -1;
    }

    if (Intersects->n_points < 2)	/* test */
	return -1;

    qsort(Intersects->x, (size_t) Intersects->n_points, sizeof(double),
	  (void *)comp_double);

    max = 0;
    maxpos = 0;

    /* find area of MAX distance */
    for (i = 0; i < Intersects->n_points; i += 2) {
	diff = Intersects->x[i + 1] - Intersects->x[i];

	if (diff > max) {
	    max = diff;
	    maxpos = i;
	}
    }
    /* ULP single precision 23, double 52 bits, here 42 */
    /* if the difference is too small, the point will be on a line
     * ULP double is too small, ULP single too large */
    fa = fabs(Intersects->x[maxpos]);
    fb = fabs(Intersects->x[maxpos + 1]);
    if (fa > fb)
	dmax = frexp(fa, &exp);
    else
	dmax = frexp(fb, &exp);
    exp -= 42;
    dmax = ldexp(dmax, exp);

    if (max > dmax) {
	*att_x = (Intersects->x[maxpos] + Intersects->x[maxpos + 1]) / 2.;
    }
    else {
	/* try x intersect */
	G_debug(3, "Vect_get_point_in_poly_isl(): trying x intersect");

	if (lo_x == hi_x)
	    return (-1);		/* area is empty */

	*att_x = (hi_x + lo_x) / 2.0;

	Intersects->n_points = 0;
	Vect__intersect_x_line_with_poly(Points, *att_x, Intersects);

	/* add in intersections w/ holes */
	for (i = 0; i < n_isles; i++) {
	    if (0 >
		Vect__intersect_x_line_with_poly(IPoints[i], *att_x, Intersects))
		return -1;
	}

	if (Intersects->n_points < 2)	/* test */
	    return -1;

	qsort(Intersects->y, (size_t) Intersects->n_points, sizeof(double),
	      (void *)comp_double);

	max = 0;
	maxpos = 0;

	/* find area of MAX distance */
	for (i = 0; i < Intersects->n_points; i += 2) {
	    diff = Intersects->y[i + 1] - Intersects->y[i];

	    if (diff > max) {
		max = diff;
		maxpos = i;
	    }
	}
	/* ULP single precision 23, double 52 bits, here 42 */
	fa = fabs(Intersects->y[maxpos]);
	fb = fabs(Intersects->y[maxpos + 1]);
	if (fa > fb)
	    dmax = frexp(fa, &exp);
	else
	    dmax = frexp(fb, &exp);
	exp -= 42;
	dmax = ldexp(dmax, exp);
	if (max > dmax) {
	    *att_y = (Intersects->y[maxpos] + Intersects->y[maxpos + 1]) / 2.;
	}
	else {
	    /* area was (nearly) empty: example ((x1,y1), (x2,y2), (x1,y1)) */
	    G_warning("Vect_get_point_in_poly_isl(): collapsed area");
	    return -1;
	}
    }

    /* is it now w/in poly? */
    cent_x = *att_x;
    cent_y = *att_y;
    point_in_sles = 0;
    
    ret = Vect_point_in_poly(cent_x, cent_y, Points);
    if (ret == 2) {
	/* point on outer ring, should not happen because of ULP test above */
	G_warning("Vect_get_point_in_poly_isl(), the hard way: centroid is on outer ring, max dist is %g", max);
	return -1;
    }
    if (ret == 1) {
	/* if the point is inside the polygon, should not happen because of ULP test above */
	for (i = 0; i < n_isles; i++) {
	    if (Vect_point_in_poly(cent_x, cent_y, IPoints[i]) >= 1) {
		point_in_sles = 1;
		G_warning("Vect_get_point_in_poly_isl(), the hard way: centroid is in isle, max dist is %g", max);
		break;
	    }
	}
	if (!point_in_sles) {
	    return 0;
	}
    }

   return -1;
}


/* Intersect segments of Points with ray from point X,Y to the right.
 * Returns: -1 point exactly on segment
 *          number of intersections
 */
static int segments_x_ray(double X, double Y, const struct line_pnts *Points)
{
    double x1, x2, y1, y2;
    double x_inter;
    int n_intersects;
    int n;

    G_debug(3, "segments_x_ray(): x = %f y = %f n_points = %d", X, Y,
	    Points->n_points);

    /* Follow the ray from X,Y along positive x and find number of intersections.
     * Coordinates exactly on ray are considered to be slightly above. */

    n_intersects = 0;
    for (n = 1; n < Points->n_points; n++) {
	x1 = Points->x[n - 1];
	y1 = Points->y[n - 1];
	x2 = Points->x[n];
	y2 = Points->y[n];

	/* G_debug() is slow, avoid it in loops over points,
	 * activate when needed */
	/*
	G_debug(3, "X = %f Y = %f x1 = %f y1 = %f x2 = %f y2 = %f", X, Y, x1,
		y1, x2, y2);
	 */

	/* I know, it should be possible to do that with less conditions,
	 * but it should be enough readable also! */

	/* first, skip segments that obviously do not intersect with test ray */

	/* segment above (X is not important) */
	if (y1 > Y && y2 > Y)
	    continue;

	/* segment below (X is not important) */
	if (y1 < Y && y2 < Y)
	    continue;

	/* segment left from X -> no intersection */
	if (x1 < X && x2 < X)
	    continue;

	/* point on vertex */
	if ((x1 == X && y1 == Y) || (x2 == X && y2 == Y))
	    return -1;

	/* on vertical boundary */
	if (x1 == x2 && x1 == X) {
	    if ((y1 <= Y && y2 >= Y) || (y1 >= Y && y2 <= Y))
		return -1;
	}

	/* on horizontal boundary */
	if (y1 == y2 && y1 == Y) {
	    if ((x1 <= X && x2 >= X) || (x1 >= X && x2 <= X))
		return -1;
	    else
		continue; 	/* segment on ray (X is not important) */
	}

	/* segment on ray (X is not important) */
	/* if (y1 == Y && y2 == Y)
	    continue; */

	/* one end on Y second above (X is not important) */
	if ((y1 == Y && y2 > Y) || (y2 == Y && y1 > Y))
	    continue;

	/* For following cases we know that at least one of x1 and x2 is >= X */

	/* one end of segment on Y second below Y */
	if (y1 == Y && y2 < Y) {
	    if (x1 >= X)	/* x of the end on the ray is >= X */
		n_intersects++;
	    continue;
	}
	if (y2 == Y && y1 < Y) {
	    if (x2 >= X)
		n_intersects++;
	    continue;
	}

	/* one end of segment above Y second below Y */
	if ((y1 < Y && y2 > Y) || (y1 > Y && y2 < Y)) {
	    if (x1 >= X && x2 >= X) {
		n_intersects++;
		continue;
	    }

	    /* now either x1 < X && x2 > X or x1 > X && x2 < X -> calculate intersection */
	    x_inter = dig_x_intersect(x1, x2, y1, y2, Y);
	    G_debug(3, "x_inter = %f", x_inter);
	    if (x_inter == X)
		return -1;	/* point on segment, do not assume inside/outside */
	    else if (x_inter > X)
		n_intersects++;

	    continue;		/* would not be necessary, just to check, see below */
	}
	/* should not be reached (one condition is not necessary, but it is maybe better readable
	 * and it is a check) */
	G_warning
	    ("segments_x_ray() %s: X = %f Y = %f x1 = %f y1 = %f x2 = %f y2 = %f",
	     _("conditions failed"), X, Y, x1, y1, x2, y2);
    }

    return n_intersects;
}

/*!
   \brief Determines if a point (X,Y) is inside a polygon.

   \param X,Y point coordinates
   \param Points polygon

   \return 0 - outside
   \return 1 - inside 
   \return 2 - on the boundary
 */
int Vect_point_in_poly(double X, double Y, const struct line_pnts *Points)
{
    int n_intersects;

    G_debug(3, "Vect_point_in_poly(): x = %f y = %f n_points = %d", X, Y,
	    Points->n_points);

    n_intersects = segments_x_ray(X, Y, Points);

    if (n_intersects == -1)
	return 2;

    /* odd number of intersections: inside, return 1 
     * even number of intersections: outside, return 0 */
    return (n_intersects & 1);
}

/*!
   \brief Determines if a point (X,Y) is inside an area outer ring. Islands are not considered.

   \param X,Y point coordinates
   \param Map vector map
   \param area area id
   \param box area bounding box

   \return 0 - outside
   \return 1 - inside 
   \return 2 - on the boundary
 */
int
Vect_point_in_area_outer_ring(double X, double Y, const struct Map_info *Map,
			      int area, struct bound_box *box)
{
    static int first = 1;
    int n_intersects, inter;
    int i, line;
    static struct line_pnts *Points;
    const struct Plus_head *Plus;
    struct P_area *Area;

    /* keep in sync with Vect_point_in_island() */

    G_debug(3, "Vect_point_in_area_outer_ring(): x = %f y = %f area = %d",
            X, Y, area);

    if (first == 1) {
	Points = Vect_new_line_struct();
	first = 0;
    }

    Plus = &(Map->plus);
    Area = Plus->Area[area];

    /* First it must be in box */
    if (X < box->W || X > box->E || Y > box->N || Y < box->S)
	return 0;

    n_intersects = 0;
    for (i = 0; i < Area->n_lines; i++) {
	line = abs(Area->lines[i]);

	Vect_read_line(Map, Points, NULL, line);

	/* if the bbox of the line would be available, 
	 * the bbox could be used for a first check: */

	/* Vect_line_box(Points, &lbox);
	 * do not check lines that obviously do not intersect with test ray */
	/* if ((lbox.N < Y) || (lbox.S > Y) || (lbox.E < X))
	    continue;
	 */

	/* retrieving the bbox from the spatial index or 
	 * calculating the box from the vertices is slower than
	 * just feeding the line to segments_x_ray() */

	inter = segments_x_ray(X, Y, Points);
	if (inter == -1)
	    return 2;
	n_intersects += inter;
    }

    /* odd number of intersections: inside, return 1 
     * even number of intersections: outside, return 0 */
    return (n_intersects & 1);
}

/*!
   \brief Determines if a point (X,Y) is inside an island.

   \param X,Y point coordinates
   \param Map vector map
   \param isle isle id
   \param box isle bounding box

   \return 0 - outside
   \return 1 - inside 
   \return 2 - on the boundary
 */
int Vect_point_in_island(double X, double Y, const struct Map_info *Map,
                         int isle, struct bound_box *box)
{
    static int first = 1;
    int n_intersects, inter;
    int i, line;
    static struct line_pnts *Points;
    const struct Plus_head *Plus;
    struct P_isle *Isle;

    /* keep in sync with Vect_point_in_area_outer_ring() */

    G_debug(3, "Vect_point_in_island(): x = %f y = %f isle = %d",
            X, Y, isle);

    if (first == 1) {
	Points = Vect_new_line_struct();
	first = 0;
    }

    Plus = &(Map->plus);
    Isle = Plus->Isle[isle];

    /* First it must be in box */
    if (X < box->W || X > box->E || Y > box->N || Y < box->S)
	return 0;

    n_intersects = 0;
    for (i = 0; i < Isle->n_lines; i++) {
	line = abs(Isle->lines[i]);

	Vect_read_line(Map, Points, NULL, line);

	/* if the bbox of the line would be available, 
	 * the bbox could be used for a first check: */

	/* Vect_line_box(Points, &lbox);
	 * don't check lines that obviously do not intersect with test ray */
	/* if ((lbox.N < Y) || (lbox.S > Y) || (lbox.E < X))
	    continue;
	 */

	/* retrieving the bbox from the spatial index or 
	 * calculating the box from the vertices is slower than
	 * just feeding the line to segments_x_ray() */

	inter = segments_x_ray(X, Y, Points);
	if (inter == -1)
	    return 2;
	n_intersects += inter;
    }

    /* odd number of intersections: inside, return 1 
     * even number of intersections: outside, return 0 */
    return (n_intersects & 1);
}
