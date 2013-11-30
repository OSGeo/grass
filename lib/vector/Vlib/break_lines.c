/*!
 * \file lib/vector/Vlib/break_lines.c
 *
 * \brief Vector library - Clean vector map (break lines)
 *
 * (C) 2001-2009 by the GRASS Development Team
 *
 * This program is free software under the 
 * GNU General Public License (>=v2). 
 * Read the file COPYING that comes with GRASS
 * for details.
 *
 * \author Radim Blazek
 */

#include <stdlib.h>
#include <grass/vector.h>
#include <grass/glocale.h>

static int break_lines(struct Map_info *, struct ilist *, struct ilist *,
		       int, struct Map_info *, int);


/*!
   \brief Break lines in vector map at each intersection.

   For details see Vect_break_lines_list().

   \param Map input vector map 
   \param type feature type
   \param[out] Err vector map where points at intersections will be written or NULL
*/
void Vect_break_lines(struct Map_info *Map, int type, struct Map_info *Err)
{
    break_lines(Map, NULL, NULL, type, Err, 0);

    return;
}

/*!
   \brief Break selected lines in vector map at each intersection.

   Breaks selected lines specified by type in vector map. Points at
   intersections may be optionally written to error map. Input vector map
   must be opened on level 2 for update at least on GV_BUILD_BASE.

   The function also breaks lines forming collapsed loop, for example
   0,0;1,0;0,0 is broken at 1,0.

   If reference lines are given (<i>List_ref</i>) break only lines
   which intersect reference lines.

   \param Map input vector map 
   \param List_break list of lines (NULL for all lines in vector map)
   \param List_ref list of reference lines or NULL
   \param type feature type
   \param[out] Err vector map where points at intersections will be written or NULL

   \return number of intersections
*/

int Vect_break_lines_list(struct Map_info *Map, struct ilist *List_break,
                          struct ilist *List_ref, int type,
                          struct Map_info *Err)
{
    return break_lines(Map, List_break, List_ref, type, Err, 0);
}

/*!
   \brief Check for and count intersecting lines, do not break.

   For details see Vect_check_line_breaks_list().

   \param Map input vector map 
   \param type feature type
   \param[out] Err vector map where points at intersections will be written or NULL

   \return number for intersections
*/
int Vect_check_line_breaks(struct Map_info *Map, int type, struct Map_info *Err)
{
    return break_lines(Map, NULL, NULL, type, Err, 1);
}

/*!
   \brief Check for and count intersecting lines, do not break.

   If <i>List_break</i> is given, only lines in the list are checked for
   intersections.

   If reference lines are given (<i>List_ref</i>) break only lines
   which intersect reference lines.

   \param Map input vector map 
   \param List_break list of lines (NULL for all lines in vector map)
   \param List_ref list of reference lines or NULL
   \param type feature type
   \param[out] Err vector map where points at intersections will be written or NULL

   \return number of intersections
*/
int Vect_check_line_breaks_list(struct Map_info *Map, struct ilist *List_break,
		      struct ilist *List_ref, int type,
		      struct Map_info *Err)
{
    return break_lines(Map, List_break, List_ref, type, Err, 1);
}

static int cmp(const void *a, const void *b)
{
    int ai = *(int *)a;
    int bi = *(int *)b;
    
    return (ai - bi);
}

static void sort_ilist(struct ilist *List)
{
    int i, j, is_sorted = 1;
    
    for (i = 1; i < List->n_values; i++) {
	if (List->value[i - 1] > List->value[i]) {
	    is_sorted = 0;
	    break;
	}
    }
    
    if (!is_sorted)
	qsort(List->value, List->n_values, sizeof(int), cmp);
    
    if (List->n_values > 1) {
	j = 1;
	for (i = 1; i < List->n_values; i++) {
	    if (List->value[j - 1] != List->value[i]) {
		List->value[j] = List->value[i];
		j++;
	    }
	}
	List->n_values = j;
    }
}

int break_lines(struct Map_info *Map, struct ilist *List_break,
                struct ilist *List_ref, int type,
                struct Map_info *Err, int check)
{
    struct line_pnts *APoints, *BPoints, *Points;
    struct line_pnts **AXLines, **BXLines;
    struct line_cats *ACats, *BCats, *Cats;
    int i, j, k, l, ret, atype, btype, aline, bline, found, iline;
    int nlines, nlines_org;
    int naxlines, nbxlines, nx;
    double *xx = NULL, *yx = NULL, *zx = NULL;
    struct bound_box ABox, *BBox;
    struct boxlist *List;
    int nbreaks;
    int touch1_n = 0, touch1_s = 0, touch1_e = 0, touch1_w = 0;	/* other vertices except node1 touching box */
    int touch2_n = 0, touch2_s = 0, touch2_e = 0, touch2_w = 0;	/* other vertices except node2 touching box */
    int is3d;
    int node, anode1, anode2, bnode1, bnode2;
    double nodex, nodey;
    int a_is_ref, b_is_ref, break_a, break_b;

