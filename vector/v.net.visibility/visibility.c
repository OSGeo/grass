
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


/* TODO take in account if it is long/lat projection */

#include "visibility.h"

/** for all points initiate their vis line to the one directly below
*/
void init_vis(struct Point *points, int num_points, struct Line *lines,
	      int num_lines)
{
    int i;
    double d;
    struct avl_table *tree = avl_create(cmp_points, NULL, NULL);
    struct avl_traverser it;
    struct Point *p;

    double y1, y2;

    struct Point *s1, *s2;

    for (i = 0; i < num_points; i++) {

	points[i].vis = NULL;
	d = PORT_DOUBLE_MAX;

	avl_t_init(&it, tree);

	/* loop through the tree */
	while ((p = avl_t_next(&it)) != NULL) {
	    if (segment1(p) == NULL && segment2(p) == NULL)
		continue;

	    /* test for intersection and get the intersecting point */
	    if (segment1(p) != NULL &&
		segment_intersect(segment1(p), &points[i], &y1) > -1) {
		/* find the closest one below */
		if (y1 < points[i].y && (points[i].y - y1) < d) {
		    d = points[i].y - y1;
		    points[i].vis = segment1(p);

		}
	    }


	    if (segment2(p) != NULL &&
		segment_intersect(segment2(p), &points[i], &y2) > -1) {
		if (y2 < points[i].y && (points[i].y - y2) < d) {
		    d = points[i].y - y2;
		    points[i].vis = segment2(p);
		}
	    }

	}			/* end loop */

	s1 = s2 = NULL;

	/* now if the other point is on the right, we can delete it */
	if (segment1(&points[i]) != NULL &&
	    cmp_points(&points[i], other1(&points[i]), NULL) > 0) {
	    p = other1(&points[i]);

	    /* unless the other point of it is on the left */
	    if (segment1(p) != NULL && other1(p) != &points[i] &&
		cmp_points(&points[i], other1(p), NULL) > 0)
		s1 = avl_delete(tree, p);
	    else if (segment2(p) != NULL && other2(p) != &points[i] &&
		     cmp_points(&points[i], other2(p), NULL) > 0)
		s1 = avl_delete(tree, p);
	}

	/* now if the other point is on the right, we can delete it */
	if (segment2(&points[i]) != NULL &&
	    cmp_points(&points[i], other2(&points[i]), NULL) > 0) {
	    p = other2(&points[i]);

	    /* unless the other point of it is on the left */
	    if (segment1(p) != NULL && other1(p) != &points[i] &&
		cmp_points(&points[i], other1(p), NULL) > 0)
		s2 = avl_delete(tree, p);
	    else if (segment2(p) != NULL && other2(p) != &points[i] &&
		     cmp_points(&points[i], other2(p), NULL) > 0)
		s2 = avl_delete(tree, p);
	}

	/* if both weren't deleted, it means there is at least one other
	   point on the left, so add the current */
	/* also there is no point adding the point if there is no segment attached to it */
	if ((s1 == NULL || s2 == NULL)) {
	    avl_insert(tree, &points[i]);
	}
    }


    avl_destroy(tree, NULL);
}



/** for a pair (p, q) of points, add the edge pq if their are visible to each other
*/
void handle(struct Point *p, struct Point *q, struct Map_info *out)
{

    /* if it's a point without segments, just report the edge */
    if (segment1(q) == NULL && segment2(q) == NULL && before(p, q, p->vis)) {
	report(p, q, out);
    }
    else if (segment1(p) != NULL && q == other1(p)) {
	/* we need to check if there is another segment at q so it can be set to the vis */
	if (segment1(q) == segment1(p) && segment2(q) != NULL &&
	    left_turn(p, q, other2(q))) {
	    p->vis = segment2(q);
	}
	else if (segment2(q) == segment1(p) && segment1(q) != NULL &&
		 left_turn(p, q, other1(q))) {
	    p->vis = segment1(q);
	}
	else
	    p->vis = q->vis;

	report(p, q, out);
    }
    else if (segment2(p) != NULL && q == other2(p)) {
	/* we need to check if there is another segment at q so it can be set to the vis */
	if (segment1(q) == segment2(p) && segment2(q) != NULL &&
	    left_turn(p, q, other2(q))) {
	    p->vis = segment2(q);
	}
	else if (segment2(q) == segment2(p) && segment1(q) != NULL &&
		 left_turn(p, q, other1(q))) {
	    p->vis = segment1(q);
	}
	else
	    p->vis = q->vis;

	report(p, q, out);
    }
    else if (segment1(q) == p->vis && segment1(q) != NULL) {
	/* we need to check if there is another segment at q so it can be set to the vis */
	if (segment2(q) != NULL && left_turn(p, q, other2(q)))
	    p->vis = segment2(q);
	else
	    p->vis = q->vis;

	/* check that p and q are not on the same boundary and that the edge pq is inside the boundary */
	if (p->cat == -1 || p->cat != q->cat ||
	    !point_inside(p, (p->x + q->x) * 0.5, (p->y + q->y) * 0.5))
	    report(p, q, out);
    }
    else if (segment2(q) == p->vis && segment2(q) != NULL) {
	/* we need to check if there is another segment at q so it can be set to the vis */
	if (segment1(q) != NULL && left_turn(p, q, other1(q)))
	    p->vis = segment1(q);
	else
	    p->vis = q->vis;

	/* check that p and q are not on the same boundary and that the edge pq is inside the boundary */
	if (p->cat == -1 || p->cat != q->cat ||
	    !point_inside(p, (p->x + q->x) * 0.5, (p->y + q->y) * 0.5))
	    report(p, q, out);
    }
    else if (before(p, q, p->vis)) {
	/* if q only has one segment, then this is the new vis */
	if (segment2(q) == NULL)
	    p->vis = segment1(q);
	else if (segment1(q) == NULL)
	    p->vis = segment2(q);

	/* otherwise take the one with biggest slope */
	else if (left_turn(p, q, other1(q)) && !left_turn(p, q, other2(q)))
	    p->vis = segment1(q);
	else if (!left_turn(p, q, other1(q)) && left_turn(p, q, other2(q)))
	    p->vis = segment2(q);
	else if (left_turn(q, other2(q), other1(q)))
	    p->vis = segment1(q);
	else
	    p->vis = segment2(q);

	/* check that p and q are not on the same boundary and that the edge pq is inside the boundary */
	if (p->cat == -1 || p->cat != q->cat ||
	    !point_inside(p, (p->x + q->x) * 0.5, (p->y + q->y) * 0.5))
	    report(p, q, out);
    }
}

