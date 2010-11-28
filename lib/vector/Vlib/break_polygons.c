/*!
   \file break_polygons.c

   \brief Vector library - clean geometry (break polygons)

   Higher level functions for reading/writing/manipulating vectors.

   (C) 2001-2009 by the GRASS Development Team

   This program is free software under the 
   GNU General Public License (>=v2). 
   Read the file COPYING that comes with GRASS
   for details.

   \author Radim Blazek
   \author Update for GRASS 7 Markus Metz
 */

#include <grass/config.h>
#include <stdlib.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>

/* TODO: 3D support
 *
 * atan2() gives angle from x-axis
 * this is unambiguous only in 2D, not in 3D
 *
 * one possibility would be to store unit vectors of length 1
 * in struct XPNT
 * double a1[3], a2[3];
 * 
 * length = sqrt(dx * dx + dy * dy + dz * dz);
 * dx /= length; dy /= length; dz /=length;
 * a1[0] = dx; a1[1] = dy; a1[2] = dz;
 *
 * get second dx, dy, dz
 * length = sqrt(dx * dx + dy * dy + dz * dz);
 * dx /= length; dy /= length; dz /=length;
 * a2[0] = dx; a2[1] = dy; a2[2] = dz;
 *
 * equal angles
 * if (a1[0] == a2[0] && a1[1] == a2[1] && a1[2] == a2[2])
 *
 * disadvantage: increased memory consumption
 *
 * new function Vect_break_faces() ?
 * 
 */

typedef struct
{
    double x, y;		/* coords */
    double a1, a2;		/* angles */
    char cross;			/* 0 - do not break, 1 - break */
} XPNT;

/* function used by binary tree to compare items */

static int compare_xpnts(const void *Xpnta, const void *Xpntb)
{
    XPNT *a, *b;

    a = (XPNT *)Xpnta;
    b = (XPNT *)Xpntb;

    if (a->x > b->x)
	return 1;
    else if (a->x < b->x)
	return -1;
    else {
	if (a->y > b->y)
	    return 1;
	else if (a->y < b->y)
	    return -1;
	else
	    return 0;
    }

    G_warning(_("Break polygons: Bug in binary tree!"));
    return 1;
}

/*!
   \brief Break polygons in vector map.

   Breaks lines specified by type in vector map. Points at
   intersections may be optionally written to error map. Input map
   must be opened on level 2 for update at least on GV_BUILD_BASE.

   Function is optimized for closed polygons rigs (e.g. imported from
   OGR) but with clean geometry - adjacent polygons mostly have
   identical boundary. Function creates database of ALL points in the
   map, and then is looking for those where polygons should be broken.
   Lines may be broken only at points existing in input map!

   \param Map input map where polygons will be broken
   \param type type of line to be broken
   \param Err vector map where points at intersections will be written or NULL

   \return
 */

