#include <stddef.h>
#include "data_types.h"
#include "memory.h"
#include "geometry.h"
#include "geom_primitives.h"
#include "edge.h"

static void find_lowest_cross_edge(struct edge *r_cw_l, struct vertex *s,
				   struct edge *l_ccw_r, struct vertex *u,
				   struct edge **l_lower,
				   struct vertex **org_l_lower,
				   struct edge **r_lower,
				   struct vertex **org_r_lower);

static void merge(struct edge *r_cw_l, struct vertex *s,
		  struct edge *l_ccw_r, struct vertex *u,
		  struct edge **l_tangent);

void divide(struct vertex *sites_sorted[], unsigned int l, unsigned int r,
	    struct edge **l_ccw, struct edge **r_cw)
{

    unsigned int n;
    unsigned int split;
    struct edge *l_ccw_l, *r_cw_l, *l_ccw_r, *r_cw_r, *l_tangent;
    struct edge *a, *b, *c;
    double c_p;

    n = r - l + 1;
    if (n == 2) {
	/* Base case #1 - 2 sites in region. Construct an edge from 
	   two sites in the region       */
	*l_ccw = *r_cw = create_edge(sites_sorted[l], sites_sorted[r]);
    }
    else if (n == 3) {
	/* Base case #2 - 3 sites. Construct a triangle or two edges */
	a = create_edge(sites_sorted[l], sites_sorted[l + 1]);
	b = create_edge(sites_sorted[l + 1], sites_sorted[r]);
	splice(a, b, sites_sorted[l + 1]);
	c_p =
	    CROSS_PRODUCT_3P(sites_sorted[l], sites_sorted[l + 1],
			     sites_sorted[r]);

	if (c_p > 0.0) {
	    /* Create a triangle */
	    c = join(a, sites_sorted[l], b, sites_sorted[r], right);
	    *l_ccw = a;
	    *r_cw = b;
	}
	else if (c_p < 0.0) {
	    /* Create a triangle */
	    c = join(a, sites_sorted[l], b, sites_sorted[r], left);
	    *l_ccw = c;
	    *r_cw = c;
	}
	else {
	    /* Cross product is zero. Sites are located on a line. 
	       Triangle cannot be created */
	    *l_ccw = a;
	    *r_cw = b;
	}
    }
    else if (n > 3) {
	/* Recursive case. Continue splitting */

	/* Splitting point */
	split = (l + r) / 2;

	/* Divide into two halves */
	divide(sites_sorted, l, split, &l_ccw_l, &r_cw_l);
	divide(sites_sorted, split + 1, r, &l_ccw_r, &r_cw_r);

	/* Merge the two triangulations */
	merge(r_cw_l, sites_sorted[split], l_ccw_r, sites_sorted[split + 1],
	      &l_tangent);

	/* The lower tangent added by merge may have invalidated 
	   l_ccw_l or r_cw_r. Update them if necessary. */
	if (ORG(l_tangent) == sites_sorted[l])
	    l_ccw_l = l_tangent;
	if (DEST(l_tangent) == sites_sorted[r])
	    r_cw_r = l_tangent;

	/* Update leftmost ccw edge and rightmost cw edge */
	*l_ccw = l_ccw_l;
	*r_cw = r_cw_r;
    }
}

/*
 *  Find the lowest cross edge of the two triangulations
 */
static void find_lowest_cross_edge(struct edge *r_cw_l, struct vertex *s,
				   struct edge *l_ccw_r, struct vertex *u,
				   struct edge **l_lower,
				   struct vertex **org_l_lower,
				   struct edge **r_lower,
				   struct vertex **org_r_lower)
{
    struct edge *l, *r;
    struct vertex *o_l, *o_r, *d_l, *d_r;
    boolean ready;

    l = r_cw_l;
    r = l_ccw_r;
    o_l = s;
    d_l = OTHER_VERTEX(l, s);
    o_r = u;
    d_r = OTHER_VERTEX(r, u);
    ready = FALSE;

    while (!ready)
	/* left_of */
	if (LEFT_OF(o_l, d_l, o_r)) {
	    l = PREV(l, d_l);
	    o_l = d_l;
	    d_l = OTHER_VERTEX(l, o_l);
	    /* right_of */
	}
	else if (RIGHT_OF(o_r, d_r, o_l)) {
	    r = NEXT(r, d_r);
	    o_r = d_r;
	    d_r = OTHER_VERTEX(r, o_r);
	}
	else
	    ready = TRUE;

    *l_lower = l;
    *r_lower = r;
    *org_l_lower = o_l;
    *org_r_lower = o_r;
}

/* 
 *  The most time-expensive function, most of the work gets done here.
 */
static void merge(struct edge *r_cw_l, struct vertex *s,
		  struct edge *l_ccw_r, struct vertex *u,
		  struct edge **l_tangent)
{
    struct edge *base, *l_cand, *r_cand;
    struct vertex *org_base, *dest_base;
    long double u_l_c_o_b, v_l_c_o_b, u_l_c_d_b, v_l_c_d_b;
    long double u_r_c_o_b, v_r_c_o_b, u_r_c_d_b, v_r_c_d_b;

    /* cross product */
    long double c_p_l_cand, c_p_r_cand;

    /* dot product */
    long double d_p_l_cand, d_p_r_cand;
    boolean above_l_cand, above_r_cand, above_next, above_prev;
    struct vertex *dest_l_cand, *dest_r_cand;
    long double cot_l_cand, cot_r_cand;
    struct edge *l_lower, *r_lower;
    struct vertex *org_r_lower, *org_l_lower;

    /* Create first cross edge by joining lower common tangent */
    find_lowest_cross_edge(r_cw_l, s, l_ccw_r, u, &l_lower, &org_l_lower,
			   &r_lower, &org_r_lower);
    base = join(l_lower, org_l_lower, r_lower, org_r_lower, right);
    org_base = org_l_lower;
    dest_base = org_r_lower;

