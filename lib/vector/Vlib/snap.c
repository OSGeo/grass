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

#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <grass/vector.h>
#include <grass/glocale.h>


/* Vertex */
typedef struct
{
    double x, y;
    int anchor;			/* 0 - anchor, do not snap this point, that means snap others to this */
    /* >0  - index of anchor to which snap this point */
    /* -1  - init value */
} XPNT;

typedef struct
{
    int anchor;
    double along;
} NEW;

/* for qsort */
static int sort_new(const void *pa, const void *pb)
{
    NEW *p1 = (NEW *) pa;
    NEW *p2 = (NEW *) pb;

    return (p1->along < p2->along ? -1 : 1);

    /*
    if (p1->along < p2->along)
	return -1;
    if (p1->along > p2->along)
	return 1;
    return 1;
    */
}

/* This function is called by RTreeSearch() to find a vertex */
static int find_item(int id, const struct RTree_Rect *rect, struct ilist *list)
{
    G_ilist_add(list, id);
    return 0;
}

/* This function is called by RTreeSearch() to add selected node/line/area/isle to the list */
static int add_item(int id, const struct RTree_Rect *rect, struct ilist *list)
{
    G_ilist_add(list, id);
    return 1;
}

/* for ilist qsort'ing and bsearch'ing */
int cmp_int(const void *a, const void *b)
{
    int ai = *(int *)a;
    int bi = *(int *)b;
    
    return (ai < bi ? -1 : (ai > bi));
}

