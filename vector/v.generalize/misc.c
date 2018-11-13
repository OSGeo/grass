
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

static int topo_debug = 0;

int set_topo_debug(void)
{
    if (getenv("GRASS_VECTOR_TOPO_DEBUG"))
	topo_debug = 1;

    return topo_debug;
}

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
	    res |= GV_BOUNDARY;
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

static int cmp(const void *a, const void *b)
{
    int ai = *(int *)a;
    int bi = *(int *)b;

    return (ai < bi ? -1 : (ai > bi));
}

/* check topology corruption by boundary modification
 * return 0 on corruption, 1 if modification is ok */
int check_topo(struct Map_info *Out, int line, struct line_pnts *APoints,
               struct line_pnts *Points, struct line_cats *Cats)
{
    int i, j, k, intersect, newline;
    struct bound_box box, abox, tbox;
    struct bound_box lbox, rbox, areabox;
    struct line_pnts **AXLines, **BXLines;
    int naxlines, nbxlines;
    static struct line_pnts *BPoints = NULL;
    static struct boxlist *List = NULL;
    static struct line_pnts *BPoints2 = NULL;
    static struct ilist *BList = NULL;
    int area, isle, centr;
    int left_o, left_n, right_o, right_n;
    int node, node_n_lines;
    float angle1, angle2;
    off_t offset;
    /* topology debugging */
    int area_l_o, area_r_o, area_l_n, area_r_n;
    int centr_l_o, centr_r_o, centr_l_n, centr_r_n;
    int *isles_l_o, nisles_l_o, *isles_l_n, nisles_l_n;
    int *isles_r_o, nisles_r_o, *isles_r_n, nisles_r_n;
    int found;

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

    i = APoints->n_points - 1;
    if (APoints->x[0] == APoints->x[i] && APoints->y[0] == APoints->y[i]) {
	i = Points->n_points - 1;
	if (Points->x[0] != Points->x[i] || Points->y[0] != Points->y[i]) {
	    /* input line forms a loop, output not */
	    return 0;
	}
    }

    /* test node angles
     * an area can be built only if there are no two lines with the same 
     * angle at the same node */
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
    if (Points->x[0] == Points->x[i] && Points->y[0] == Points->y[i]) {
	if (angle1 == angle2) {
	    /* same angle, same start and end coordinates,
	     * an area can not be constructed -> error */
	    return 0;
	}
    }
    else {
	node = dig_find_node(&(Out->plus), Points->x[i], Points->y[i], Points->z[i]);
    }

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

    /* test centroid attachment (point-in-poly) */
    Vect_get_line_areas(Out, line, &left_o, &right_o);

    Vect_line_box(APoints, &abox);

