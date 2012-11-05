/*
 *****************************************************************************
 *  
 *  MODULE: v.overlay 
 *
 *  AUTHOR(S): Radim Blazek
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

int area_area(struct Map_info *In, int *field, struct Map_info *Out,
	      struct field_info *Fi, dbDriver * driver, int operator,
	      int *ofield, ATTRIBUTES * attr, struct ilist *BList)
{
    int ret, input, line, nlines, area, nareas;
    int in_area, in_centr, out_cat;
    struct line_pnts *Points;
    struct line_cats *Cats;
    CENTR *Centr;
    char *Del;
    char buf[1000];
    dbString stmt;
    int nmodif;

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    /* Vect_clean_small_angles_at_nodes() can change the geometry so that new intersections
     * are created. We must call Vect_break_lines(), Vect_remove_duplicates()
     * and Vect_clean_small_angles_at_nodes() until no more small dangles are found */
    do {
	G_message(_("Breaking lines..."));
	Vect_break_lines_list(Out, NULL, BList, GV_LINE | GV_BOUNDARY, NULL);

	/* Probably not necessary for LINE x AREA */
	G_message(_("Removing duplicates..."));
	Vect_remove_duplicates(Out, GV_BOUNDARY, NULL);

	G_message(_("Cleaning boundaries at nodes..."));
	nmodif =
	    Vect_clean_small_angles_at_nodes(Out, GV_BOUNDARY, NULL);
    } while (nmodif > 0);

    /* ?: May be result of Vect_break_lines() + Vect_remove_duplicates() any dangle or bridge?
     * In that case, calls to Vect_remove_dangles() and Vect_remove_bridges() would be also necessary */

    Vect_build_partial(Out, GV_BUILD_AREAS);
    nlines = Vect_get_num_lines(Out);
    ret = 0;
    for (line = 1; line <= nlines; line++) {
	if (!Vect_line_alive(Out, line))
	    continue;
	if (Vect_get_line_type(Out, line) == GV_BOUNDARY) {
	    int left, rite;
	    
	    Vect_get_line_areas(Out, line, &left, &rite);
	    
	    if (left == 0 || rite == 0) {
		ret = 1;
		break;
	    }
	}
    }
    if (ret) {
	Vect_remove_dangles(Out, GV_BOUNDARY, -1, NULL);
	Vect_remove_bridges(Out, NULL, NULL, NULL);
    }

    /* Attach islands */
    G_message(_("Attaching islands..."));
    Vect_build_partial(Out, GV_BUILD_ATTACH_ISLES);


    /* Calculate new centroids for all areas */
    nareas = Vect_get_num_areas(Out);

    Centr = (CENTR *) G_malloc((nareas + 1) * sizeof(CENTR));	/* index from 1 ! */
    for (area = 1; area <= nareas; area++) {
	ret =
	    Vect_get_point_in_area(Out, area, &(Centr[area].x),
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

			    Vect_cat_set(Centr[area].cat[input], field[input],
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
	    G_percent(area, nareas, 1);
	}
    }

    G_message(_("Writing centroids..."));

    db_init_string(&stmt);
    out_cat = 1;
    for (area = 1; area <= nareas; area++) {
	int i;

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

	/* Add all cats from imput vectors */
	if (ofield[1] > 0) {
	    for (i = 0; i < Centr[area].cat[0]->n_cats; i++) {
		Vect_cat_set(Cats, ofield[1], Centr[area].cat[0]->cat[i]);
	    }
	}

	if (ofield[2] > 0) {
	    for (i = 0; i < Centr[area].cat[1]->n_cats; i++) {
		Vect_cat_set(Cats, ofield[2], Centr[area].cat[1]->cat[i]);
	    }
	}

	Vect_write_line(Out, GV_CENTROID, Points, Cats);

	G_percent(area, nareas, 1);
    }

    /* Build topology and remove boundaries with area without centroid on both sides */
    G_message(_("Attaching centroids..."));
    Vect_build_partial(Out, GV_BUILD_ALL);

    /* Create a list of lines to be deleted */
    nlines = Vect_get_num_lines(Out);
    Del = (char *)G_calloc(nlines + 1, sizeof(char));	/* index from 1 ! */

    for (line = 1; line <= nlines; line++) {
	int i, ltype, side[2], centr[2];

	G_percent(line, nlines, 1);	/* must be before any continue */

	if (!Vect_line_alive(Out, line))
	    continue;

	ltype = Vect_read_line(Out, NULL, NULL, line);
	if (!(ltype & GV_BOUNDARY))
	    continue;

	Vect_get_line_areas(Out, line, &side[0], &side[1]);

	for (i = 0; i < 2; i++) {
	    if (side[i] == 0) {	/* This should not happen ! */
		centr[i] = 0;
		continue;
	    }

	    if (side[i] > 0) {
		area = side[i];
	    }
	    else {		/* island */
		area = Vect_get_isle_area(Out, abs(side[i]));
	    }

	    if (area > 0)
		centr[i] = Vect_get_area_centroid(Out, area);
	    else
		centr[i] = 0;
	}

	if (!centr[0] && !centr[1])
	    Del[line] = 1;
    }

    /* Delete boundaries */
    for (line = 1; line <= nlines; line++) {
	if (Del[line])
	    Vect_delete_line(Out, line);
    }
    G_free(Del);

    return 0;
}
