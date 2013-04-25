/*!
   \file lib/vector/Vlib/remove_duplicates.c

   \brief Vector library - clean geometry (remove duplicates)

   Higher level functions for reading/writing/manipulating vectors.

   (C) 2001-2009 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2).  Read the file COPYING that comes with GRASS for details.

   \author Radim Blazek
 */

#include <stdlib.h>
#include <grass/vector.h>
#include <grass/glocale.h>

static int cmp_int(const void *a, const void *b)
{
    return (*(int *)a - *(int *)b);
}

static int boxlist_add_sorted(struct boxlist *list, int id)
{
    int i;

    if (list->n_values > 0) {
	if (bsearch(&id, list->id, list->n_values, sizeof(int), cmp_int))
	    return 0;
    }

    if (list->n_values == list->alloc_values) {
	size_t size = (list->n_values + 100) * sizeof(int);

	list->id = (int *)G_realloc((void *)list->id, size);
	list->alloc_values = list->n_values + 100;
    }
    
    i = 0;
    if (list->n_values > 0) {
	for (i = list->n_values; i > 0; i--) {
	    if (list->id[i - 1] < id)
		break;
	    list->id[i] = list->id[i - 1];
	}
    }
    list->id[i] = id;
    list->n_values++;
    
    return 1;
}

/*!
   \brief Remove duplicate features from vector map.

   Remove duplicate lines of given types from vector map. Duplicate
   lines may be optionally written to error map. Input map must be
   opened on level 2 for update. Categories are merged.
   GV_BUILD_BASE is sufficient.

   \param[in,out] Map vector map where duplicate lines will be deleted
   \param type type of line to be delete
   \param[out] Err vector map where duplicate lines will be written or NULL

   \return void
 */
void Vect_remove_duplicates(struct Map_info *Map, int type, struct Map_info *Err)
{
    struct line_pnts *APoints, *BPoints;
    struct line_cats *ACats, *BCats;
    int i, c, atype, btype, aline, bline;
    int nlines, nacats_orig, npoints;
    int na1, na2, nb1, nb2, nodelines, nline;
    struct bound_box ABox;
    struct boxlist *List;
    int ndupl, is_dupl;


    APoints = Vect_new_line_struct();
    BPoints = Vect_new_line_struct();
    ACats = Vect_new_cats_struct();
    BCats = Vect_new_cats_struct();
    List = Vect_new_boxlist(0);

    nlines = Vect_get_num_lines(Map);

    G_debug(1, "nlines =  %d", nlines);
    /* Go through all lines in vector, for each line select lines which
     * overlap with the first vertex of this line and check if a 
     * selected line is identical. If yes, remove the selected line.
     * If the line vertices are identical with those of any other line, 
     * merge categories and rewrite the current line.
     */

    ndupl = 0;

    for (aline = 1; aline <= nlines; aline++) {
	G_percent(aline, nlines, 1);
	if (!Vect_line_alive(Map, aline))
	    continue;

	atype = Vect_read_line(Map, APoints, ACats, aline);
	if (!(atype & type))
	    continue;
	
	npoints = APoints->n_points;
	Vect_line_prune(APoints);
	
	if (npoints != APoints->n_points) {
	    G_debug(3, "Line %d pruned, %d vertices removed", aline, npoints - APoints->n_points);
	    Vect_rewrite_line(Map, aline, atype, APoints, ACats);
	    nlines = Vect_get_num_lines(Map);
	    continue;
	}

	na1 = na2 = -1;
	if (atype & GV_LINES) {
	    /* faster than Vect_select_lines_by_box() */
	    Vect_reset_boxlist(List);
	    Vect_get_line_nodes(Map, aline, &na1, &na2);
	    nodelines = Vect_get_node_n_lines(Map, na1);

	    for (i = 0; i < nodelines; i++) {
		nline = abs(Vect_get_node_line(Map, na1, i));
		
		if (nline == aline)
		    continue;
		if (Vect_get_line_type(Map, nline) != atype)
		    continue;
		
		boxlist_add_sorted(List, nline);
	    }
	}
	else {
	    /* select potential duplicates */
	    ABox.E = ABox.W = APoints->x[0];
	    ABox.N = ABox.S = APoints->y[0];
	    ABox.T = ABox.B = APoints->z[0];
	    Vect_select_lines_by_box(Map, &ABox, atype, List);
	    G_debug(3, "  %d lines selected by box", List->n_values);
	}
	
	is_dupl = 0;

	for (i = 0; i < List->n_values; i++) {
	    bline = List->id[i];
	    G_debug(3, "  j = %d bline = %d", i, bline);

	    /* compare aline and bline only once */
	    if (aline <= bline)
		continue;

	    nb1 = nb2 = -1;

	    if (atype & GV_LINES) {
		Vect_get_line_nodes(Map, bline, &nb1, &nb2);
		if ((na1 == nb1 && na2 != nb2) ||
		    (na1 == nb2 && na2 != nb1))
		    continue;
	    }

	    btype = Vect_read_line(Map, BPoints, BCats, bline);
	    Vect_line_prune(BPoints);

	    /* check for duplicate */
	    if (!Vect_line_check_duplicate(APoints, BPoints, Vect_is_3d(Map)))
		continue;

	    /* bline is identical to aline */
	    if (!is_dupl) {
		if (Err) {
		    Vect_write_line(Err, atype, APoints, ACats);
		}
		is_dupl = 1;
	    }
	    Vect_delete_line(Map, bline);

	    /* merge categories */
	    nacats_orig = ACats->n_cats;

	    for (c = 0; c < BCats->n_cats; c++)
		Vect_cat_set(ACats, BCats->field[c], BCats->cat[c]);

	    if (ACats->n_cats > nacats_orig) {
		G_debug(4, "cats merged: n_cats %d -> %d", nacats_orig,
			ACats->n_cats);
	    }

	    ndupl++;
	}
	if (is_dupl) {
	    Vect_rewrite_line(Map, aline, atype, APoints, ACats);
	    nlines = Vect_get_num_lines(Map);
	    G_debug(3, "nlines =  %d\n", nlines);
	}
    }
    G_verbose_message("Removed duplicates: %d", ndupl);
}

/*!
   \brief Check for duplicate lines
   
   Note that lines must be pruned with Vect_line_prune() before passed 
   to Vect_line_check_duplicate()

   \param APoints first line geometry
   \param BPoints second line geometry

   \return 1 duplicate
   \return 0 not duplicate
 */
int Vect_line_check_duplicate(const struct line_pnts *APoints,
			      const struct line_pnts *BPoints, int with_z)
{
    int k;
    int npoints;
    int forw, backw;

    if (APoints->n_points != BPoints->n_points)
	return 0;

    npoints = APoints->n_points;

    /* Forward */
    forw = 1;
    for (k = 0; k < APoints->n_points; k++) {
	if (APoints->x[k] != BPoints->x[k] ||
	    APoints->y[k] != BPoints->y[k] ||
	    (with_z && APoints->z[k] != BPoints->z[k])) {
	    forw = 0;
	    break;
	}
    }

    /* Backward */
    backw = 1;
    for (k = 0; k < APoints->n_points; k++) {
	if (APoints->x[k] != BPoints->x[npoints - k - 1] ||
	    APoints->y[k] != BPoints->y[npoints - k - 1] ||
	    (with_z && APoints->z[k] != BPoints->z[npoints - k - 1])) {
	    backw = 0;
	    break;
	}
    }

    if (!forw && !backw)
	return 0;

    return 1;
}
