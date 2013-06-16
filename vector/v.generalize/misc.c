
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
    int i, intersect, newline, left_old, right_old,
	left_new, right_new;
    struct bound_box box;
    static struct line_pnts *BPoints = NULL;
    static struct boxlist *List = NULL;

    if (!BPoints)
	BPoints = Vect_new_line_struct();
    if (!List)
	List = Vect_new_boxlist(1);

    /* Check intersection of the modified boundary with other boundaries */
    Vect_line_box(Points, &box);
    Vect_select_lines_by_box(Out, &box, GV_BOUNDARY, List);

    intersect = 0;
    for (i = 0; i < List->n_values; i++) {
	int j, bline;
	struct line_pnts **AXLines, **BXLines;
	int naxlines, nbxlines;

	bline = List->id[i];
	if (bline == line)
	    continue;

	Vect_read_line(Out, BPoints, NULL, bline);

	/* Vect_line_intersection is quite slow, hopefully not so bad because only few 
	 * intersections should be found if any */

	AXLines = BXLines = NULL;
	Vect_line_intersection(Points, BPoints, &box, &List->box[i], &AXLines, &BXLines,
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

    /* Get centroids on the left and right side */
    Vect_get_line_areas(Out, line, &left_old, &right_old);
    if (left_old < 0)
	left_old = Vect_get_isle_area(Out, abs(left_old));
    if (left_old > 0)
	left_old = Vect_get_area_centroid(Out, left_old);
    if (right_old < 0)
	right_old = Vect_get_isle_area(Out, abs(right_old));
    if (right_old > 0)
	right_old = Vect_get_area_centroid(Out, right_old);

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
	Vect_rewrite_line(Out, newline, GV_BOUNDARY, APoints, Cats);
	return 0;
    }
    
    return 1;
}
