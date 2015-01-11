
/****************************************************************
 *
 * MODULE:     v.generalize
 *
 * AUTHOR(S):  Daniel Bundala, Markus Metz
 *
 * PURPOSE:    miscellaneous functions of v.generalize
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

#include <stdlib.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>
#include "misc.h"

int type_mask(struct Option *type_opt)
{
    int res = 0;
    int i;

    for (i = 0; type_opt->answers[i]; i++)
	switch (type_opt->answers[i][0]) {
	case 'l':
	    res |= GV_LINE;
	    break;
	case 'b':
	    res |= GV_BOUNDARY;
	    break;
	case 'a':
	    res |= GV_AREA;
	}

    return res;
}

int get_furthest(struct line_pnts *Points, int a, int b, int with_z,
		 double *dist)
{
    int index = a;
    double d = 0;

    int i;
    double x0 = Points->x[a];
    double x1 = Points->x[b];
    double y0 = Points->y[a];
    double y1 = Points->y[b];
    double z0 = Points->z[a];
    double z1 = Points->z[b];

    double px, py, pz, pdist, di;
    int status;

    for (i = a + 1; i < b; i++) {
	di = dig_distance2_point_to_line(Points->x[i], Points->y[i],
					 Points->z[i], x0, y0, z0, x1, y1, z1,
					 with_z, &px, &py, &pz, &pdist,
					 &status);
	if (di > d) {
	    d = di;
	    index = i;
	}
    }
    *dist = d;
    return index;
}

/* TODO: The collection of categories is horrible in current version! 
 * Everything repeats many times. We need some data structure
 * implementing set! */
int copy_tables_by_cats(struct Map_info *In, struct Map_info *Out)
{
    /* this is the (mostly) code from v.extract, it should be moved to 
     * some vector library (probably) */

    int nlines, line, nfields;
    int ttype, ntabs = 0;
    struct field_info *IFi, *OFi;
    struct line_cats *Cats;
    int **ocats, *nocats, *fields;
    int i;

    /* Collect list of output cats */
    Cats = Vect_new_cats_struct();
    nfields = Vect_cidx_get_num_fields(In);
    ocats = (int **)G_malloc(nfields * sizeof(int *));
    nocats = (int *)G_malloc(nfields * sizeof(int));
    fields = (int *)G_malloc(nfields * sizeof(int));
    for (i = 0; i < nfields; i++) {
	nocats[i] = 0;
	ocats[i] =
	    (int *)G_malloc(Vect_cidx_get_num_cats_by_index(In, i) *
			    sizeof(int));
	fields[i] = Vect_cidx_get_field_number(In, i);
    }
    nlines = Vect_get_num_lines(Out);
    for (line = 1; line <= nlines; line++) {
	Vect_read_line(Out, NULL, Cats, line);
	for (i = 0; i < Cats->n_cats; i++) {
	    int f = 0, j;

	    for (j = 0; j < nfields; j++) {	/* find field */
		if (fields[j] == Cats->field[i]) {
		    f = j;
		    break;
		}
	    }
	    ocats[f][nocats[f]] = Cats->cat[i];
	    nocats[f]++;
	}
    }

    /* Copy tables */
    G_message(_("Writing attributes..."));

    /* Number of output tabs */
    for (i = 0; i < Vect_get_num_dblinks(In); i++) {
	int j, f = -1;

	IFi = Vect_get_dblink(In, i);

	for (j = 0; j < nfields; j++) {	/* find field */
	    if (fields[j] == IFi->number) {
		f = j;
		break;
	    }
	}
	if (f >= 0 && nocats[f] > 0)
	    ntabs++;
    }

    if (ntabs > 1)
	ttype = GV_MTABLE;
    else
	ttype = GV_1TABLE;

    for (i = 0; i < nfields; i++) {
	int ret;

	if (fields[i] == 0)
	    continue;
	if (nocats[i] == 0)
	    continue;
	/*        if ( fields[i] == field && new_cat != -1 ) continue; */

	G_message(_("Layer %d"), fields[i]);
	/* Make a list of categories */
	IFi = Vect_get_field(In, fields[i]);
	if (!IFi) {		/* no table */
	    G_warning(_("Database connection not defined for layer %d"),
		      fields[i]);
	    continue;
	}

	OFi = Vect_default_field_info(Out, IFi->number, IFi->name, ttype);

	ret = db_copy_table_by_ints(IFi->driver, IFi->database, IFi->table,
				    OFi->driver, Vect_subst_var(OFi->database,
								Out),
				    OFi->table, IFi->key, ocats[i],
				    nocats[i]);

	if (ret == DB_FAILED) {
	    G_warning(_("Unable to copy table <%s>"), IFi->table);
	}
	else {
	    Vect_map_add_dblink(Out, OFi->number, OFi->name, OFi->table,
				IFi->key, OFi->database, OFi->driver);
	}
    }

    for (i = 0; i < nfields; i++)
	G_free(ocats[i]);
    G_free(ocats);
    G_free(nocats);
    G_free(fields);
    return 1;
}

