/*!
 * \file lib/vector/Vlib/snap.c
 *
 * \brief Vector library - Clean vector map (snap lines)
 *
 * Higher level functions for reading/writing/manipulating vectors.
 *
 * (C) 2001-2009 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2).  Read the file COPYING that comes with GRASS for details.
 *
 * \author Radim Blazek
 * \author update to GRASS 7 Markus Metz
 */

#include <grass/config.h>
#include <stdlib.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>

/* function prototypes */
static int sort_new(const void *pa, const void *pb);

/* Vertex */
typedef struct
{
    double x, y;
    double anchor_x, anchor_y;
    int anchor;			/* 0 - anchor, do not snap this point, that means snap others to this */
    /* >0  - index of anchor to which snap this point */
    /* -1  - init value */
} XPNT;

typedef struct
{
    double anchor_x, anchor_y;
    int anchor;
    double along;
} NEW;

/* This function is called by  RTreeSearch() to add selected node/line/area/isle to thelist */
int add_item(int id, struct ilist *list)
{
    dig_list_add(list, id);
    return 1;
}

/* function used by binary tree to compare items */

int compare_snappnts(const void *Xpnta, const void *Xpntb)
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

    G_warning("Break polygons: Bug in binary tree!");
    return 1;
}

/*!
  \brief Snap selected lines to existing vertex in threshold.
  
  Snap selected lines to existing vertices.
  
  \warning Lines are not necessarily snapped to nearest vertex, but to vertex in threshold! 
  
  Lines showing how vertices were snapped may be optionally written to error map. 
  Input map must be opened on level 2 for update at least on GV_BUILD_BASE.
  
  As mentioned above, lines are not necessarily snapped to nearest vertex! For example:
  <pre>
   |                    
   | 1         line 3 is snapped to line 1,
   |           then line 2 is not snapped to common node at lines 1 and 3,
   because it is already outside of threshold
   ----------- 3   

   |
   | 2
   |    
   </pre>
   
   The algorithm selects anchor vertices and snaps non-anchor vertices
   to these anchors.
   The distance between anchor vertices is always > threshold.
   If there is more than one anchor vertex within threshold around a
   non-anchor vertex, this vertex is snapped to the nearest anchor
   vertex within threshold.

   \param Map input map where vertices will be snapped
   \param List_lines list of lines to snap
   \param thresh threshold in which snap vertices
   \param[out] Err vector map where lines representing snap are written or NULL
   
   \return void
*/
void
Vect_snap_lines_list(struct Map_info *Map, const struct ilist *List_lines,
		     double thresh, struct Map_info *Err)
{
    struct line_pnts *Points, *NPoints;
    struct line_cats *Cats;
    int line, ltype, line_idx;
    double thresh2;
    double xmin, xmax, ymin, ymax;

    struct RB_TREE *RBTree;
    struct RB_TRAV RBTrav1, RBTrav2;
    int point;			/* index in points array */
    int nanchors, ntosnap;	/* number of anchors and number of points to be snapped */
    int nsnapped, ncreated;	/* number of snapped verices, number of new vertices (on segments) */
    int apoints, npoints, nvertices;	/* number of allocated points, registered points, vertices */
    XPNT *XPnt_found, *XPnt_found2, XPnt_search;	/* snap points */
    NEW *New = NULL;		/* Array of new points */
    int anew = 0, nnew;		/* allocated new points , number of new points */

    if (List_lines->n_values < 1)
	return;

    Points = Vect_new_line_struct();
    NPoints = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();
    RBTree = rbtree_create(compare_snappnts, sizeof(XPNT));

    thresh2 = thresh * thresh;

    /* Go through all lines in vector, and add each point to search tree
     * points are kept sorted in search tree first by x, then by y */
    apoints = 0;
    point = 1;			/* index starts from 1 ! */
    nvertices = 0;

    G_verbose_message(_("Snap vertices Pass 1: select points"));
    for (line_idx = 0; line_idx < List_lines->n_values; line_idx++) {
	int v;

	G_percent(line_idx, List_lines->n_values, 2);

	line = List_lines->value[line_idx];

	G_debug(3, "line =  %d", line);
	if (!Vect_line_alive(Map, line))
	    continue;

	ltype = Vect_read_line(Map, Points, Cats, line);

	for (v = 0; v < Points->n_points; v++) {
	    G_debug(3, "  vertex v = %d", v);
	    nvertices++;

	    XPnt_search.x = Points->x[v];
	    XPnt_search.y = Points->y[v];

	    /* Already registered ? */
	    XPnt_found = rbtree_find(RBTree, &XPnt_search);

	    if (XPnt_found == NULL) {	/* Not found */
		/* Add to tree */
		XPnt_search.anchor = -1;
		XPnt_search.anchor_x = Points->x[v];
		XPnt_search.anchor_y = Points->y[v];
		rbtree_insert(RBTree, &XPnt_search);
		point++;
	    }
	}
    } /* end of points selection */
    G_percent(line_idx, List_lines->n_values, 2); /* finish it */

    npoints = point - 1;

    /* Go through all registered points and if not yet marked,
     * mark it as anchor and assign this anchor to all not yet marked
     * points within threshold.
     * Update anchor for marked points if new anchor is closer. */

    G_verbose_message(_("Snap vertices Pass 2: assign anchor vertices"));

    nanchors = ntosnap = 0;
    rbtree_init_trav(&RBTrav1, RBTree);
    rbtree_init_trav(&RBTrav2, RBTree);
    point = 1;
    /* XPnts in the old version were not sorted, this causes subtle differences */
    while ((XPnt_found = rbtree_traverse(&RBTrav1)) != NULL) {
	double dx, dy, dist2;

	G_percent(point, npoints, 2);

	G_debug(3, "  point = %d", point);

	point++;

	if (XPnt_found->anchor >= 0)
	    continue;
	
	XPnt_found->anchor = 0;	/* make it anchor */
	nanchors++;

	/* Find points in threshold */
	xmin = XPnt_found->x - thresh;
	xmax = XPnt_found->x + thresh;
	ymin = XPnt_found->y - thresh;
	ymax = XPnt_found->y + thresh;

	XPnt_search.x = xmin;
	XPnt_search.y = ymin;

	/* traverse search tree from start point onwards */
	rbtree_init_trav(&RBTrav2, RBTree);
	while ((XPnt_found2 = rbtree_traverse_start(&RBTrav2, &XPnt_search)) != NULL) {
	    if (XPnt_found2->x > xmax)
		break;   /* outside x search limit */
		
	    /* not an anchor, and within y search limits */
	    if (XPnt_found2->anchor != 0 &&
		XPnt_found2->y >= ymin && XPnt_found2->y <= ymax) {

		dx = XPnt_found2->x - XPnt_found->x;
		dy = XPnt_found2->y - XPnt_found->y;
		if (dx == 0 && dy == 0)    /* XPnt_found2 == XPnt_found */
		    continue;
		    
		dist2 = dx * dx + dy * dy;

		if (dist2 > thresh2) /* outside threshold */
		    continue;
		    
		/* doesn't have an anchor yet */
		if (XPnt_found2->anchor == -1) {
		    XPnt_found2->anchor = 1;
		    XPnt_found2->anchor_x = XPnt_found->x;
		    XPnt_found2->anchor_y = XPnt_found->y;
		    ntosnap++;
		}
		else {   /* check distance to previously assigned anchor */
		    double dist2_a;

		    dx = XPnt_found2->anchor_x - XPnt_found2->x;
		    dy = XPnt_found2->anchor_y - XPnt_found2->y;
		    dist2_a = dx * dx + dy * dy;

		    /* replace old anchor */
		    if (dist2 < dist2_a) {
			XPnt_found2->anchor_x = XPnt_found->x;
			XPnt_found2->anchor_y = XPnt_found->y;
		    }
		}
	    }
	}
    } /* end of anchor assignment */

    /* Go through all lines and: 
     *   1) for all vertices: if not anchor snap it to its anchor
     *   2) for all segments: snap it to all anchors in threshold (except anchors of vertices of course) */

    nsnapped = ncreated = 0;

    G_verbose_message(_("Snap vertices Pass 3: snap to assigned points"));

    for (line_idx = 0; line_idx < List_lines->n_values; line_idx++) {
	int v;
	int changed = 0;

	G_percent(line_idx, List_lines->n_values, 1);

	line = List_lines->value[line_idx];

	G_debug(3, "line =  %d", line);
	if (!Vect_line_alive(Map, line))
	    continue;

	ltype = Vect_read_line(Map, Points, Cats, line);

	/* Snap all vertices */
	G_debug(2, "snap vertices");
	for (v = 0; v < Points->n_points; v++) {

	    /* Find point ( should always find one point ) */
	    XPnt_search.x = Points->x[v];
	    XPnt_search.y = Points->y[v];

	    XPnt_found = rbtree_find(RBTree, &XPnt_search);

	    if (XPnt_found == NULL)
		G_fatal_error("Snap lines: could not find vertex");

	    if (XPnt_found->anchor > 0) {	/* to be snapped */
		Points->x[v] = XPnt_found->anchor_x;
		Points->y[v] = XPnt_found->anchor_y;
		nsnapped++;
		changed = 1;
	    }
	}

	/* New points */
	Vect_reset_line(NPoints);

	/* Snap all segments to anchors in threshold */
	G_debug(2, "snap segments");
	for (v = 0; v < Points->n_points - 1; v++) {
	    int i;
	    double x1, x2, y1, y2;
	    double dist2, along;
	    int status;

	    x1 = Points->x[v];
	    x2 = Points->x[v + 1];
	    y1 = Points->y[v];
	    y2 = Points->y[v + 1];

	    Vect_append_point(NPoints, Points->x[v], Points->y[v],
			      Points->z[v]);

	    /* Search limits */
	    xmin = (x1 < x2 ? x1 : x2) - thresh;
	    xmax = (x1 > x2 ? x1 : x2) + thresh;
	    ymin = (y1 < y2 ? y1 : y2) - thresh;
	    ymax = (y1 > y2 ? y1 : y2) + thresh;

	    XPnt_search.x = xmin;
	    XPnt_search.y = ymin;

	    /* Find points within search limits */
	    nnew = 0;
	    rbtree_init_trav(&RBTrav2, RBTree);
	    G_debug(3, "snap segment");
	    while ((XPnt_found2 = rbtree_traverse_start(&RBTrav2, &XPnt_search)) != NULL) {

		if (XPnt_found2->x > xmax)
		    break;   /* outside x search limit */
		    
		/* found point must be within y search limits */
		if (XPnt_found2->y < ymin || XPnt_found2->y > ymax)
		    continue;

		/* found point must be anchor */
		if (XPnt_found2->anchor > 0)
		    continue;	/* point is not anchor */

		/* found point must not be end point */
		if ((XPnt_found2->x == x1 && XPnt_found2->y == y1) ||
		    (XPnt_found2->x == x2 && XPnt_found2->y == y2))
		    continue;	/* end point */

		/* Check the distance */
		dist2 =
		    dig_distance2_point_to_line(XPnt_found2->x,
						XPnt_found2->y, 0, x1, y1, 0,
						x2, y2, 0, 0, NULL, NULL,
						NULL, &along, &status);

		G_debug(4, "      distance = %lf", sqrt(dist2));

		/* status == 0 if point is w/in segment space
		 * avoids messy lines and boundaries */
		if (status == 0 && dist2 <= thresh2) {
		    G_debug(4, "      anchor in thresh, along = %lf", along);

		    if (nnew == anew) {
			anew += 100;
			New = (NEW *) G_realloc(New, anew * sizeof(NEW));
		    }
		    New[nnew].anchor_x = XPnt_found2->x;
		    New[nnew].anchor_y = XPnt_found2->y;
		    New[nnew].along = along;
		    nnew++;
		}
	    }
	    
	    G_debug(3, "  nnew = %d", nnew);
	    /* insert new vertices */
	    if (nnew > 0) {
		/* sort by distance along the segment */
		qsort(New, sizeof(char) * nnew, sizeof(NEW), sort_new);

		for (i = 0; i < nnew; i++) {
		    Vect_append_point(NPoints, New[i].anchor_x,
				      New[i].anchor_y, 0);
		    ncreated++;
		}
		changed = 1;
	    }
	}

	/* append end point */
	v = Points->n_points - 1;
	Vect_append_point(NPoints, Points->x[v], Points->y[v], Points->z[v]);

	if (changed) {		/* rewrite the line */
	    Vect_line_prune(NPoints);	/* remove duplicates */
	    if (NPoints->n_points > 1 || ltype & GV_LINES) {
		Vect_rewrite_line(Map, line, ltype, NPoints, Cats);
	    }
	    else {
		Vect_delete_line(Map, line);
	    }
	    if (Err) {
		Vect_write_line(Err, ltype, Points, Cats);
	    }
	}
    }				/* for each line */
    G_percent(line_idx, List_lines->n_values, 2); /* finish it */

    Vect_destroy_line_struct(Points);
    Vect_destroy_line_struct(NPoints);
    Vect_destroy_cats_struct(Cats);
    G_free(New);
    rbtree_destroy(RBTree);

    G_verbose_message(_("Snapped vertices: %d"), nsnapped);
    G_verbose_message(_("New vertices: %d"), ncreated);
}

