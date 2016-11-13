/*!
  \file lib/vector/vedit/merge.c
  
  \brief Vedit library - merge lines
  
  (C) 2006-2008 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2).  Read the file COPYING that comes with GRASS for details.
  
  \author Jachym Cepicky <jachym.cepicky gmail.com>
  \author Martin Landa <landa.martin gmail.com>
*/

#include <grass/vedit.h>

/*!
  \brief Merge two given lines a, b
  
  a : Points1/Cats1
  b : Points2/Cats2
  merged line : Points/Cats
   
  \param Points1,Cats1 first line
  \param Points2,Cats2 second line
  \param thresh threshold value
  \param[out] Points result line
  
  \return 1 on success
  \return 0 on error
*/
static int merge_lines(struct line_pnts *Points1, struct line_cats *Cats1,
		       struct line_pnts *Points2, struct line_cats *Cats2,
		       double thresh, struct line_pnts **Points);

/*!
  \brief Merge lines/boundaries
  
  At least two lines need to be given.
  
  \param Map pointer to Map_info
  \param List list of selected lines
  
   \return number of merged lines
   \return -1 on error
*/
int Vedit_merge_lines(struct Map_info *Map, struct ilist *List)
{
    struct ilist *List_in_box;

    struct line_pnts *Points1, *Points2, *Points;
    struct line_cats *Cats1, *Cats2;

    int line_i, i, j;
    int line, line1, type1, line2;
    int do_merge;

    /* number of lines (original, selected, merged) */
    int nlines, nlines_merged;

    nlines_merged = 0;

    if (List->n_values < 2) {
	return 0;
    }

    G_debug(1, "Vedit_merge_lines(): merging %d lines", List->n_values);

    Points1 = Vect_new_line_struct();
    Cats1 = Vect_new_cats_struct();
    Points2 = Vect_new_line_struct();
    Cats2 = Vect_new_cats_struct();
    Points = Vect_new_line_struct();

    List_in_box = Vect_new_list();

    nlines = Vect_get_num_lines(Map);
    
    /* merge lines */
    for (line_i = 0; line_i < List->n_values; line_i++) {
	line1 = List->value[line_i];

	if (!Vect_line_alive(Map, line1))
	    continue;

	type1 = Vect_read_line(Map, Points1, Cats1, line1);

	if (!(type1 & GV_LINES))
	    continue;

	/* remove duplicate points */
	Vect_line_prune(Points1);

	if (Points1->n_points < 2) {
	    G_debug(3, "Vedit_merge_lines(): skipping zero length line");
	    continue;
	}

	Vect_reset_line(Points);

	for (i = 0; i < Points1->n_points; i += Points1->n_points - 1) {
	    Vect_reset_list(List_in_box);

	    /* define searching region */
	    Vect_reset_line(Points2);
	    /*
	       Vect_append_point (Points2, Points1 -> x[i] - thresh,
	       Points1 -> y[i] + thresh, Points1 -> z[i]);
	       Vect_append_point (Points2, Points1 -> x[i] + thresh,
	       Points1 -> y[i] + thresh, Points1 -> z[i]);
	       Vect_append_point (Points2, Points1 -> x[i] + thresh,
	       Points1 -> y[i] - thresh, Points1 -> z[i]);
	       Vect_append_point (Points2, Points1 -> x[i] - thresh,
	       Points1 -> y[i] - thresh, Points1 -> z[i]);
	     */
	    Vect_append_point(Points2, Points1->x[i],
			      Points1->y[i], Points1->z[i]);

	    /* 
	     * merge lines only if two lines found in the region
	     * i.e. the current line and an adjacent line
	     */
	    /* NOTE
	     * - this merges two lines also if more than two lines are 
	     *   found in the region, but only two of these lines are 
	     *   in the List
	     * - this not only merges lines connected by end points 
	     *   but also any adjacent line with a mid point identical 
	     *   to one of the end points of the current line 
	     */
	    if (0 < Vect_find_line_list(Map, Points1->x[i], Points1->y[i],
	                                Points1->z[i], GV_LINES, 0, 0,
					NULL, List_in_box)) {
		do_merge = 1;
		line2 = -1;
		for (j = 0; do_merge && j < List_in_box->n_values; j++) {
		    if (List_in_box->value[j] == line1 ||
			!Vect_line_alive(Map, List_in_box->value[j]))
			continue;

		    if (Vect_val_in_list(List, List_in_box->value[j])) {
			
			Vect_read_line(Map, Points2, Cats2, List_in_box->value[j]);
			Vect_line_prune(Points2);
			if (Points2->n_points == 1) {
			    line2 = List_in_box->value[j];
			    do_merge = 1;
			    break;
			}
			if (line2 > 0) {
			    /* three lines found
			     * selected lines will be not merged
			     */
			    do_merge = 0;
			}
			else {
			    line2 = List_in_box->value[j];
			}
		    }
		}

		if (!do_merge || line2 < 0)
		    continue;

		Vect_read_line(Map, Points2, Cats2, line2);

		merge_lines(Points1, Cats1, Points2, Cats2, -1.0, &Points);	/* do not use threshold value */

		G_debug(3, "Vedit_merge_lines(): lines=%d,%d", line1, line2);

		if (Points->n_points > 0) {
		    if (Vect_delete_line(Map, line2) == -1) {
			return -1;
		    }

		    if (line2 <= nlines)
			nlines_merged++;
		}
	    }
	}			/* for each node */

	if (Points->n_points > 0) {
	    line = Vect_rewrite_line(Map, line1, type1, Points, Cats1);
	    if (line < 0) {
		return -1;
	    }

	    if (line1 <= nlines)
		nlines_merged++;

	    /* update number of lines */
	    G_ilist_add(List, line);
	}
    }				/* for each line */

    /* destroy structures */
    Vect_destroy_line_struct(Points1);
    Vect_destroy_line_struct(Points2);
    Vect_destroy_line_struct(Points);

    Vect_destroy_cats_struct(Cats1);
    Vect_destroy_cats_struct(Cats2);

    return nlines_merged;
}