/* check topology corruption by boundary modification
 * return 0 on corruption, 1 if modification is ok */
int check_topo(struct Map_info *Out, int line, struct line_pnts *APoints,
               struct line_pnts *Points, struct line_cats *Cats)
{
    int i, j, intersect, newline, left_old, right_old,
	left_new, right_new;
    struct bound_box box, abox;
    struct line_pnts **AXLines, **BXLines;
    int naxlines, nbxlines;
    static struct line_pnts *BPoints = NULL;
    static struct boxlist *List = NULL;
    static struct line_pnts *BPoints2 = NULL;
    static struct ilist *BList = NULL;
    int area, isle, centr;
    int node, node_n_lines;
    float angle1, angle2;

    /* order of tests:
     * first: fast tests
     * last: tests that can only be done after writing out the new line  */

    if (!BPoints) {
	BPoints = Vect_new_line_struct();
	BPoints2 = Vect_new_line_struct();
	List = Vect_new_boxlist(1);
	BList = Vect_new_list();
    }

    if (APoints->n_points == Points->n_points) {
	int same = 1;
	
	for (i = 0; i < APoints->n_points; i++) {
	    if (APoints->x[i] != Points->x[i] || APoints->y[i] != Points->y[i]) {
		same = 0;
		break;
	    }
	}
	if (same)
	    return 1;
    }

    /* test node angles
     * same like dig_build_area_with_line() */
    /* line start */
    angle1 = dig_calc_begin_angle(Points, 0);
    if (angle1 == -9)
	return 0;

    node = dig_find_node(&(Out->plus), Points->x[0], Points->y[0], Points->z[0]);
    if (node) {
	/* if another line exists with the same angle at this node,
	 * an area can not be constructed -> error */
	node_n_lines = Vect_get_node_n_lines(Out, node);
	for (i = 0; i < node_n_lines; i++) {

	    if (abs(Vect_get_node_line(Out, node, i)) == line)
		continue;

	    if (angle1 == Vect_get_node_line_angle(Out, node, i))
		return 0;
	}
    }
    
    /* line end */
    angle2 = dig_calc_end_angle(Points, 0);
    if (angle2 == -9)
	return 0;

    i = Points->n_points - 1;
    if (angle1 == angle2 && 
        Points->x[0] == Points->x[i] && Points->y[0] == Points->y[i]) {
	/* same angle, same start and end coordinates,
	 * an area can not be constructed -> error */
	return 0;
    }

    node = dig_find_node(&(Out->plus), Points->x[i], Points->y[i], Points->z[i]);
    if (node) {
	/* if another line exists with the same angle at this node,
	 * an area can not be constructed -> error */
	node_n_lines = Vect_get_node_n_lines(Out, node);
	for (i = 0; i < node_n_lines; i++) {

	    if (abs(Vect_get_node_line(Out, node, i)) == line)
		continue;

	    if (angle2 == Vect_get_node_line_angle(Out, node, i))
		return 0;
	}
    }

    Vect_line_box(Points, &box);

    /* Check the modified boundary for self-intersection */
    AXLines = BXLines = NULL;
    Vect_line_intersection2(Points, NULL, &box, &box, &AXLines, &BXLines,
			   &naxlines, &nbxlines, 0);
    /* Free */
    if (naxlines > 0) {
	for (j = 0; j < naxlines; j++) {
	    Vect_destroy_line_struct(AXLines[j]);
	}
    }
    if (AXLines)
	G_free(AXLines);
    if (naxlines > 0)
	return 0;

    /* Check intersection of the modified boundary with other boundaries */
    Vect_select_lines_by_box(Out, &box, GV_BOUNDARY, List);

    intersect = 0;
    for (i = 0; i < List->n_values; i++) {
	int bline;

	bline = List->id[i];
	if (bline == line)
	    continue;

	Vect_read_line(Out, BPoints, NULL, bline);

	/* Vect_line_intersection is quite slow, hopefully not so bad because only few 
	 * intersections should be found if any */

	AXLines = BXLines = NULL;
	Vect_line_intersection2(Points, BPoints, &box, &List->box[i],
	                        &AXLines, &BXLines,
			        &naxlines, &nbxlines, 0);

	G_debug(4,
		"bline = %d intersect = %d naxlines = %d nbxlines = %d",
		bline, intersect, naxlines, nbxlines);

	/* Free */
	if (naxlines > 0) {
	    for (j = 0; j < naxlines; j++) {
		Vect_destroy_line_struct(AXLines[j]);
	    }
	}
	if (AXLines)
	    G_free(AXLines);
	if (nbxlines > 0) {
	    for (j = 0; j < nbxlines; j++) {
		Vect_destroy_line_struct(BXLines[j]);
	    }
	}
	if (BXLines)
	    G_free(BXLines);

	if (naxlines > 1 || nbxlines > 1) {
	    intersect = 1;
	    break;
	}
    }
    
    /* modified boundary intersects another boundary */
    if (intersect)
	return 0;