/* for qsort */
static int sort_new(const void *pa, const void *pb)
{
    NEW *p1 = (NEW *) pa;
    NEW *p2 = (NEW *) pb;

    if (p1->along < p2->along)
	return -1;
    if (p1->along > p2->along)
	return 1;
    return 1;
}

/*!
 * \brief Snap lines in vector map to existing vertex in threshold.
 *
 * For details see Vect_snap_lines_list()
 *
 * \param[in] Map input map where vertices will be snapped
 * \param[in] type type of lines to snap
 * \param[in] thresh threshold in which snap vertices
 * \param[out] Err vector map where lines representing snap are written or NULL
 *
 * \return void
 */
void
Vect_snap_lines(struct Map_info *Map, int type, double thresh,
		struct Map_info *Err)
{
    int line, nlines, ltype;

    struct ilist *List;

    List = Vect_new_list();

    nlines = Vect_get_num_lines(Map);

    for (line = 1; line <= nlines; line++) {
	G_debug(3, "line =  %d", line);

	if (!Vect_line_alive(Map, line))
	    continue;

	ltype = Vect_read_line(Map, NULL, NULL, line);

	if (!(ltype & type))
	    continue;

	/* no need to check for duplicates:
	 * use dig_list_add() instead of Vect_list_append() */
	dig_list_add(List, line);
    }

    Vect_snap_lines_list(Map, List, thresh, Err);

    Vect_destroy_list(List);

    return;
}