/** add the edge pq to the map out 
*/
void report(struct Point *p, struct Point *q, struct Map_info *out)
{
    struct line_pnts *sites;
    struct line_cats *cats;
    double *tmpx = G_malloc(2 * sizeof(double));
    double *tmpy = G_malloc(2 * sizeof(double));

    sites = Vect_new_line_struct();
    cats = Vect_new_cats_struct();

    tmpx[0] = p->x;
    tmpx[1] = q->x;
    tmpy[0] = p->y;
    tmpy[1] = q->y;

    Vect_copy_xyz_to_pnts(sites, tmpx, tmpy, 0, 2);
    Vect_write_line(out, GV_LINE, sites, cats);

    Vect_destroy_line_struct(sites);
    Vect_destroy_cats_struct(cats);
    G_free(tmpx);
    G_free(tmpy);
}

/** the algorithm that computes the visibility graph
*/
void construct_visibility(struct Point *points, int num_points,
			  struct Line *lines, int num_lines,
			  struct Map_info *out)
{
    struct Point *p, *p_r, *q, *z;
    struct Point *p_infinity, *p_ninfinity;
    int i;

    p_ninfinity = (struct Point *)malloc(sizeof(struct Point));
    p_infinity = (struct Point *)malloc(sizeof(struct Point));

    p_ninfinity->x = PORT_DOUBLE_MAX;
    p_ninfinity->y = -PORT_DOUBLE_MAX;
    p_ninfinity->father = NULL;
    p_ninfinity->left_brother = NULL;
    p_ninfinity->right_brother = NULL;
    p_ninfinity->rightmost_son = NULL;

    p_infinity->x = PORT_DOUBLE_MAX;
    p_infinity->y = PORT_DOUBLE_MAX;
    p_infinity->father = NULL;
    p_infinity->left_brother = NULL;
    p_infinity->right_brother = NULL;
    p_infinity->rightmost_son = NULL;

    init_stack(num_points);

    /* sort points in decreasing x order */
    quickSort(points, 0, num_points - 1);

    /* initialize the vis pointer of the vertices */
    init_vis(points, num_points, lines, num_lines);

    add_rightmost(p_ninfinity, p_infinity);

    for (i = 0; i < num_points; i++) {
	add_rightmost(&points[i], p_ninfinity);
    }

    push(&points[0]);

    /* main loop */
    while (!empty_stack()) {

	p = pop();
	p_r = right_brother(p);
	q = father(p);

	/* if the father is not -infinity, handle p and q */
	if (q != p_ninfinity) {
	    handle(p, q, out);
	}

	z = left_brother(q);

	remove_point(p);

	/* remove and reattach p to the tree */
	if (z == NULL || !left_turn(p, z, father(z))) {
	    add_leftof(p, q);
	}
	else {

	    while (rightmost_son(z) != NULL &&
		   left_turn(p, rightmost_son(z), z))
		z = rightmost_son(z);

	    add_rightmost(p, z);

	    if (z == top())
		z = pop();
	}

	/* if p not attached to infinity, then p has more points to visit */
	if (left_brother(p) == NULL && father(p) != p_infinity) {
	    push(p);
	}

	/* and continue with the next one ( from left to right ) */
	if (p_r != NULL) {
	    push(p_r);
	}
    }

    G_free(p_infinity);
    G_free(p_ninfinity);
}


void visibility_points(struct Point *points, int num_points,
		       struct Line *lines, int num_lines,
		       struct Map_info *out, int n)
{

    int i, j, k;

    double x1, y1, z1, x2, y2, z2;

    /* loop through the points to add */
    for (i = 0; i < n; i++) {
	/* loop through the other points */
	for (j = 0; j < num_points - n; j++) {

	    /* loop trhough the lines */
	    for (k = 0; k < num_lines; k++) {
		if (segment1(&points[j]) == &lines[k] ||
		    segment2(&points[j]) == &lines[k])
		    continue;
		if (Vect_segment_intersection
		    (points[num_points - i - 1].x,
		     points[num_points - i - 1].y, 0, points[j].x,
		     points[j].y, 0, lines[k].p1->x, lines[k].p1->y, 0,
		     lines[k].p2->x, lines[k].p2->y, 0, &x1, &y1, &z1, &x2,
		     &y2, &z2, 0) != 0)
		    break;
	    }

	    if (k == num_lines)
		report(&points[num_points - i - 1], &points[j], out);
	}
    }
}