    type &= GV_LINES;
    if (!type)
	return 0;

    APoints = Vect_new_line_struct();
    BPoints = Vect_new_line_struct();
    Points = Vect_new_line_struct();
    ACats = Vect_new_cats_struct();
    BCats = Vect_new_cats_struct();
    Cats = Vect_new_cats_struct();
    List = Vect_new_boxlist(1);

    is3d = Vect_is_3d(Map);

    if (List_ref)
	sort_ilist(List_ref);
    if (List_break)
	sort_ilist(List_break);

    if (List_ref) {
	nlines = List_ref->n_values;
	nlines_org = List_ref->value[List_ref->n_values - 1];
    }
    else if (List_break) {
	nlines = List_break->n_values;
	nlines_org = List_break->value[List_break->n_values - 1];
    }
    else {
	nlines = Vect_get_num_lines(Map);
	nlines_org = nlines;
    }
    G_debug(3, "nlines =  %d", nlines);
    

    /* TODO:
     * 1. It seems that lines/boundaries are not broken at intersections
     *    with points/centroids. Check if true, if yes, skip GV_POINTS
     * 2. list of lines to break and list of reference lines
     *    aline: reference line, if List_ref == NULL, use all
     *           break aline only if it is in the list of lines to break
     *    bline: line to break, if List_break == NULL, break all
     */

    /* To find intersection of two lines (Vect_line_intersection) is quite slow.
     * Fortunately usual lines/boundaries in GIS often forms a network where lines
     * are connected by end points, and touch by MBR. This function checks and occasionaly
     * skips such cases. This is currently done for 2D only
     */

    /* Go through all lines in vector, for each select lines which overlap MBR of
     * this line exclude those connected by one endpoint (see above)
     * and try to intersect, if lines intersect write new lines at the end of 
     * the file, and process next line (remaining lines overlapping box are skipped)
     */
    nbreaks = 0;
    