/*!
   \brief Snap selected lines to existing vertex in threshold.
   
   Snap selected lines to existing vertices of other selected lines.
   
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

    int point;			/* index in points array */
    int nanchors, ntosnap;	/* number of anchors and number of points to be snapped */
    int nsnapped, ncreated;	/* number of snapped verices, number of new vertices (on segments) */
    int apoints, npoints, nvertices;	/* number of allocated points, registered points, vertices */
    XPNT *XPnts;		/* Array of points */
    NEW *New = NULL;		/* Array of new points */
    int anew = 0, nnew;		/* allocated new points , number of new points */
    struct ilist *List;
    int *Index = NULL;		/* indexes of anchors for vertices */
    int aindex = 0;		/* allocated Index */

    struct RTree *RTree;
    int rtreefd = -1;
    static struct RTree_Rect rect;
    static int rect_init = 0;

    if (!rect_init) {
	rect.boundary = G_malloc(6 * sizeof(RectReal));
	rect_init = 6;
    }

    if (List_lines->n_values < 1)
	return;

    Points = Vect_new_line_struct();
    NPoints = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();
    List = Vect_new_list();
    if (getenv("GRASS_VECTOR_LOWMEM")) {
	char *filename = G_tempfile();

	rtreefd = open(filename, O_RDWR | O_CREAT | O_EXCL, 0600);
	remove(filename);
    }
    RTree = RTreeCreateTree(rtreefd, 0, 2);

    thresh2 = thresh * thresh;

    /* Go through all lines in vector, and add each point to structure of points */
    apoints = 0;
    point = 1;			/* index starts from 1 ! */
    nvertices = 0;
    XPnts = NULL;

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

	    /* Box */
	    rect.boundary[0] = Points->x[v];
	    rect.boundary[3] = Points->x[v];
	    rect.boundary[1] = Points->y[v];
	    rect.boundary[4] = Points->y[v];
	    rect.boundary[2] = 0;
	    rect.boundary[5] = 0;

	    /* Already registered ? */
	    Vect_reset_list(List);
	    RTreeSearch(RTree, &rect, (void *)find_item, List);
	    G_debug(3, "List : nvalues =  %d", List->n_values);

	    if (List->n_values == 0) {	/* Not found */
		/* Add to tree and to structure */
		RTreeInsertRect(&rect, point, RTree);
		if ((point - 1) == apoints) {
		    apoints += 10000;
		    XPnts =
			(XPNT *) G_realloc(XPnts,
					   (apoints + 1) * sizeof(XPNT));
		}
		XPnts[point].x = Points->x[v];
		XPnts[point].y = Points->y[v];
		XPnts[point].anchor = -1;
		point++;
	    }
	}
    }
    G_percent(line_idx, List_lines->n_values, 2); /* finish it */

    npoints = point - 1;

    /* Go through all registered points and if not yet marked mark it as anchor and assign this anchor
     * to all not yet marked points in threshold */

    G_verbose_message(_("Snap vertices Pass 2: assign anchor vertices"));

    nanchors = ntosnap = 0;
    for (point = 1; point <= npoints; point++) {
	int i;

	G_percent(point, npoints, 2);

	G_debug(3, "  point = %d", point);

	if (XPnts[point].anchor >= 0)
	    continue;

	XPnts[point].anchor = 0;	/* make it anchor */
	nanchors++;

	/* Find points in threshold */
	rect.boundary[0] = XPnts[point].x - thresh;
	rect.boundary[3] = XPnts[point].x + thresh;
	rect.boundary[1] = XPnts[point].y - thresh;
	rect.boundary[4] = XPnts[point].y + thresh;
	rect.boundary[2] = 0;
	rect.boundary[5] = 0;

	Vect_reset_list(List);
	RTreeSearch(RTree, &rect, (void *)add_item, List);
	G_debug(4, "  %d points in threshold box", List->n_values);

	for (i = 0; i < List->n_values; i++) {
	    int pointb;
	    double dx, dy, dist2;

	    pointb = List->value[i];
	    if (pointb == point)
		continue;

	    dx = XPnts[pointb].x - XPnts[point].x;
	    dy = XPnts[pointb].y - XPnts[point].y;
	    dist2 = dx * dx + dy * dy;

	    if (dist2 > thresh2) /* outside threshold */
		continue;
		
	    /* doesn't have an anchor yet */
	    if (XPnts[pointb].anchor == -1) {
		XPnts[pointb].anchor = point;
		ntosnap++;
	    }
	    else if (XPnts[pointb].anchor > 0) {   /* check distance to previously assigned anchor */
		double dist2_a;

		dx = XPnts[XPnts[pointb].anchor].x - XPnts[pointb].x;
		dy = XPnts[XPnts[pointb].anchor].y - XPnts[pointb].y;
		dist2_a = dx * dx + dy * dy;

		/* replace old anchor */
		if (dist2 < dist2_a) {
		    XPnts[pointb].anchor = point;
		}
	    }
	}
    }

    /* Go through all lines and: 
     *   1) for all vertices: if not anchor snap it to its anchor
     *   2) for all segments: snap it to all anchors in threshold (except anchors of vertices of course) */

    nsnapped = ncreated = 0;

    G_verbose_message(_("Snap vertices Pass 3: snap to assigned points"));

    for (line_idx = 0; line_idx < List_lines->n_values; line_idx++) {
	int v, spoint, anchor;
	int changed = 0;

	G_percent(line_idx, List_lines->n_values, 2);

	line = List_lines->value[line_idx];

	G_debug(3, "line =  %d", line);
	if (!Vect_line_alive(Map, line))
	    continue;

	ltype = Vect_read_line(Map, Points, Cats, line);

	if (Points->n_points >= aindex) {
	    aindex = Points->n_points;
	    Index = (int *)G_realloc(Index, aindex * sizeof(int));
	}

	/* Snap all vertices */
	for (v = 0; v < Points->n_points; v++) {
	    /* Box */
	    rect.boundary[0] = Points->x[v];
	    rect.boundary[3] = Points->x[v];
	    rect.boundary[1] = Points->y[v];
	    rect.boundary[4] = Points->y[v];
	    rect.boundary[2] = 0;
	    rect.boundary[5] = 0;

	    /* Find point ( should always find one point ) */
	    Vect_reset_list(List);

	    RTreeSearch(RTree, &rect, (void *)add_item, List);

	    spoint = List->value[0];
	    anchor = XPnts[spoint].anchor;

	    if (anchor > 0) {	/* to be snapped */
		Points->x[v] = XPnts[anchor].x;
		Points->y[v] = XPnts[anchor].y;
		nsnapped++;
		changed = 1;
		Index[v] = anchor;	/* point on new location */
	    }
	    else {
		Index[v] = spoint;	/* old point */
	    }
	}

	/* New points */
	Vect_reset_line(NPoints);

	/* Snap all segments to anchors in threshold */
	for (v = 0; v < Points->n_points - 1; v++) {
	    int i;
	    double x1, x2, y1, y2, xmin, xmax, ymin, ymax;

	    G_debug(3, "  segment = %d end anchors : %d  %d", v, Index[v],
		    Index[v + 1]);

	    x1 = Points->x[v];
	    x2 = Points->x[v + 1];
	    y1 = Points->y[v];
	    y2 = Points->y[v + 1];

	    Vect_append_point(NPoints, Points->x[v], Points->y[v],
			      Points->z[v]);

	    /* Box */
	    if (x1 <= x2) {
		xmin = x1;
		xmax = x2;
	    }
	    else {
		xmin = x2;
		xmax = x1;
	    }
	    if (y1 <= y2) {
		ymin = y1;
		ymax = y2;
	    }
	    else {
		ymin = y2;
		ymax = y1;
	    }

	    rect.boundary[0] = xmin - thresh;
	    rect.boundary[3] = xmax + thresh;
	    rect.boundary[1] = ymin - thresh;
	    rect.boundary[4] = ymax + thresh;
	    rect.boundary[2] = 0;
	    rect.boundary[5] = 0;

	    /* Find points */
	    Vect_reset_list(List);
	    RTreeSearch(RTree, &rect, (void *)add_item, List);

	    G_debug(3, "  %d points in box", List->n_values);

	    /* Snap to anchor in threshold different from end points */
	    nnew = 0;
	    for (i = 0; i < List->n_values; i++) {
		double dist2, along;

		spoint = List->value[i];
		G_debug(4, "    spoint = %d anchor = %d", spoint,
			XPnts[spoint].anchor);

		if (spoint == Index[v] || spoint == Index[v + 1])
		    continue;	/* end point */
		if (XPnts[spoint].anchor > 0)
		    continue;	/* point is not anchor */

		/* Check the distance */
		dist2 =
		    dig_distance2_point_to_line(XPnts[spoint].x,
						XPnts[spoint].y, 0, x1, y1, 0,
						x2, y2, 0, 0, NULL, NULL,
						NULL, &along, NULL);

		G_debug(4, "      distance = %lf", sqrt(dist2));

		if (dist2 <= thresh2) {
		    G_debug(4, "      anchor in thresh, along = %lf", along);

		    if (nnew == anew) {
			anew += 100;
			New = (NEW *) G_realloc(New, anew * sizeof(NEW));
		    }
		    New[nnew].anchor = spoint;
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
		    anchor = New[i].anchor;
		    /* Vect_line_insert_point ( Points, ++v, XPnts[anchor].x, XPnts[anchor].y, 0); */
		    Vect_append_point(NPoints, XPnts[anchor].x,
				      XPnts[anchor].y, 0);
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
	    if (NPoints->n_points > 1 || !(ltype & GV_LINES)) {
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
    G_free(XPnts);
    G_free(Index);
    G_free(New);
    RTreeDestroyTree(RTree);
    if (rtreefd >= 0)
	close(rtreefd);

    G_verbose_message(_("Snapped vertices: %d"), nsnapped);
    G_verbose_message(_("New vertices: %d"), ncreated);
}

/*!
   \brief Snap selected lines to existing vertex in threshold.
   
   Snap selected lines to existing vertices of lines of type type.
   
   Lines in the list will only be snapped to lines not in the list.
   
   If a list is given, new snapped lines are added to the list.
   
   If no list is given, all lines of type type are snapped to lines of 
   type type.
   
   \param Map input map where vertices will be snapped
   \param List_lines list of lines to snap
   \param type type of lines where lines in list will be snapped to
   \param thresh threshold in which snap vertices
   \param[out] Err vector map where lines representing snap are written or NULL
   
   \return void
*/
void
Vect_snap_lines_list2(struct Map_info *Map, struct ilist *List_lines,
		     int type, double thresh, struct Map_info *Err)
{
    struct line_pnts *Points, *NPoints;
    struct line_cats *Cats;
    int line, ltype, line_idx, nlines, new_line, val;
    double thresh2;

    int point;			/* index in points array */
    int nanchors, ntosnap;	/* number of anchors and number of points to be snapped */
    int nsnapped, ncreated;	/* number of snapped verices, number of new vertices (on segments) */
    int apoints, npoints, nvertices;	/* number of allocated points, registered points, vertices */
    XPNT *XPnts;		/* Array of points */
    NEW *New = NULL;		/* Array of new points */
    int anew = 0, nnew;		/* allocated new points , number of new points */
    struct ilist *List;
    int *Index = NULL;		/* indexes of anchors for vertices */
    int aindex = 0;		/* allocated Index */

    struct RTree *RTree;
    int rtreefd = -1;
    struct RTree_Rect rect;

    rect.boundary = G_malloc(6 * sizeof(RectReal));

    if (List_lines) {
	nlines = List_lines->n_values;
	if (nlines < 1)
	    return;
	qsort(List_lines->value, nlines, sizeof(int), cmp_int);
    }

    Points = Vect_new_line_struct();
    NPoints = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();
    List = Vect_new_list();
    if (getenv("GRASS_VECTOR_LOWMEM")) {
	char *filename = G_tempfile();

	rtreefd = open(filename, O_RDWR | O_CREAT | O_EXCL, 0600);
	remove(filename);
    }
    RTree = RTreeCreateTree(rtreefd, 0, 2);

    thresh2 = thresh * thresh;

    /* Go through all lines in vector, and add each point to structure of points */
    apoints = 0;
    point = 1;			/* index starts from 1 ! */
    nvertices = 0;
    XPnts = NULL;

    nlines = Vect_get_num_lines(Map);

    G_verbose_message(_("Snap vertices Pass 1: select points"));
    for (line = 1; line <= nlines; line++) {
	int v;

	G_percent(line, nlines, 2);

	G_debug(3, "line =  %d", line);
	if (!Vect_line_alive(Map, line))
	    continue;

	ltype = Vect_read_line(Map, Points, Cats, line);
	val = -1;
	if (List_lines) {
	    if (bsearch(&line, List_lines->value, List_lines->n_values,
			sizeof(int), cmp_int)) {
		/* snap this line to other lines,
		 * don't make its vertices anchor points */
		val = -2;
	    }
	    else {
		if (!(ltype & type))
		    continue;
	    }
	}
	else {
	    if (!(ltype & type))
		continue;
	}

	for (v = 0; v < Points->n_points; v++) {
	    G_debug(3, "  vertex v = %d", v);
	    nvertices++;

	    /* Box */
	    rect.boundary[0] = Points->x[v];
	    rect.boundary[3] = Points->x[v];
	    rect.boundary[1] = Points->y[v];
	    rect.boundary[4] = Points->y[v];
	    rect.boundary[2] = 0;
	    rect.boundary[5] = 0;

	    /* Already registered ? */
	    Vect_reset_list(List);
	    RTreeSearch(RTree, &rect, (void *)find_item, List);
	    G_debug(3, "List : nvalues =  %d", List->n_values);

	    if (List->n_values == 0) {	/* Not found */
		/* Add to tree and to structure */
		RTreeInsertRect(&rect, point, RTree);
		if ((point - 1) == apoints) {
		    apoints += 10000;
		    XPnts =
			(XPNT *) G_realloc(XPnts,
					   (apoints + 1) * sizeof(XPNT));
		}
		XPnts[point].x = Points->x[v];
		XPnts[point].y = Points->y[v];
		XPnts[point].anchor = val;
		point++;
	    }
	}
    }

    npoints = point - 1;

    /* Go through all registered points and if not yet marked mark it as anchor and assign this anchor
     * to all not yet marked points in threshold */

    G_verbose_message(_("Snap vertices Pass 2: assign anchor vertices"));

    nanchors = ntosnap = 0;
    for (point = 1; point <= npoints; point++) {
	int i;

	G_percent(point, npoints, 2);

	G_debug(3, "  point = %d", point);

	/* also skip vertices of lines to snap */
	if (XPnts[point].anchor >= 0 || XPnts[point].anchor == -2)
	    continue;

	XPnts[point].anchor = 0;	/* make it anchor */
	nanchors++;

	/* Find points in threshold */
	rect.boundary[0] = XPnts[point].x - thresh;
	rect.boundary[3] = XPnts[point].x + thresh;
	rect.boundary[1] = XPnts[point].y - thresh;
	rect.boundary[4] = XPnts[point].y + thresh;
	rect.boundary[2] = 0;
	rect.boundary[5] = 0;

	Vect_reset_list(List);
	RTreeSearch(RTree, &rect, (void *)add_item, List);
	G_debug(4, "  %d points in threshold box", List->n_values);

	for (i = 0; i < List->n_values; i++) {
	    int pointb;
	    double dx, dy, dist2;

	    pointb = List->value[i];
	    if (pointb == point)
		continue;

	    dx = XPnts[pointb].x - XPnts[point].x;
	    dy = XPnts[pointb].y - XPnts[point].y;
	    dist2 = dx * dx + dy * dy;

	    if (dist2 > thresh2) /* outside threshold */
		continue;
		
	    /* doesn't have an anchor yet */
	    if (XPnts[pointb].anchor < 0) {
		XPnts[pointb].anchor = point;
		ntosnap++;
	    }
	    else if (XPnts[pointb].anchor > 0) {   /* check distance to previously assigned anchor */
		double dist2_a;

		dx = XPnts[XPnts[pointb].anchor].x - XPnts[pointb].x;
		dy = XPnts[XPnts[pointb].anchor].y - XPnts[pointb].y;
		dist2_a = dx * dx + dy * dy;

		/* replace old anchor */
		if (dist2 < dist2_a) {
		    XPnts[pointb].anchor = point;
		}
	    }
	}
    }

    /* Go through all lines and: 
     *   1) for all vertices: if not anchor snap it to its anchor
     *   2) for all segments: snap it to all anchors in threshold (except anchors of vertices of course) */

    nsnapped = ncreated = 0;

    G_verbose_message(_("Snap vertices Pass 3: snap to assigned points"));

    if (List_lines)
	nlines = List_lines->n_values;
    else
	nlines = Vect_get_num_lines(Map);
    for (line_idx = 0; line_idx < nlines; line_idx++) {
	int v, spoint, anchor;
	int changed = 0;

	G_percent(line_idx, nlines, 2);

	if (List_lines)
	    line = List_lines->value[line_idx];
	else
	    line = line_idx;

	G_debug(3, "line =  %d", line);
	if (!Vect_line_alive(Map, line))
	    continue;

	ltype = Vect_read_line(Map, Points, Cats, line);

	if (!List_lines && !(type & ltype))
	    continue;

	if (Points->n_points >= aindex) {
	    aindex = Points->n_points;
	    Index = (int *)G_realloc(Index, aindex * sizeof(int));
	}

	/* Snap all vertices */
	for (v = 0; v < Points->n_points; v++) {
	    /* Box */
	    rect.boundary[0] = Points->x[v];
	    rect.boundary[3] = Points->x[v];
	    rect.boundary[1] = Points->y[v];
	    rect.boundary[4] = Points->y[v];
	    rect.boundary[2] = 0;
	    rect.boundary[5] = 0;

	    /* Find point ( should always find one point ) */
	    Vect_reset_list(List);

	    RTreeSearch(RTree, &rect, (void *)add_item, List);

	    spoint = List->value[0];
	    anchor = XPnts[spoint].anchor;

	    if (anchor > 0) {	/* to be snapped */
		Points->x[v] = XPnts[anchor].x;
		Points->y[v] = XPnts[anchor].y;
		nsnapped++;
		changed = 1;
		Index[v] = anchor;	/* point on new location */
	    }
	    else {
		Index[v] = spoint;	/* old point */
	    }
	}

	/* New points */
	Vect_reset_line(NPoints);

	/* Snap all segments to anchors in threshold */
	for (v = 0; v < Points->n_points - 1; v++) {
	    int i;
	    double x1, x2, y1, y2, xmin, xmax, ymin, ymax;

	    G_debug(3, "  segment = %d end anchors : %d  %d", v, Index[v],
		    Index[v + 1]);

	    x1 = Points->x[v];
	    x2 = Points->x[v + 1];
	    y1 = Points->y[v];
	    y2 = Points->y[v + 1];

	    Vect_append_point(NPoints, Points->x[v], Points->y[v],
			      Points->z[v]);

	    /* Box */
	    if (x1 <= x2) {
		xmin = x1;
		xmax = x2;
	    }
	    else {
		xmin = x2;
		xmax = x1;
	    }
	    if (y1 <= y2) {
		ymin = y1;
		ymax = y2;
	    }
	    else {
		ymin = y2;
		ymax = y1;
	    }

	    rect.boundary[0] = xmin - thresh;
	    rect.boundary[3] = xmax + thresh;
	    rect.boundary[1] = ymin - thresh;
	    rect.boundary[4] = ymax + thresh;
	    rect.boundary[2] = 0;
	    rect.boundary[5] = 0;

	    /* Find points */
	    Vect_reset_list(List);
	    RTreeSearch(RTree, &rect, (void *)add_item, List);

	    G_debug(3, "  %d points in box", List->n_values);

	    /* Snap to anchor in threshold different from end points */
	    nnew = 0;
	    for (i = 0; i < List->n_values; i++) {
		double dist2, along;

		spoint = List->value[i];
		G_debug(4, "    spoint = %d anchor = %d", spoint,
			XPnts[spoint].anchor);

		if (spoint == Index[v] || spoint == Index[v + 1])
		    continue;	/* end point */
		if (XPnts[spoint].anchor > 0)
		    continue;	/* point is not anchor */

		/* Check the distance */
		dist2 =
		    dig_distance2_point_to_line(XPnts[spoint].x,
						XPnts[spoint].y, 0, x1, y1, 0,
						x2, y2, 0, 0, NULL, NULL,
						NULL, &along, NULL);

		G_debug(4, "      distance = %lf", sqrt(dist2));

		if (dist2 <= thresh2) {
		    G_debug(4, "      anchor in thresh, along = %lf", along);

		    if (nnew == anew) {
			anew += 100;
			New = (NEW *) G_realloc(New, anew * sizeof(NEW));
		    }
		    New[nnew].anchor = spoint;
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
		    anchor = New[i].anchor;
		    /* Vect_line_insert_point ( Points, ++v, XPnts[anchor].x, XPnts[anchor].y, 0); */
		    Vect_append_point(NPoints, XPnts[anchor].x,
				      XPnts[anchor].y, 0);
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
	    if (NPoints->n_points > 1 || !(ltype & GV_LINES)) {
		new_line = Vect_rewrite_line(Map, line, ltype, NPoints, Cats);
		if (List_lines)
		    G_ilist_add(List_lines, new_line);
	    }
	    else {
		Vect_delete_line(Map, line);
	    }
	    if (Err) {
		Vect_write_line(Err, ltype, Points, Cats);
	    }
	}
    }				/* for each line */
    G_percent(line_idx, nlines, 2); /* finish it */

    Vect_destroy_line_struct(Points);
    Vect_destroy_line_struct(NPoints);
    Vect_destroy_cats_struct(Cats);
    G_free(XPnts);
    G_free(Index);
    G_free(New);
    RTreeDestroyTree(RTree);
    if (rtreefd >= 0)
	close(rtreefd);

    G_free(rect.boundary);

    G_verbose_message(_("Snapped vertices: %d"), nsnapped);
    G_verbose_message(_("New vertices: %d"), ncreated);
}


/*!
   \brief Snap lines in vector map to existing vertex in threshold.
  
   For details see Vect_snap_lines_list()
  
   \param[in] Map input map where vertices will be snapped
   \param[in] type type of lines to snap
   \param[in] thresh threshold in which snap vertices
   \param[out] Err vector map where lines representing snap are written or NULL
  
   \return void
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
	 * use G_ilist_add() instead of Vect_list_append() */
	G_ilist_add(List, line);
    }

    Vect_snap_lines_list(Map, List, thresh, Err);

    Vect_destroy_list(List);

    return;
}