    /* Need to return lower tangent. */
    *l_tangent = base;

    /* The merge loop */
    while (TRUE) {
	/* Initialise edges l_cand and r_cand */
	l_cand = NEXT(base, org_base);
	r_cand = PREV(base, dest_base);
	dest_l_cand = OTHER_VERTEX(l_cand, org_base);
	dest_r_cand = OTHER_VERTEX(r_cand, dest_base);

	/* Vectors used for above and modified IN_CIRCLE tests
	   u/v left/right candidate origin/destination */
	CREATE_VECTOR(dest_l_cand, org_base, u_l_c_o_b, v_l_c_o_b);
	CREATE_VECTOR(dest_l_cand, dest_base, u_l_c_d_b, v_l_c_d_b);
	CREATE_VECTOR(dest_r_cand, org_base, u_r_c_o_b, v_r_c_o_b);
	CREATE_VECTOR(dest_r_cand, dest_base, u_r_c_d_b, v_r_c_d_b);

	/* Above tests. */
	c_p_l_cand =
	    CROSS_PRODUCT_2V(u_l_c_o_b, v_l_c_o_b, u_l_c_d_b, v_l_c_d_b);
	c_p_r_cand =
	    CROSS_PRODUCT_2V(u_r_c_o_b, v_r_c_o_b, u_r_c_d_b, v_r_c_d_b);
	above_l_cand = c_p_l_cand > 0.0;
	above_r_cand = c_p_r_cand > 0.0;

	/* Terminate merge loop. No valid sites left in L or R.
	   The top-most cross-edge have already been added. */
	if (!above_l_cand && !above_r_cand)
	    break;

	/* Move to next l_cand ccw, delete the old l_cand edge, until the 
	   in_circle test gets invalid. */
	if (above_l_cand) {
	    double u_n_o_b, v_n_o_b, u_n_d_b, v_n_d_b;
	    double c_p_next, d_p_next, cot_next;
	    struct edge *next;
	    struct vertex *dest_next;

	    d_p_l_cand =
		DOT_PRODUCT_2V(u_l_c_o_b, v_l_c_o_b, u_l_c_d_b, v_l_c_d_b);
	    cot_l_cand = d_p_l_cand / c_p_l_cand;

	    while (TRUE) {
		next = NEXT(l_cand, org_base);
		dest_next = OTHER_VERTEX(next, org_base);
		CREATE_VECTOR(dest_next, org_base, u_n_o_b, v_n_o_b);
		CREATE_VECTOR(dest_next, dest_base, u_n_d_b, v_n_d_b);
		c_p_next =
		    CROSS_PRODUCT_2V(u_n_o_b, v_n_o_b, u_n_d_b, v_n_d_b);
		above_next = c_p_next > 0.0;

		if (!above_next)
		    break;	/* Terminate loop. */

		d_p_next = DOT_PRODUCT_2V(u_n_o_b, v_n_o_b, u_n_d_b, v_n_d_b);
		cot_next = d_p_next / c_p_next;

		if (cot_next > cot_l_cand)
		    break;	/* Terminate loop. */

		delete_edge(l_cand);
		l_cand = next;
		cot_l_cand = cot_next;
	    }
	}

	/* Essentially the same done for r_cand symmetrically. */
	/* Move to prev r_cand cw, delete the old r_cand edge, until the 
	   in_circle test gets invalid. */
	if (above_r_cand) {
	    double u_p_o_b, v_p_o_b, u_p_d_b, v_p_d_b;
	    double c_p_prev, d_p_prev, cot_prev;
	    struct edge *prev;
	    struct vertex *dest_prev;

	    d_p_r_cand =
		DOT_PRODUCT_2V(u_r_c_o_b, v_r_c_o_b, u_r_c_d_b, v_r_c_d_b);
	    cot_r_cand = d_p_r_cand / c_p_r_cand;

	    while (TRUE) {
		prev = PREV(r_cand, dest_base);
		dest_prev = OTHER_VERTEX(prev, dest_base);
		CREATE_VECTOR(dest_prev, org_base, u_p_o_b, v_p_o_b);
		CREATE_VECTOR(dest_prev, dest_base, u_p_d_b, v_p_d_b);
		c_p_prev =
		    CROSS_PRODUCT_2V(u_p_o_b, v_p_o_b, u_p_d_b, v_p_d_b);
		above_prev = c_p_prev > 0.0;

		if (!above_prev)
		    break;	/* Terminate. */

		d_p_prev = DOT_PRODUCT_2V(u_p_o_b, v_p_o_b, u_p_d_b, v_p_d_b);
		cot_prev = d_p_prev / c_p_prev;

		if (cot_prev > cot_r_cand)
		    break;	/* Terminate. */

		delete_edge(r_cand);
		r_cand = prev;
		cot_r_cand = cot_prev;
	    }
	}

	/*
	   Add a successive L-R cross edge from base 
	   If both candidates are valid, choose the one with smallest circumcircle. 
	   Set the base to the new L-R cross edge.
	 */
	dest_l_cand = OTHER_VERTEX(l_cand, org_base);
	dest_r_cand = OTHER_VERTEX(r_cand, dest_base);
	if (!above_l_cand ||
	    (above_l_cand && above_r_cand && cot_r_cand < cot_l_cand)) {
	    /* Connect to the right */
	    base = join(base, org_base, r_cand, dest_r_cand, right);
	    dest_base = dest_r_cand;
	}
	else {
	    /* Connect to the left */
	    base = join(l_cand, dest_l_cand, base, dest_base, right);
	    org_base = dest_l_cand;
	}
    }
}