void
Vect_break_polygons(struct Map_info *Map, int type, struct Map_info *Err)
{
    struct line_pnts *BPoints, *Points;
    struct line_cats *Cats, *ErrCats;
    int i, j, k, ret, ltype, broken, last, nlines;
    int nbreaks;
    struct RB_TREE *RBTree;
    int apoints, npoints, nallpoints, nmarks;
    XPNT *XPnt_found, XPnt_search;
    double dx, dy, a1 = 0, a2 = 0;
    int closed, last_point, cross;

    RBTree = rbtree_create(compare_xpnts, sizeof(XPNT));

    BPoints = Vect_new_line_struct();
    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();
    ErrCats = Vect_new_cats_struct();

    nlines = Vect_get_num_lines(Map);

    G_debug(3, "nlines =  %d", nlines);
    /* Go through all lines in vector, and add each point to structure of points,
     * if such point already exists check angles of segments and if differ mark for break */

    apoints = 0;
    nmarks = 0;
    npoints = 0;
    nallpoints = 0;

    G_verbose_message(_("Break polygons Pass 1: select break points"));

    for (i = 1; i <= nlines; i++) {
	G_percent(i, nlines, 1);
	G_debug(3, "i =  %d", i);
	if (!Vect_line_alive(Map, i))
	    continue;

	ltype = Vect_read_line(Map, Points, Cats, i);
	if (!(ltype & type))
	    continue;

	/* This would be confused by duplicate coordinates (angle cannot be calculated) ->
	 * prune line first */
	Vect_line_prune(Points);

	/* If first and last point are identical it is close polygon, we dont need to register last point
	 * and we can calculate angle for first.
	 * If first and last point are not identical we have to mark for break both */
	last_point = Points->n_points - 1;
	if (Points->x[0] == Points->x[last_point] &&
	    Points->y[0] == Points->y[last_point])
	    closed = 1;
	else
	    closed = 0;

	for (j = 0; j < Points->n_points; j++) {
	    G_debug(3, "j =  %d", j);
	    nallpoints++;

	    if (j == last_point && closed)
		continue;	/* do not register last of close polygon */

	    XPnt_search.x = Points->x[j];
	    XPnt_search.y = Points->y[j];

	    /* Already in DB? */
	    XPnt_found = rbtree_find(RBTree, &XPnt_search);

	    if (Points->n_points <= 2 ||
		(!closed && (j == 0 || j == last_point))) {
		cross = 1;	/* mark for cross in any case */
	    }
	    else {		/* calculate angles */
		cross = 0;
		if (j == 0 && closed) {	/* closed polygon */
		    dx = Points->x[last_point] - Points->x[0];
		    dy = Points->y[last_point] - Points->y[0];
		    a1 = atan2(dy, dx);
		    dx = Points->x[1] - Points->x[0];
		    dy = Points->y[1] - Points->y[0];
		    a2 = atan2(dy, dx);
		}
		else {
		    dx = Points->x[j - 1] - Points->x[j];
		    dy = Points->y[j - 1] - Points->y[j];
		    a1 = atan2(dy, dx);
		    dx = Points->x[j + 1] - Points->x[j];
		    dy = Points->y[j + 1] - Points->y[j];
		    a2 = atan2(dy, dx);
		}
	    }

	    if (XPnt_found) {	/* found */
		if (XPnt_found->cross == 1)
		    continue;	/* already marked */

		/* check angles */
		if (cross) {
		    XPnt_found->cross = 1;
		    nmarks++;
		}
		else {
		    G_debug(3, "a1 = %f xa1 = %f a2 = %f xa2 = %f", a1,
			    XPnt_found->a1, a2, XPnt_found->a2);
		    if ((a1 == XPnt_found->a1 && a2 == XPnt_found->a2) ||
		        (a1 == XPnt_found->a2 && a2 == XPnt_found->a1)) {	/* identical */

		    }
		    else {
			XPnt_found->cross = 1;
			nmarks++;
		    }
		}
	    }
	    else {
		if (j == 0 || j == (Points->n_points - 1) ||
		    Points->n_points < 3) {
		    XPnt_search.a1 = 0;
		    XPnt_search.a2 = 0;
		    XPnt_search.cross = 1;
		    nmarks++;
		}
		else {
		    XPnt_search.a1 = a1;
		    XPnt_search.a2 = a2;
		    XPnt_search.cross = 0;
		}

		/* Add to tree */
		rbtree_insert(RBTree, &XPnt_search);
		npoints++;
	    }
	}
    }

    nbreaks = 0;
    nallpoints = 0;
    G_debug(2, "Break polygons: unique vertices: %d", RBTree->count);

    /* uncomment to check if search tree is healthy */
    /* if (rbtree_debug(RBTree, RBTree->root) == 0)
	G_warning("Break polygons: RBTree not ok"); */

    /* Second loop through lines (existing when loop is started, no need to process lines written again)
     * and break at points marked for break */

    G_verbose_message(_("Break polygons Pass 2: break at selected points"));

    for (i = 1; i <= nlines; i++) {
	int n_orig_points;

	G_percent(i, nlines, 1);
	G_debug(3, "i =  %d", i);
	if (!Vect_line_alive(Map, i))
	    continue;

	ltype = Vect_read_line(Map, Points, Cats, i);
	if (!(ltype & type))
	    continue;
	if (!(ltype & GV_LINES))
	    continue;		/* Nonsense to break points */

	/* Duplicates would result in zero length lines -> prune line first */
	n_orig_points = Points->n_points;
	Vect_line_prune(Points);

	broken = 0;
	last = 0;
	G_debug(3, "n_points =  %d", Points->n_points);
	for (j = 1; j < Points->n_points; j++) {
	    G_debug(3, "j =  %d", j);
	    nallpoints++;

	    if (Points->n_points <= 1 ||
		(j == (Points->n_points - 1) && !broken))
		break;
	    /* One point only or 
	     * last point and line is not broken, do nothing */

	    XPnt_search.x = Points->x[j];
	    XPnt_search.y = Points->y[j];

	    XPnt_found = rbtree_find(RBTree, &XPnt_search);

	    /* all points must be in the search tree, without duplicates */
	    if (XPnt_found == NULL)
		G_fatal_error(_("Point not in search tree!"));

	    /* break or write last segment of broken line */
	    if ((j == (Points->n_points - 1) && broken) ||
		XPnt_found->cross) {
		Vect_reset_line(BPoints);
		for (k = last; k <= j; k++) {
		    Vect_append_point(BPoints, Points->x[k], Points->y[k],
				      Points->z[k]);
		}

		/* Result may collapse to one point */
		Vect_line_prune(BPoints);
		if (BPoints->n_points > 1) {
		    ret = Vect_write_line(Map, ltype, BPoints, Cats);
		    G_debug(3,
			    "Line %d written j = %d n_points(orig,pruned) = %d n_points(new) = %d",
			    ret, j, Points->n_points, BPoints->n_points);
		}

		if (!broken)
		    Vect_delete_line(Map, i);	/* not yet deleted */

		/* Write points on breaks */
		if (Err) {
		    if (j < (Points->n_points - 1)) {
			Vect_reset_line(BPoints);
			Vect_append_point(BPoints, Points->x[j], Points->y[j], 0);
			Vect_write_line(Err, GV_POINT, BPoints, ErrCats);
		    }
		}

		last = j;
		broken = 1;
		nbreaks++;
	    }
	}
	if (!broken && n_orig_points > Points->n_points) {	/* was pruned before -> rewrite */
	    if (Points->n_points > 1) {
		Vect_rewrite_line(Map, i, ltype, Points, Cats);
		G_debug(3, "Line %d pruned, npoints = %d", i,
			Points->n_points);
	    }
	    else {
		Vect_delete_line(Map, i);
		G_debug(3, "Line %d was deleted", i);
	    }
	}
	else {
	    G_debug(3, "Line %d was not changed", i);
	}
    }

    rbtree_destroy(RBTree);
    Vect_destroy_line_struct(Points);
    Vect_destroy_line_struct(BPoints);
    Vect_destroy_cats_struct(Cats);
    Vect_destroy_cats_struct(ErrCats);
    G_verbose_message(_("Breaks: %d"), nbreaks);
}