    /* centroid on the left side */
    isle = centr = 0;
    area = left_o;
    if (area < 0) {
	isle = -area;
	area = Vect_get_isle_area(Out, isle);
    }
    if (area > 0)
	centr = Vect_get_area_centroid(Out, area);
    centr_l_o = centr;
    area_l_o = area;
    lbox = box;

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
	    Vect_line_box(BPoints, &tbox);
	    Vect_box_extend(&lbox, &tbox);
	    Vect_append_points(BPoints2, BPoints, dir);
	}
	else
	    Vect_append_points(BPoints2, Points, dir);

	BPoints2->n_points--;    /* skip last point, avoids duplicates */
    }
    BPoints2->n_points++;        /* close polygon */

    if (centr > 0) {
	int ret;
	double cx, cy, cz;

	Vect_read_line(Out, BPoints, NULL, centr);
	cx = BPoints->x[0];
	cy = BPoints->y[0];
	cz = BPoints->z[0];

	if (Vect_point_in_box(cx, cy, cz, &box) ||
	    Vect_point_in_box(cx, cy, cz, &abox)) {

	    ret = Vect_point_in_poly(cx, cy, BPoints2);
	    if (!isle) {
		/* area: centroid must be inside */
		if (ret != 1)
		    return 0;
	    }
	    else {
		/* isle: centroid must be outside */
		if (ret > 0)
		    return 0;
	    }
	}
    }

    /* centroid on the right side */
    isle = centr = 0;
    area = right_o;
    if (area < 0) {
	isle = -area;
	area = Vect_get_isle_area(Out, isle);
    }
    if (area > 0)
	centr = Vect_get_area_centroid(Out, area);
    centr_r_o = centr;
    area_r_o = area;
    rbox = box;

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
	    Vect_line_box(BPoints, &tbox);
	    Vect_box_extend(&rbox, &tbox);
	    Vect_append_points(BPoints2, BPoints, dir);
	}
	else
	    Vect_append_points(BPoints2, Points, dir);

	BPoints2->n_points--;    /* skip last point, avoids duplicates */
    }
    BPoints2->n_points++;        /* close polygon */

    if (centr > 0) {
	int ret;
	double cx, cy, cz;

	Vect_read_line(Out, BPoints, NULL, centr);
	cx = BPoints->x[0];
	cy = BPoints->y[0];
	cz = BPoints->z[0];

	if (Vect_point_in_box(cx, cy, cz, &box) ||
	    Vect_point_in_box(cx, cy, cz, &abox)) {

	    ret = Vect_point_in_poly(cx, cy, BPoints2);
	    if (!isle) {
		/* area: centroid must be inside */
		if (ret != 1)
		    return 0;
	    }
	    else {
		/* isle: centroid must be outside */
		if (ret > 0)
		    return 0;
	    }
	}
    }

    /* all fine:
     * areas/isles can be built
     * no intersection with another boundary, e.g. isle attachment will be preserved
     * centroids are still on the correct side of the boundary */

    if (!topo_debug) {
	/* update only those parts of topology that actually get changed */
	/* boundary:
	 * node in case of loop support
	 * node angles
	 * bounding box */
	
	/* rewrite boundary on level 1 */
	offset = Out->plus.Line[line]->offset;
	Out->level = 1;
	offset = Vect_rewrite_line(Out, offset, GV_BOUNDARY, Points, Cats);
	Out->level = 2;
	/* delete line from topo */
	dig_del_line(&Out->plus, line, APoints->x[0], APoints->y[0], APoints->z[0]);
	/* restore line in topo */
	dig_restore_line(&Out->plus, line, GV_BOUNDARY, Points, &box, offset);

	/* update area/isle box to the left */
	if (left_o < 0) {
	    dig_spidx_del_isle(&Out->plus, -left_o);
	    dig_spidx_add_isle(&Out->plus, -left_o, &lbox);
	}
	else if (left_o > 0) {
	    dig_spidx_del_area(&Out->plus, left_o);
	    dig_spidx_add_area(&Out->plus, left_o, &lbox);
	}
	/* update area/isle box to the right */
	if (right_o < 0) {
	    dig_spidx_del_isle(&Out->plus, -right_o);
	    dig_spidx_add_isle(&Out->plus, -right_o, &rbox);
	}
	else if (right_o > 0) {
	    dig_spidx_del_area(&Out->plus, right_o);
	    dig_spidx_add_area(&Out->plus, right_o, &rbox);
	}
	
	/* done */
	return 1;
    }

    /* debug topology */

    /* record isles of the old areas to the left and right */
    nisles_l_o = 0;
    isles_l_o = NULL;
    if (area_l_o) {
	nisles_l_o = Out->plus.Area[area_l_o]->n_isles;
	if (nisles_l_o) {
	    isles_l_o = G_malloc(nisles_l_o * sizeof(int));
	    for (i = 0; i < nisles_l_o; i++) {
		isles_l_o[i] = Out->plus.Area[area_l_o]->isles[i];
	    }
	    qsort(isles_l_o, nisles_l_o, sizeof(int), cmp);
	}
    }
    nisles_r_o = 0;
    isles_r_o = NULL;
    if (area_r_o) {
	nisles_r_o = Out->plus.Area[area_r_o]->n_isles;
	if (nisles_r_o) {
	    isles_r_o = G_malloc(nisles_r_o * sizeof(int));
	    for (i = 0; i < nisles_r_o; i++) {
		isles_r_o[i] = Out->plus.Area[area_r_o]->isles[i];
	    }
	    qsort(isles_r_o, nisles_r_o, sizeof(int), cmp);
	}
    }

    /* OK, rewrite modified boundary */
    newline = Vect_rewrite_line(Out, line, GV_BOUNDARY, Points, Cats);
    if (newline != line)
	G_fatal_error("Vect_rewrite_line(): new line id %d != old line id %d",
	              newline, line);

    /* get new area and centroid ids to the left and right */
    centr_l_n = centr_r_n = 0;
    Vect_get_line_areas(Out, newline, &left_n, &right_n);
    area = left_n;
    if (area < 0)
	area = Vect_get_isle_area(Out, -area);
    if (area > 0)
	centr_l_n = Vect_get_area_centroid(Out, area);
    area_l_n = area;
    
    area = right_n;
    if (area < 0)
	area = Vect_get_isle_area(Out, -area);
    if (area > 0)
	centr_r_n = Vect_get_area_centroid(Out, area);
    area_r_n = area;

    /* record isles of the new areas to the left and right */
    nisles_l_n = 0;
    isles_l_n = NULL;
    if (area_l_n) {
	nisles_l_n = Out->plus.Area[area_l_n]->n_isles;
	if (nisles_l_n) {
	    isles_l_n = G_malloc(nisles_l_n * sizeof(int));
	    for (i = 0; i < nisles_l_n; i++) {
		isles_l_n[i] = Out->plus.Area[area_l_n]->isles[i];
	    }
	    qsort(isles_l_n, nisles_l_n, sizeof(int), cmp);
	}
    }
    nisles_r_n = 0;
    isles_r_n = NULL;
    if (area_r_n) {
	nisles_r_n = Out->plus.Area[area_r_n]->n_isles;
	if (nisles_r_n) {
	    isles_r_n = G_malloc(nisles_r_n * sizeof(int));
	    for (i = 0; i < nisles_r_n; i++) {
		isles_r_n[i] = Out->plus.Area[area_r_n]->isles[i];
	    }
	    qsort(isles_r_n, nisles_r_n, sizeof(int), cmp);
	}
    }

    /* compare isle numbers and ids on the left and right */
    /* left */
    if (nisles_l_o != nisles_l_n)
	G_fatal_error("Number of isles to the left differ: old %d, new %d",
	              nisles_l_o, nisles_l_n);
    found = 0;
    k = 0;
    for (i = 0; i < nisles_l_o; i++) {
	if (isles_l_o[i] != isles_l_n[k]) {
	    if (!found) {
		found = 1;
		k--;
	    }
	    else {
		for (j = 0; j < nisles_l_o; j++) {
		    G_message("old %d new %d", isles_l_o[j], isles_l_n[j]);
		}
		G_fatal_error("New isle to the left %d is wrong",
			      isles_l_n[i]);
	    }
	}
	k++;
    }
    /* right */
    if (nisles_r_o != nisles_r_n)
	G_fatal_error("Number of isles to the left differ: old %d, new %d",
	              nisles_r_o, nisles_r_n);
    found = 0;
    k = 0;
    for (i = 0; i < nisles_r_o; i++) {
	if (isles_r_o[i] != isles_r_n[k]) {
	    if (!found) {
		found = 1;
		k--;
	    }
	    else {
		for (j = 0; j < nisles_r_o; j++) {
		    G_message("old %d new %d", isles_r_o[j], isles_r_n[j]);
		}
		G_fatal_error("New isle to the right %d is wrong",
			      isles_l_n[i]);
	    }
	}
	k++;
    }

    /* Check position of centroids */
    if (centr_l_n != centr_l_o || centr_r_n != centr_r_o) {
	/* should not happen if the above topo checks work as expected */
	G_warning("The modified boundary changes attachment of centroid -> topo checks failed");

	if (centr_l_n != centr_l_o) {
	    G_warning("Left area/isle old: %d, new: %d", left_o, left_n);
	    G_warning("Left centroid old: %d, new: %d", centr_l_o, centr_l_n);

	    if (centr_l_o) {
		int ret1, ret2, ret3;

		Vect_read_line(Out, BPoints, NULL, centr_l_o);
		Vect_get_area_box(Out, area_l_n, &abox);

		ret1 = (BPoints->x[0] >= abox.W && BPoints->x[0] <= abox.E &&
		       BPoints->y[0] >= abox.S && BPoints->y[0] <= abox.N);

		ret2 = Vect_point_in_area_outer_ring(BPoints->x[0], BPoints->y[0], Out,
		                                    area_l_n, &abox);

		Vect_get_area_points(Out, area_l_n, BPoints2);
		ret3 = Vect_point_in_poly(BPoints->x[0], BPoints->y[0], BPoints2);

		if (ret2 != ret3) {
		    G_warning("Left old centroid in new area box: %d", ret1);
		    G_warning("Left old centroid in new area outer ring: %d", ret2);
		    G_warning("Left old centroid in new area as poly: %d", ret3);
		}
	    }
	}
	if (centr_r_n != centr_r_o) {
	    G_warning("Right area/isle old: %d, new: %d", right_o, right_n);
	    G_warning("Right centroid old: %d, new: %d", centr_r_o, centr_r_n);

	    if (centr_r_o) {
		int ret1, ret2, ret3;

		Vect_read_line(Out, BPoints, NULL, centr_r_o);
		Vect_get_area_box(Out, area_r_n, &abox);

		ret1 = (BPoints->x[0] >= abox.W && BPoints->x[0] <= abox.E &&
		       BPoints->y[0] >= abox.S && BPoints->y[0] <= abox.N);

		ret2 = Vect_point_in_area_outer_ring(BPoints->x[0], BPoints->y[0], Out,
		                                    area_r_n, &abox);

		Vect_get_area_points(Out, area_r_n, BPoints2);
		ret3 = Vect_point_in_poly(BPoints->x[0], BPoints->y[0], BPoints2);

		if (ret2 != ret3) {
		    G_warning("Right old centroid in new area box: %d", ret1);
		    G_warning("Right old centroid in new area outer ring: %d", ret2);
		    G_warning("Right old centroid in new area as poly: %d", ret3);
		}
	    }
	}

	/* rewrite old line */
	newline = Vect_rewrite_line(Out, newline, GV_BOUNDARY, APoints, Cats);

	centr_l_n = centr_r_n = 0;
	Vect_get_line_areas(Out, newline, &area_l_n, &area_r_n);
	area = area_l_n;
	if (area < 0)
	    area = Vect_get_isle_area(Out, -area);
	if (area > 0)
	    centr_l_n = Vect_get_area_centroid(Out, area);
	area_l_n = area;
	
	area = area_r_n;
	if (area < 0)
	    area = Vect_get_isle_area(Out, -area);
	if (area > 0)
	    centr_r_n = Vect_get_area_centroid(Out, area);
	area_r_n = area;

	if (centr_l_n != centr_l_o) {
	    Vect_get_area_box(Out, area_l_n, &areabox);
	    
	    if (centr_l_n > 0) {
		Vect_read_line(Out, BPoints, NULL, centr_l_n);
		if (Vect_point_in_area(BPoints->x[0], BPoints->y[0], Out,
		    area_l_n, &areabox)) {

		    G_warning("New left centroid is in new left area %d", area_l_n);

		    G_warning("New left centroid on outer ring: %d",
		    Vect_point_in_area_outer_ring(BPoints->x[0], BPoints->y[0], Out,
		    area_l_n, &areabox));

		    G_warning("Best area for new left centroid: %d",
			      Vect_find_area(Out, BPoints->x[0], BPoints->y[0]));
		}
		else
		    G_warning("New left centroid is not in new left area");
	    }

	    if (centr_l_o > 0) {
		Vect_read_line(Out, BPoints, NULL, centr_l_o);
		if (Vect_point_in_area(BPoints->x[0], BPoints->y[0], Out,
		    area_l_n, &areabox)) {

		    G_warning("Old left centroid is in new left area %d", area_l_n);

		    G_warning("Old left centroid on outer ring: %d",
		    Vect_point_in_area_outer_ring(BPoints->x[0], BPoints->y[0], Out,
		    area_l_n, &areabox));

		    G_warning("Best area for old left centroid: %d",
			      Vect_find_area(Out, BPoints->x[0], BPoints->y[0]));
		}
		else
		    G_warning("Old left centroid is not in new left area");
	    }

	    G_fatal_error("Left centroid old %d, restored %d", centr_l_o, centr_l_n);
	    return 0;
	}
	if (centr_r_n != centr_r_o) {
	    Vect_get_area_box(Out, area_r_n, &areabox);

	    if (centr_r_n > 0) {
		Vect_read_line(Out, BPoints, NULL, centr_r_n);
		if (Vect_point_in_area(BPoints->x[0], BPoints->y[0], Out,
		    area_r_n, &areabox)) {

		    G_warning("New right centroid is in new right area %d", area_r_n);

		    G_warning("New right centroid on outer ring: %d",
		    Vect_point_in_area_outer_ring(BPoints->x[0], BPoints->y[0], Out,
		    area_r_n, &areabox));

		    G_warning("Best area for new right centroid: %d",
			      Vect_find_area(Out, BPoints->x[0], BPoints->y[0]));
		}
		else
		    G_warning("New right centroid is not in new right area");
	    }

	    if (centr_r_o > 0) {
		Vect_read_line(Out, BPoints, NULL, centr_r_o);
		if (Vect_point_in_area(BPoints->x[0], BPoints->y[0], Out,
		    area_r_n, &areabox)) {

		    G_warning("Old right centroid is in new right area %d", area_r_n);

		    G_warning("Old right centroid on outer ring: %d",
		    Vect_point_in_area_outer_ring(BPoints->x[0], BPoints->y[0], Out,
		    area_r_n, &areabox));

		    G_warning("Best area for old right centroid: %d",
			      Vect_find_area(Out, BPoints->x[0], BPoints->y[0]));
		}
		else
		    G_warning("Old right centroid is not in new right area");
	    }

	    G_fatal_error("Right centroid old %d, restored %d", centr_r_o, centr_r_n);
	    return 0;
	}

	G_fatal_error("Topology check failure");
	return 0;
    }
    if (isles_l_o)
	G_free(isles_l_o);
    if (isles_r_o)
	G_free(isles_r_o);
    if (isles_l_n)
	G_free(isles_l_n);
    if (isles_r_n)
	G_free(isles_r_n);
    
    return 1;
}
