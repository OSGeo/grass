/*!
   \file lib/vector/Vlib/break_polygons.c

   \brief Vector library - clean geometry (break polygons)

   Higher level functions for reading/writing/manipulating vectors.

   (C) 2001-2009 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2).  Read the file COPYING that comes with GRASS for details.

   \author Radim Blazek
   \author Update for GRASS 7 Markus Metz
 */

#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
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
    char used;			/* 0 - was not used to break line, 1 - was used to break line
				 *   this is stored because points are automatically marked as cross, even if not used 
				 *   later to break lines */
} XPNT;

typedef struct
{
    double a1, a2;		/* angles */
    char cross;			/* 0 - do not break, 1 - break */
    char used;			/* 0 - was not used to break line, 1 - was used to break line
				 *   this is stored because points are automatically marked as cross, even if not used 
				 *   later to break lines */
} XPNT2;

static int fpoint;

/* Function called from RTreeSearch for point found */
static void srch(int id, const struct RTree_Rect *rect, int *arg)
{
    fpoint = id;
}

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

/* break polygons using a file-based search index */
void Vect_break_polygons_file(struct Map_info *Map, int type, struct Map_info *Err)
{
    struct line_pnts *BPoints, *Points;
    struct line_cats *Cats, *ErrCats;
    int i, j, k, ret, ltype, broken, last, nlines;
    int nbreaks;
    struct RTree *RTree;
    int npoints, nallpoints, nmarks;
    XPNT2 XPnt;
    double dx, dy, a1 = 0, a2 = 0;
    int closed, last_point;
    char cross;
    int fd, xpntfd;
    char *filename;
    static struct RTree_Rect rect;
    static int rect_init = 0;

    if (!rect_init) {
	rect.boundary = G_malloc(6 * sizeof(RectReal));
	rect_init = 6;
    }
    
    G_debug(1, "File-based version of Vect_break_polygons()");

    filename = G_tempfile();
    fd = open(filename, O_RDWR | O_CREAT | O_EXCL, 0600);
    RTree = RTreeCreateTree(fd, 0, 2);
    remove(filename);

    filename = G_tempfile();
    xpntfd = open(filename, O_RDWR | O_CREAT | O_EXCL, 0600);
    remove(filename);

    BPoints = Vect_new_line_struct();
    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();
    ErrCats = Vect_new_cats_struct();

    nlines = Vect_get_num_lines(Map);

    G_debug(3, "nlines =  %d", nlines);
    /* Go through all lines in vector, and add each point to structure of points,
     * if such point already exists check angles of segments and if differ mark for break */

    nmarks = 0;
    npoints = 1;		/* index starts from 1 ! */
    nallpoints = 0;
    XPnt.used = 0;

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

	    /* Box */
	    rect.boundary[0] = Points->x[j];
	    rect.boundary[3] = Points->x[j];
	    rect.boundary[1] = Points->y[j];
	    rect.boundary[4] = Points->y[j];
	    rect.boundary[2] = 0;
	    rect.boundary[5] = 0;

	    /* Already in DB? */
	    fpoint = -1;
	    RTreeSearch(RTree, &rect, (void *)srch, NULL);
	    G_debug(3, "fpoint =  %d", fpoint);

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

	    if (fpoint > 0) {	/* Found */
		/* read point */
		lseek(xpntfd, (off_t) (fpoint - 1) * sizeof(XPNT2), SEEK_SET);
		read(xpntfd, &XPnt, sizeof(XPNT2));
		if (XPnt.cross == 1)
		    continue;	/* already marked */

		/* Check angles */
		if (cross) {
		    XPnt.cross = 1;
		    nmarks++;
		    /* write point */
		    lseek(xpntfd, (off_t) (fpoint - 1) * sizeof(XPNT2), SEEK_SET);
		    write(xpntfd, &XPnt, sizeof(XPNT2));
		}
		else {
		    G_debug(3, "a1 = %f xa1 = %f a2 = %f xa2 = %f", a1,
			    XPnt.a1, a2, XPnt.a2);
		    if ((a1 == XPnt.a1 && a2 == XPnt.a2) ||
		        (a1 == XPnt.a2 && a2 == XPnt.a1)) {	/* identical */

		    }
		    else {
			XPnt.cross = 1;
			nmarks++;
			/* write point */
			lseek(xpntfd, (off_t) (fpoint - 1) * sizeof(XPNT2), SEEK_SET);
			write(xpntfd, &XPnt, sizeof(XPNT2));
		    }
		}
	    }
	    else {
		/* Add to tree and to structure */
		RTreeInsertRect(&rect, npoints, RTree);
		if (j == 0 || j == (Points->n_points - 1) ||
		    Points->n_points < 3) {
		    XPnt.a1 = 0;
		    XPnt.a2 = 0;
		    XPnt.cross = 1;
		    nmarks++;
		}
		else {
		    XPnt.a1 = a1;
		    XPnt.a2 = a2;
		    XPnt.cross = 0;
		}
		/* write point */
		lseek(xpntfd, (off_t) (npoints - 1) * sizeof(XPNT2), SEEK_SET);
		write(xpntfd, &XPnt, sizeof(XPNT2));

		npoints++;
	    }
	}
    }

    nbreaks = 0;

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

	    /* Box */
	    rect.boundary[0] = Points->x[j];
	    rect.boundary[3] = Points->x[j];
	    rect.boundary[1] = Points->y[j];
	    rect.boundary[4] = Points->y[j];
	    rect.boundary[2] = 0;
	    rect.boundary[5] = 0;

	    if (Points->n_points <= 1 ||
		(j == (Points->n_points - 1) && !broken))
		break;
	    /* One point only or 
	     * last point and line is not broken, do nothing */

	    RTreeSearch(RTree, &rect, (void *)srch, 0);
	    G_debug(3, "fpoint =  %d", fpoint);

	    /* read point */
	    lseek(xpntfd, (off_t) (fpoint - 1) * sizeof(XPNT2), SEEK_SET);
	    read(xpntfd, &XPnt, sizeof(XPNT2));

	    /* break or write last segment of broken line */
	    if ((j == (Points->n_points - 1) && broken) ||
		XPnt.cross) {
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
		    if (XPnt.cross && !XPnt.used) {
			Vect_reset_line(BPoints);
			Vect_append_point(BPoints, Points->x[j], Points->y[j], 0);
			Vect_write_line(Err, GV_POINT, BPoints, ErrCats);
		    }
		    if (!XPnt.used) {
			XPnt.used = 1;
			/* write point */
			lseek(xpntfd, (off_t) (fpoint - 1) * sizeof(XPNT2), SEEK_SET);
			write(xpntfd, &XPnt, sizeof(XPNT2));
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

    close(RTree->fd);
    RTreeDestroyTree(RTree);
    close(xpntfd);
    Vect_destroy_line_struct(Points);
    Vect_destroy_line_struct(BPoints);
    Vect_destroy_cats_struct(Cats);
    Vect_destroy_cats_struct(ErrCats);
    G_verbose_message(_("Breaks: %d"), nbreaks);
}


/* break polygons using a memory-based search index */
void Vect_break_polygons_mem(struct Map_info *Map, int type, struct Map_info *Err)
{
    struct line_pnts *BPoints, *Points;
    struct line_cats *Cats, *ErrCats;
    int i, j, k, ret, ltype, broken, last, nlines;
    int nbreaks;
    struct RB_TREE *RBTree;
    int npoints, nallpoints, nmarks;
    XPNT *XPnt_found, XPnt_search;
    double dx, dy, a1 = 0, a2 = 0;
    int closed, last_point, cross;

    G_debug(1, "Memory-based version of Vect_break_polygons()");

    RBTree = rbtree_create(compare_xpnts, sizeof(XPNT));

    BPoints = Vect_new_line_struct();
    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();
    ErrCats = Vect_new_cats_struct();

    nlines = Vect_get_num_lines(Map);

    G_debug(3, "nlines =  %d", nlines);
    /* Go through all lines in vector, and add each point to structure of points,
     * if such point already exists check angles of segments and if differ mark for break */

    nmarks = 0;
    npoints = 0;
    nallpoints = 0;
    XPnt_search.used = 0;

    G_message(_("Breaking polygons (pass 1: select break points)..."));

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
    G_debug(2, "Break polygons: unique vertices: %ld", (long int)RBTree->count);

    /* uncomment to check if search tree is healthy */
    /* if (rbtree_debug(RBTree, RBTree->root) == 0)
	G_warning("Break polygons: RBTree not ok"); */

    /* Second loop through lines (existing when loop is started, no need to process lines written again)
     * and break at points marked for break */

    G_message(_("Breaking polygons (pass 2: break at selected points)..."));

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
		    if (XPnt_found->cross && !XPnt_found->used) {
			Vect_reset_line(BPoints);
			Vect_append_point(BPoints, Points->x[j], Points->y[j], 0);
			Vect_write_line(Err, GV_POINT, BPoints, ErrCats);
		    }
		    XPnt_found->used = 1;
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
    G_verbose_message(_("Breaks: %d"), nbreaks);
}

/*!
   \brief Break polygons in vector map

   Breaks lines specified by type in vector map. Points at
   intersections may be optionally written to error map. Input vector
   map must be opened on level 2 for update at least on GV_BUILD_BASE.

   Function is optimized for closed polygons rings (e.g. imported from
   OGR) but with clean geometry - adjacent polygons mostly have
   identical boundary. Function creates database of ALL points in the
   vector map, and then is looking for those where polygons should be
   broken.  Lines may be broken only at points existing in input
   vector map!

   \param Map input map where polygons will be broken
   \param type type of line to be broken (GV_LINE or GV_BOUNDARY)
   \param Err vector map where points at intersections will be written or NULL
 */
void Vect_break_polygons(struct Map_info *Map, int type, struct Map_info *Err)
{
    if (getenv("GRASS_VECTOR_LOWMEM"))
	return Vect_break_polygons_file(Map, type, Err);
    else
	return Vect_break_polygons_mem(Map, type, Err);
}
