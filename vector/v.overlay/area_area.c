/*
 *****************************************************************************
 *  
 *  MODULE: v.overlay 
 *
 *  AUTHOR(S): Radim Blazek, Markus Metz
 *  
 ******************************************************************************/
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/vector.h>
#include <grass/glocale.h>
#include "local.h"

/* for ilist qsort'ing and bsearch'ing */
static int cmp_int(const void *a, const void *b)
{
    return (*(int *)a - *(int *)b);
}

int area_area(struct Map_info *In, int *field, struct Map_info *Tmp,
	      struct Map_info *Out, struct field_info *Fi,
	      dbDriver * driver, int operator, int *ofield,
	      ATTRIBUTES * attr, struct ilist *BList, double snap)
{
    int ret, input, line, nlines, area, nareas;
    int in_area, in_centr, out_cat;
    struct line_pnts *Points;
    struct line_cats *Cats;
    CENTR *Centr;
    char buf[1000];
    dbString stmt;
    int nmodif;
    int verbose;

    verbose = G_verbose();

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    /* optional snap */
    if (snap > 0) {
	int i, j, snapped_lines = 0;
	struct bound_box box;
	struct boxlist *boxlist = Vect_new_boxlist(0);
	struct ilist *reflist = Vect_new_list();
	
	G_message(_("Snapping boundaries with %g ..."), snap);

	/* snap boundaries in B to boundaries in A,
	 * not modifying boundaries in A */

	if (BList->n_values > 1)
	    qsort(BList->value, BList->n_values, sizeof(int), cmp_int);

	snapped_lines = 0;
	nlines = BList->n_values;
	for (i = 0; i < nlines; i++) {
	    line = BList->value[i];
	    Vect_read_line(Tmp, Points, Cats, line);
	    /* select lines by box */
	    Vect_get_line_box(Tmp, line, &box);
	    box.E += snap;
	    box.W -= snap;
	    box.N += snap;
	    box.S -= snap;
	    box.T = 0.0;
	    box.B = 0.0;
	    Vect_select_lines_by_box(Tmp, &box, GV_BOUNDARY, boxlist);
	    
	    if (boxlist->n_values > 0) {
		Vect_reset_list(reflist);
		for (j = 0; j < boxlist->n_values; j++) {
		    int aline = boxlist->id[j];

		    if (!bsearch(&aline, BList->value, BList->n_values,
			sizeof(int), cmp_int)) {
			G_ilist_add(reflist, aline);
		    }
		}
		
		/* snap bline to alines */
		if (Vect_snap_line(Tmp, reflist, Points, snap, 0, NULL, NULL)) {
		    /* rewrite bline*/
		    Vect_delete_line(Tmp, line);
		    ret = Vect_write_line(Tmp, GV_BOUNDARY, Points, Cats);
		    G_ilist_add(BList, ret);
		    snapped_lines++;
		    G_debug(3, "line %d snapped", line);
		}
	    }
	}
	Vect_destroy_boxlist(boxlist);
	Vect_destroy_list(reflist);

	G_verbose_message(_("%d boundaries snapped"), snapped_lines);
    }

    /* same procedure like for v.in.ogr:
     * Vect_clean_small_angles_at_nodes() can change the geometry so that new intersections
     * are created. We must call Vect_break_lines(), Vect_remove_duplicates()
     * and Vect_clean_small_angles_at_nodes() until no more small dangles are found */
    do {
	G_message(_("Breaking lines..."));
	Vect_break_lines_list(Tmp, NULL, BList, GV_BOUNDARY, NULL);

	/* Probably not necessary for LINE x AREA */
	G_message(_("Removing duplicates..."));
	Vect_remove_duplicates(Tmp, GV_BOUNDARY, NULL);

	G_message(_("Cleaning boundaries at nodes..."));
	nmodif =
	    Vect_clean_small_angles_at_nodes(Tmp, GV_BOUNDARY, NULL);
    } while (nmodif > 0);

    /* ?: May be result of Vect_break_lines() + Vect_remove_duplicates() any dangle or bridge?
     * In that case, calls to Vect_remove_dangles() and Vect_remove_bridges() would be also necessary */

    G_set_verbose(0);
    /* should be fast, be silent */
    Vect_build_partial(Tmp, GV_BUILD_AREAS);
    G_set_verbose(verbose);
    nlines = Vect_get_num_lines(Tmp);
    ret = 0;
    for (line = 1; line <= nlines; line++) {
	if (!Vect_line_alive(Tmp, line))
	    continue;
	if (Vect_get_line_type(Tmp, line) == GV_BOUNDARY) {
	    int left, rite;
	    
	    Vect_get_line_areas(Tmp, line, &left, &rite);
	    
	    if (left == 0 || rite == 0) {
		/* invalid boundary */
		ret = 1;
		break;
	    }
	}
    }
    if (ret) {
	Vect_remove_dangles(Tmp, GV_BOUNDARY, -1, NULL);
	Vect_remove_bridges(Tmp, NULL, NULL, NULL);
    }

    G_set_verbose(0);
    Vect_build_partial(Tmp, GV_BUILD_NONE);
    Vect_build_partial(Tmp, GV_BUILD_BASE);
    G_set_verbose(verbose);
    G_message(_("Merging lines..."));
    Vect_merge_lines(Tmp, GV_BOUNDARY, NULL, NULL);

    /* Attach islands */
    G_message(_("Attaching islands..."));
    /* can take some time, show messages */
    Vect_build_partial(Tmp, GV_BUILD_ATTACH_ISLES);

    /* Calculate new centroids for all areas */
    nareas = Vect_get_num_areas(Tmp);

    Centr = (CENTR *) G_malloc((nareas + 1) * sizeof(CENTR));	/* index from 1 ! */
    for (area = 1; area <= nareas; area++) {
	ret =
	    Vect_get_point_in_area(Tmp, area, &(Centr[area].x),
				   &(Centr[area].y));
	if (ret < 0) {
	    G_warning(_("Cannot calculate area centroid"));
	    Centr[area].valid = 0;
	}
	else {
	    Centr[area].valid = 1;
	}
    }

    /* Query input maps */
    for (input = 0; input < 2; input++) {
	G_message(_("Querying vector map <%s>..."),
		  Vect_get_full_name(&(In[input])));

	for (area = 1; area <= nareas; area++) {
	    Centr[area].cat[input] = Vect_new_cats_struct();

	    G_percent(area, nareas, 1);

	    in_area =
		Vect_find_area(&(In[input]), Centr[area].x, Centr[area].y);
	    if (in_area > 0) {
		in_centr = Vect_get_area_centroid(&(In[input]), in_area);
		if (in_centr > 0) {
		    int i;

		    Vect_read_line(&(In[input]), NULL, Cats, in_centr);
		    /* Add all cats with original field number */
		    for (i = 0; i < Cats->n_cats; i++) {
			if (Cats->field[i] == field[input]) {
			    ATTR *at;

			    Vect_cat_set(Centr[area].cat[input], ofield[input + 1],
					 Cats->cat[i]);

			    /* Mark as used */
			    at = find_attr(&(attr[input]), Cats->cat[i]);
			    if (!at)
				G_fatal_error(_("Attribute not found"));

			    at->used = 1;
			}
		    }
		}
	    }
	}
    }

    G_message(_("Writing centroids..."));

    db_init_string(&stmt);
    out_cat = 1;
    for (area = 1; area <= nareas; area++) {
	int i;

	G_percent(area, nareas, 1);

	/* check the condition */
	switch (operator) {
	case OP_AND:
	    if (!
		(Centr[area].cat[0]->n_cats > 0 &&
		 Centr[area].cat[1]->n_cats > 0))
		continue;
	    break;
	case OP_OR:
	    if (!
		(Centr[area].cat[0]->n_cats > 0 ||
		 Centr[area].cat[1]->n_cats > 0))
		continue;
	    break;
	case OP_NOT:
	    if (!
		(Centr[area].cat[0]->n_cats > 0 &&
		 !(Centr[area].cat[1]->n_cats > 0)))
		continue;
	    break;
	case OP_XOR:
	    if ((Centr[area].cat[0]->n_cats > 0 &&
		 Centr[area].cat[1]->n_cats > 0) ||
		(!(Centr[area].cat[0]->n_cats > 0) &&
		 !(Centr[area].cat[1]->n_cats > 0)))
		continue;
	    break;
	}

	Vect_reset_line(Points);
	Vect_reset_cats(Cats);

	Vect_append_point(Points, Centr[area].x, Centr[area].y, 0.0);

	if (ofield[0] > 0) {
	    /* Add new cats for all combinations of input cats (-1 in cycle for null) */
	    for (i = -1; i < Centr[area].cat[0]->n_cats; i++) {
		int j;

		if (i == -1 && Centr[area].cat[0]->n_cats > 0)
		    continue;	/* no need to make null */

		for (j = -1; j < Centr[area].cat[1]->n_cats; j++) {
		    if (j == -1 && Centr[area].cat[1]->n_cats > 0)
			continue;	/* no need to make null */

		    if (ofield[0] > 0)
			Vect_cat_set(Cats, ofield[0], out_cat);

		    /* attributes */
		    if (driver) {
			ATTR *at;

			sprintf(buf, "insert into %s values ( %d", Fi->table,
				out_cat);
			db_set_string(&stmt, buf);

			/* cata */
			if (i >= 0) {
			    if (attr[0].columns) {
				at = find_attr(&(attr[0]),
					       Centr[area].cat[0]->cat[i]);
				if (!at)
				    G_fatal_error(_("Attribute not found"));

				if (at->values)
				    db_append_string(&stmt, at->values);
				else
				    db_append_string(&stmt, attr[0].null_values);
			    }
			    else {
				sprintf(buf, ", %d", Centr[area].cat[0]->cat[i]);
				db_append_string(&stmt, buf);
			    }
			}
			else {
			    if (attr[0].columns) {
				db_append_string(&stmt, attr[0].null_values);
			    }
			    else {
				sprintf(buf, ", null");
				db_append_string(&stmt, buf);
			    }
			}

			/* catb */
			if (j >= 0) {
			    if (attr[1].columns) {
				at = find_attr(&(attr[1]),
					       Centr[area].cat[1]->cat[j]);
				if (!at)
				    G_fatal_error(_("Attribute not found"));

				if (at->values)
				    db_append_string(&stmt, at->values);
				else
				    db_append_string(&stmt, attr[1].null_values);
			    }
			    else {
				sprintf(buf, ", %d", Centr[area].cat[1]->cat[j]);
				db_append_string(&stmt, buf);
			    }
			}
			else {
			    if (attr[1].columns) {
				db_append_string(&stmt, attr[1].null_values);
			    }
			    else {
				sprintf(buf, ", null");
				db_append_string(&stmt, buf);
			    }
			}

			db_append_string(&stmt, " )");

			G_debug(3, db_get_string(&stmt));

			if (db_execute_immediate(driver, &stmt) != DB_OK)
			    G_warning(_("Unable to insert new record: '%s'"),
				      db_get_string(&stmt));
		    }
		    out_cat++;
		}
	    }
	}

	/* Add all cats from input vectors */
	if (ofield[1] > 0 && field[0] > 0) {
	    for (i = 0; i < Centr[area].cat[0]->n_cats; i++) {
		if (Centr[area].cat[0]->field[i] == field[0])
		    Vect_cat_set(Cats, ofield[1], Centr[area].cat[0]->cat[i]);
	    }
	}

	if (ofield[2] > 0 && field[1] > 0 && ofield[1] != ofield[2]) {
	    for (i = 0; i < Centr[area].cat[1]->n_cats; i++) {
		if (Centr[area].cat[1]->field[i] == field[1])
		    Vect_cat_set(Cats, ofield[2], Centr[area].cat[1]->cat[i]);
	    }
	}

	Vect_write_line(Tmp, GV_CENTROID, Points, Cats);
	Vect_write_line(Out, GV_CENTROID, Points, Cats);
    }

    G_set_verbose(0);
    /* should be fast, be silent */
    Vect_build_partial(Tmp, GV_BUILD_CENTROIDS);
    G_set_verbose(verbose);
    /* Copy valid boundaries to final output */
    nlines = Vect_get_num_lines(Tmp);

    for (line = 1; line <= nlines; line++) {
	int i, ltype, side[2], centr[2];

	G_percent(line, nlines, 1);	/* must be before any continue */

	if (!Vect_line_alive(Tmp, line))
	    continue;

	ltype = Vect_read_line(Tmp, Points, Cats, line);
	if (!(ltype & GV_BOUNDARY))
	    continue;

	Vect_get_line_areas(Tmp, line, &side[0], &side[1]);

	for (i = 0; i < 2; i++) {
	    if (side[i] == 0) {	/* This should not happen ! */
		centr[i] = 0;
		continue;
	    }

	    if (side[i] > 0) {
		area = side[i];
	    }
	    else {		/* island */
		area = Vect_get_isle_area(Tmp, abs(side[i]));
	    }

	    if (area > 0)
		centr[i] = Vect_get_area_centroid(Tmp, area);
	    else
		centr[i] = 0;
	}

	if (centr[0] || centr[1])
	    Vect_write_line(Out, GV_BOUNDARY, Points, Cats);
    }

    return 0;
}