    /* the point-in-poly tests are needed to avoid some cases up duplicate centroids */
    Vect_get_line_areas(Out, line, &left_old, &right_old);

    Vect_line_box(APoints, &abox);

    /* centroid on the left side */
    isle = centr = 0;
    area = left_old;
    if (area < 0) {
	isle = -area;
	area = Vect_get_isle_area(Out, isle);
    }
    if (area > 0)
	centr = Vect_get_area_centroid(Out, area);
    if (centr > 0) {
	int ret;
	double cx, cy, cz;

	Vect_read_line(Out, BPoints, NULL, centr);
	cx = BPoints->x[0];
	cy = BPoints->y[0];
	cz = BPoints->z[0];
	
	if (1 || Vect_point_in_box(cx, cy, cz, &box) ||
	    Vect_point_in_box(cx, cy, cz, &abox)) {

	    if (isle)
		Vect_get_isle_boundaries(Out, isle, BList);
	    else
		Vect_get_area_boundaries(Out, area, BList);

	    Vect_reset_line(BPoints2);
	    for (i = 0; i < BList->n_values; i++) {
		int bline = BList->value[i];
		int dir = bline > 0 ? GV_FORWARD : GV_BACKWARD;

		if (abs(bline) != line) {
		    Vect_read_line(Out, BPoints, NULL, abs(bline));
		    Vect_append_points(BPoints2, BPoints, dir);
		}
		else
		    Vect_append_points(BPoints2, Points, dir);

		BPoints2->n_points--;    /* skip last point, avoids duplicates */
	    }
	    BPoints2->n_points++;        /* close polygon */

	    ret = Vect_point_in_poly(cx, cy, BPoints2);
	    /* see Vect_point_in_area() */
	    if (!isle) {
		/* area: centroid must be inside */
		if (ret == 0)
		    return 0;
	    }
	    else {
		/* isle: centroid must be outside */
		if (ret > 0)
		    return 0;
	    }
	}
    }
    left_old = centr;

    /* centroid on the right side */
    isle = centr = 0;
    area = right_old;
    if (area < 0) {
	isle = -area;
	area = Vect_get_isle_area(Out, isle);
    }
    if (area > 0)
	centr = Vect_get_area_centroid(Out, area);
    if (centr > 0) {
	int ret;
	double cx, cy, cz;

	Vect_read_line(Out, BPoints, NULL, centr);
	cx = BPoints->x[0];
	cy = BPoints->y[0];
	cz = BPoints->z[0];
	
	if (1 || Vect_point_in_box(cx, cy, cz, &box) ||
	    Vect_point_in_box(cx, cy, cz, &abox)) {

	    if (isle)
		Vect_get_isle_boundaries(Out, isle, BList);
	    else
		Vect_get_area_boundaries(Out, area, BList);

	    Vect_reset_line(BPoints2);
	    for (i = 0; i < BList->n_values; i++) {
		int bline = BList->value[i];
		int dir = bline > 0 ? GV_FORWARD : GV_BACKWARD;

		if (abs(bline) != line) {
		    Vect_read_line(Out, BPoints, NULL, abs(bline));
		    Vect_append_points(BPoints2, BPoints, dir);
		}
		else
		    Vect_append_points(BPoints2, Points, dir);

		BPoints2->n_points--;    /* skip last point, avoids duplicates */
	    }
	    BPoints2->n_points++;        /* close polygon */

	    ret = Vect_point_in_poly(cx, cy, BPoints2);
	    /* see Vect_point_in_area() */
	    if (!isle) {
		/* area: centroid must be inside */
		if (ret == 0)
		    return 0;
	    }
	    else {
		/* isle: centroid must be outside */
		if (ret > 0)
		    return 0;
	    }
	}
    }
    right_old = centr;

    /* OK, rewrite modified boundary */
    newline = Vect_rewrite_line(Out, line, GV_BOUNDARY, Points, Cats);

    /* Check position of centroids */
    Vect_get_line_areas(Out, newline, &left_new, &right_new);
    if (left_new < 0)
	left_new = Vect_get_isle_area(Out, abs(left_new));
    if (left_new > 0)
	left_new = Vect_get_area_centroid(Out, left_new);
    if (right_new < 0)
	right_new = Vect_get_isle_area(Out, abs(right_new));
    if (right_new > 0)
	right_new = Vect_get_area_centroid(Out, right_new);

    if (left_new != left_old || right_new != right_old) {
	G_debug(3,
		"The modified boundary changes attachment of centroid -> not modified");

	G_debug(1, "Left centroid old: %d, new: %d", left_old, left_new);
	G_debug(1, "Right centroid old: %d, new: %d", right_old, right_new);
	Vect_get_line_areas(Out, newline, &left_new, &right_new);
	G_debug(1, "New areas left: %d, right: %d", left_new, right_new);

	Vect_rewrite_line(Out, newline, GV_BOUNDARY, APoints, Cats);
	return 0;
    }
    
    return 1;
}