    for (iline = 0; iline < nlines; iline++) {
	G_percent(iline, nlines, 1);
	
	/* aline: reference line */
	if (List_ref) {
	    aline = List_ref->value[iline];
	}
	else if (List_break) {
	    aline = List_break->value[iline];
	}
	else {
	    aline = iline + 1;
	}

	G_debug(3, "aline =  %d", aline);
	if (!Vect_line_alive(Map, aline))
	    continue;

	a_is_ref = 0;
	break_a = 1;
	if (List_ref) {
	    a_is_ref = 1;
	}

	if (List_break) {
	    break_a = 0;
	    if (bsearch(&aline, List_break->value, List_break->n_values, sizeof(int), cmp)) {
		break_a = 1;
	    }
	}

	atype = Vect_read_line(Map, APoints, ACats, aline);
	if (!(atype & type))
	    continue;

	Vect_line_prune(APoints);
	Vect_line_box(APoints, &ABox);

	/* Find which sides of the box are touched by intermediate (non-end) points of line */
	if (!is3d) {
	    touch1_n = touch1_s = touch1_e = touch1_w = 0;
	    for (j = 1; j < APoints->n_points; j++) {
		if (APoints->y[j] == ABox.N)
		    touch1_n = 1;
		if (APoints->y[j] == ABox.S)
		    touch1_s = 1;
		if (APoints->x[j] == ABox.E)
		    touch1_e = 1;
		if (APoints->x[j] == ABox.W)
		    touch1_w = 1;
	    }
	    G_debug(3, "touch1: n = %d s = %d e = %d w = %d", touch1_n,
		    touch1_s, touch1_e, touch1_w);
	    touch2_n = touch2_s = touch2_e = touch2_w = 0;
	    for (j = 0; j < APoints->n_points - 1; j++) {
		if (APoints->y[j] == ABox.N)
		    touch2_n = 1;
		if (APoints->y[j] == ABox.S)
		    touch2_s = 1;
		if (APoints->x[j] == ABox.E)
		    touch2_e = 1;
		if (APoints->x[j] == ABox.W)
		    touch2_w = 1;
	    }
	    G_debug(3, "touch2: n = %d s = %d e = %d w = %d", touch2_n,
		    touch2_s, touch2_e, touch2_w);
	}

	Vect_select_lines_by_box(Map, &ABox, type, List);
	G_debug(3, "  %d lines selected by box", List->n_values);

	for (j = -1; j < List->n_values; j++) {
	    
	    /* bline: line to break */

	    if (j == -1) {
		/* check first for self-intersections */
		if (aline <= nlines_org)
		    bline = aline;
		else
		    continue;
	    }
	    else {
		bline = List->id[j];
		if (bline == aline)
		    continue;
	    }

	    b_is_ref = 0;
	    break_b = 1;
	    if (List_ref && 
		bsearch(&bline, List_ref->value, List_ref->n_values, sizeof(int), cmp)) {
		b_is_ref = 1;
		/* reference bline will be broken when it is aline */
		break_b = 0;
	    }

	    if (List_break) {
		break_b = 0;
		if (bsearch(&bline, List_break->value, List_break->n_values, sizeof(int), cmp)) {
		    break_b = 1;
		}
	    }
	    
	    if (!break_a && !break_b)
		continue;

	    /* check intersection of aline with bline only once
	     * if possible */
	    if (break_a && break_b && 
		aline > bline && 
		(!List_ref || b_is_ref)) { 
		continue; 
	    } 

	    G_debug(3, "  j = %d bline = %d", j, bline);

	    btype = Vect_read_line(Map, BPoints, BCats, bline);
	    Vect_line_prune(BPoints);

	    if (j == -1)
		BBox = &ABox;
	    else
		BBox = &List->box[j];

	    /* Check if touch by end node only */
	    if (!is3d) {
		Vect_get_line_nodes(Map, aline, &anode1, &anode2);
		Vect_get_line_nodes(Map, bline, &bnode1, &bnode2);

		node = 0;
		if (anode1 == bnode1 || anode1 == bnode2)
		    node = anode1;
		else if (anode2 == bnode1 || anode2 == bnode2)
		    node = anode2;

		if (node) {
		    Vect_get_node_coor(Map, node, &nodex, &nodey, NULL);
		    if ((node == anode1 && nodey == ABox.N &&
		         !touch1_n && nodey == BBox->S) ||
		        (node == anode2 && nodey == ABox.N &&
			 !touch2_n && nodey == BBox->S) ||
			(node == anode1 && nodey == ABox.S &&
			 !touch1_s && nodey == BBox->N) ||
			(node == anode2 && nodey == ABox.S &&
			 !touch2_s && nodey == BBox->N) ||
			(node == anode1 && nodex == ABox.E &&
			 !touch1_e && nodex == BBox->W) ||
			(node == anode2 && nodex == ABox.E &&
			 !touch2_e && nodex == BBox->W) ||
			(node == anode1 && nodex == ABox.W &&
			 !touch1_w && nodex == BBox->E) ||
			(node == anode2 && nodex == ABox.W &&
			 !touch2_w && nodex == BBox->E)) {

			G_debug(3,
				"lines %d and %d touching by end nodes only -> no intersection",
				aline, bline);
			continue;
		    }
		}
	    }

	    AXLines = NULL;
	    BXLines = NULL;
	    Vect_line_intersection(APoints, BPoints, &ABox, BBox,
	                           &AXLines, &BXLines,
				   &naxlines, &nbxlines, 0);
	    G_debug(3, "  naxlines = %d nbxlines = %d", naxlines, nbxlines);

	    /* This part handles a special case when aline == bline, no other intersection was found
	     * and the line is forming collapsed loop, for example  0,0;1,0;0,0 should be broken at 1,0.
	     * ---> */
	    if (aline == bline && naxlines == 0 && nbxlines == 0 &&
		APoints->n_points >= 3 && break_a) {
		int centre;

		G_debug(3, "  Check collapsed loop");
		if (APoints->n_points % 2) {	/* odd number of vertices */
		    centre = APoints->n_points / 2;	/* index of centre */
		    if (APoints->x[centre - 1] == APoints->x[centre + 1] && APoints->y[centre - 1] == APoints->y[centre + 1] && APoints->z[centre - 1] == APoints->z[centre + 1]) {	/* -> break */
			AXLines =
			    (struct line_pnts **)G_malloc(2 *
							  sizeof(struct
								 line_pnts
								 *));
			AXLines[0] = Vect_new_line_struct();
			AXLines[1] = Vect_new_line_struct();

			for (i = 0; i <= centre; i++)
			    Vect_append_point(AXLines[0], APoints->x[i],
					      APoints->y[i], APoints->z[i]);

			for (i = centre; i < APoints->n_points; i++)
			    Vect_append_point(AXLines[1], APoints->x[i],
					      APoints->y[i], APoints->z[i]);

			naxlines = 2;
		    }
		}
	    }
	    /* <--- */

	    if (Err) {		/* array for intersections (more than needed */
		xx = (double *)G_malloc((naxlines + nbxlines) *
					sizeof(double));
		yx = (double *)G_malloc((naxlines + nbxlines) *
					sizeof(double));
		zx = (double *)G_malloc((naxlines + nbxlines) *
					sizeof(double));
	    }
	    nx = 0;		/* number of intersections to be written to Err */
	    if (naxlines > 0) {	/* intersection -> write out */

		G_debug(3, "  aline = %d,  bline = %d,  naxlines = %d",
		        aline, bline, naxlines);

		if (!check && break_a)
		    Vect_delete_line(Map, aline);
		for (k = 0; k < naxlines; k++) {
		    /* Write new line segments */
		    /* line may collapse, don't write zero length lines */
		    Vect_line_prune(AXLines[k]);
		    if ((atype & GV_POINTS) || AXLines[k]->n_points > 1) {
			if (!check && break_a) {
			    ret = Vect_write_line(Map, atype, AXLines[k],
			                          ACats);
			    G_debug(3, "Line %d written, npoints = %d", ret,
				    AXLines[k]->n_points);
			    if (List_ref && a_is_ref) {
				G_ilist_add(List_ref, ret);
			    }
			    if (List_break && break_a) {
				G_ilist_add(List_break, ret);
			    }
			}
		    }
		    else
			G_debug(3, "axline %d has zero length", k);

		    /* Write intersection points */
		    if (Err) {
			if (k > 0) {
			    xx[nx] = AXLines[k]->x[0];
			    yx[nx] = AXLines[k]->y[0];
			    zx[nx] = AXLines[k]->z[0];
			    nx++;
			}
		    }
		    Vect_destroy_line_struct(AXLines[k]);
		}
		nbreaks += naxlines - 1;
	    }
	    if (AXLines)
		G_free(AXLines);

	    if (nbxlines > 0) {
		if (aline != bline) {	/* Self intersection, do not write twice, TODO: is it OK? */

		    G_debug(3, "  aline = %d,  bline = %d,  nbxlines = %d",
			    aline, bline, nbxlines);

		    if (!check && break_b)
			Vect_delete_line(Map, bline);
		    for (k = 0; k < nbxlines; k++) {
			/* Write new line segments */
			/* line may collapse, don't write zero length lines */
			Vect_line_prune(BXLines[k]);
			if ((btype & GV_POINTS) || BXLines[k]->n_points > 1) {
			    if (!check && break_b) {
				ret =
				    Vect_write_line(Map, btype, BXLines[k],
						    BCats);
				G_debug(5, "Line %d written", ret);
				if (List_ref && b_is_ref) {
				    G_ilist_add(List_ref, ret);
				}
				if (List_break) {
				    G_ilist_add(List_break, ret);
				}
			    }
			}
			else
			    G_debug(3, "bxline %d has zero length", k);

			/* Write intersection points */
			if (Err) {
			    if (k > 0) {
				found = 0;
				for (l = 0; l < nx; l++) {
				    if (xx[l] == BXLines[k]->x[0] &&
					yx[l] == BXLines[k]->y[0] &&
					zx[l] == BXLines[k]->z[0]) {
					found = 1;
					break;
				    }
				}
				if (!found) {
				    xx[nx] = BXLines[k]->x[0];
				    yx[nx] = BXLines[k]->y[0];
				    zx[nx] = BXLines[k]->z[0];
				    nx++;
				}
			    }
			}
		    }
		    nbreaks += nbxlines - 1;
		}
		for (k = 0; k < nbxlines; k++)
		    Vect_destroy_line_struct(BXLines[k]);
	    }
	    if (BXLines)
		G_free(BXLines);
	    if (Err) {
		for (l = 0; l < nx; l++) {	/* Write out errors */
		    Vect_reset_line(Points);
		    Vect_append_point(Points, xx[l], yx[l], zx[l]);
		    ret = Vect_write_line(Err, GV_POINT, Points, Cats);
		}

		G_free(xx);
		G_free(yx);
		G_free(zx);
	    }
	    if (naxlines > 0 && !check && break_a) {
		G_debug(3, "aline was broken, use next one");
		break;		/* first line was broken and deleted -> take the next one */
	    }
	}

	if (List_ref) {
	    nlines = List_ref->n_values;
	}
	else if (List_break) {
	    nlines = List_break->n_values;
	}
	else {
	    nlines = Vect_get_num_lines(Map);
	}
	G_debug(3, "nlines =  %d", nlines);
    }				/* for each line */
    G_percent(nlines, nlines, 1); /* finish it */

    G_verbose_message(_("Intersections: %d"), nbreaks);

    Vect_destroy_line_struct(APoints);
    Vect_destroy_line_struct(BPoints);
    Vect_destroy_line_struct(Points);
    Vect_destroy_cats_struct(ACats);
    Vect_destroy_cats_struct(BCats);
    Vect_destroy_cats_struct(Cats);
    Vect_destroy_boxlist(List);

    return nbreaks;
}