static int merge_lines(struct line_pnts *Points1, struct line_cats *Cats1,
		       struct line_pnts *Points2, struct line_cats *Cats2,
		       double thresh, struct line_pnts **Points)
{
    struct line_pnts *ps = *Points;
    struct line_cats *cs = Cats1;

    int i, mindistidx;
    double mindist;

    /* find mininal distance and its index */
    mindist = Vedit_get_min_distance(Points1, Points2, 0,	/* TODO 3D */
				     &mindistidx);

    G_debug(3, "   merge line ? index: %d, mindist: %g, thresh: %g",
	    mindistidx, mindist, thresh);

    if (thresh > 0 && mindist > thresh) {
	return 0;
    }

    /* set index and other things */
    switch (mindistidx) {
	/* for each mindistidx create new line */
    case 0:
	Vect_append_points(ps, Points2, GV_BACKWARD);
	if (ps->n_points == Points2->n_points)
	    Vect_append_points(ps, Points1, GV_FORWARD);
	break;
    case 1:
	Vect_append_points(ps, Points2, GV_FORWARD);
	if (ps->n_points == Points2->n_points)
	    Vect_append_points(ps, Points1, GV_FORWARD);
	break;
    case 2:
	if (ps->n_points == 0)
	    Vect_append_points(ps, Points1, GV_FORWARD);
	Vect_append_points(ps, Points2, GV_FORWARD);
	break;
    case 3:
	if (ps->n_points == 0)
	    Vect_append_points(ps, Points1, GV_FORWARD);
	Vect_append_points(ps, Points2, GV_BACKWARD);
	break;
    default:
	break;
    }

    /* remove duplicate points */
    Vect_line_prune(ps);

    /* copy categories if needed */
    for (i = 0; i < Cats2->n_cats; i++) {
	Vect_cat_set(cs, Cats2->field[i], Cats2->cat[i]);
    }

    return 1;
}
