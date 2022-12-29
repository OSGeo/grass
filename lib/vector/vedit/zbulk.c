/*!
  \file lib/vector/vedit/zbulk.c

  \brief Vedit library - Bulk labeling (automated labeling of vector
  features)
  
  (C) 2007-2008 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2).  Read the file COPYING that comes with GRASS for details.
  
  \author Martin Landa <landa.martin gmail.com>
*/

#include <grass/dbmi.h>
#include <grass/vedit.h>

/*!
  \brief Lines z-bulk labeling
  
  Automated labeling (z coordinate assignment) of vector lines (iso-lines).
  
  \param Map pointer to Map_info
  \param List list of selected lines
  \param point_start_end staring and ending point
  \param start starting value
  \param step step value
  
  \return number of modified features
  \return -1 on error
*/
int Vedit_bulk_labeling(struct Map_info *Map, struct ilist *List,
			double x1, double y1, double x2, double y2,
			double start, double step)
{
    int i, cv_i, p_i;
    int line, type, temp_line;
    int nlines_modified;
    double value, dist;

    struct line_cats *Cats;
    struct line_pnts *Points, *Points_se;	/* start - end */
    struct bound_box box, box_se;

    /* for intersection */
    struct line_pnts **Points_a, **Points_b;
    int nlines_a, nlines_b;

    dbCatValArray cv;		/* line_id / dist */

    nlines_modified = 0;

    value = start;

    Points = Vect_new_line_struct();
    Points_se = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    db_CatValArray_alloc(&cv, List->n_values);
    cv.ctype = DB_C_TYPE_DOUBLE;
    cv.n_values = 0;

    Vect_append_point(Points_se, x1, y1, -PORT_DOUBLE_MAX);
    Vect_append_point(Points_se, x2, y2, PORT_DOUBLE_MAX);

    /* write temporaly line */
    temp_line = Vect_write_line(Map, GV_LINE, Points_se, Cats);
    if (temp_line < 0) {
	return -1;
    }
    
    Vect_line_box(Points_se, &box_se);

    /* determine order of lines */
    cv_i = 0;
    for (i = 0; i < List->n_values; i++) {
	line = List->value[i];

	if (!Vect_line_alive(Map, line))
	    continue;

	type = Vect_read_line(Map, Points, NULL, line);

	if (!(type & GV_LINE))
	    continue;

	Vect_line_box(Points, &box);
	if (Vect_line_check_intersection(Points_se, Points, WITH_Z)) {
	    Vect_line_intersection(Points_se, Points, &box_se, &box,
				   &Points_a, &Points_b, &nlines_a, &nlines_b,
				   WITHOUT_Z);

	    if (nlines_a < 2 || nlines_b < 1)	/* should not happen */
		continue;

	    /* calculate distance start point -> point of intersection */
	    for (p_i = 0; p_i < Points_a[0]->n_points; p_i++) {
		Points_a[0]->z[p_i] = 0;
	    }
	    dist = Vect_line_length(Points_a[0]);	/* always first line in array? */

	    cv.value[cv_i].cat = line;
	    cv.value[cv_i++].val.d = dist;
	    cv.n_values++;
	}
    }

    /* sort array by distance */
    db_CatValArray_sort_by_value(&cv);

    /* z bulk-labeling */
    for (cv_i = 0; cv_i < cv.n_values; cv_i++) {
	line = cv.value[cv_i].cat;
	type = Vect_read_line(Map, Points, Cats, line);

	for (p_i = 0; p_i < Points->n_points; p_i++) {
	    Points->z[p_i] = value;
	}

	if (Vect_rewrite_line(Map, line, type, Points, Cats) < 0) {
	    return -1;
	}
	nlines_modified++;

	value += step;
    }

    if (Vect_delete_line(Map, temp_line) < 0) {
	return -1;
    }

    db_CatValArray_free(&cv);
    Vect_destroy_line_struct(Points);
    Vect_destroy_line_struct(Points_se);
    Vect_destroy_cats_struct(Cats);

    return nlines_modified;
}
