/* ****************************************************************************
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

/* Check if point is inside area with category of given field. All cats are set in 
 * Cats with original field.
 * returns number of cats.
 */
int point_area(struct Map_info *Map, int field, double x, double y,
	       struct line_cats *Cats)
{
    int i, area, centr;
    static struct line_cats *CCats = NULL;

    Vect_reset_cats(Cats);
    area = Vect_find_area(Map, x, y);
    G_debug(4, "  area = %d", area);

    if (!area)
	return 0;

    centr = Vect_get_area_centroid(Map, area);

    if (centr <= 0)
	return 0;

    if (!CCats)
	CCats = Vect_new_cats_struct();
    Vect_read_line(Map, NULL, CCats, centr);

    for (i = 0; i < CCats->n_cats; i++) {
	if (CCats->field[i] == field) {
	    Vect_cat_set(Cats, field, CCats->cat[i]);
	}
    }

    return Cats->n_cats;
}

int line_area(struct Map_info *In, int *field, struct Map_info *Out,
	      struct field_info *Fi, dbDriver * driver, int operator,
	      int *ofield, ATTRIBUTES * attr, struct ilist *BList)
{
    int line, nlines, ncat;
    struct line_pnts *Points;
    struct line_cats *Cats, *ACats, *OCats;

    char buf[1000];
    dbString stmt;

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();
    ACats = Vect_new_cats_struct();
    OCats = Vect_new_cats_struct();
    db_init_string(&stmt);

    G_message(_("Breaking lines..."));
    Vect_break_lines_list(Out, NULL, BList, GV_LINE | GV_BOUNDARY, NULL);
    G_message(_("Merging lines..."));
    Vect_merge_lines(Out, GV_LINE, NULL, NULL);

#if 0
    /* Basic topology needed only */
    Vect_build_partial(Out, GV_BUILD_BASE);
#endif

    nlines = Vect_get_num_lines(Out);

    /* Warning!: cleaning process (break) creates new vertices which are usually slightly 
     * moved (RE), to compare such new vertex with original input is a problem?
     * 
     * TODO?: would it be better to copy centroids also and query output map? 
     */

    /* Check if the line is inside or outside binput area */
    ncat = 1;
    for (line = 1; line <= nlines; line++) {
	int ltype;

	G_percent(line, nlines, 1);	/* must be before any continue */

	if (!Vect_line_alive(Out, line))
	    continue;

	ltype = Vect_read_line(Out, Points, Cats, line);

	if (ltype == GV_BOUNDARY) {	/* No more needed */
	    Vect_delete_line(Out, line);
	    continue;
	}

	/* Now the type should be only GV_LINE */

	/* Decide if the line is inside or outside the area. In theory:
	 * 1) All vertices outside
	 *      - easy, first vertex must be outside
	 * 2) All vertices inside 
	 * 3) All vertices on the boundary, we take it as inside (attention, 
	 *    result of Vect_point_in_area() for points on segments between vertices may be both
	 *    inside or outside, because of representation of numbers)
	 * 4) One or two end vertices on the boundary, all others outside
	 * 5) One or two end vertices on the boundary, all others inside 
	 *
	 */

	/* Note/TODO: the test done is quite simple, check the point in the middle of segment.
	 * If the line overpals the boundary, the result may be both outside and inside
	 * this should be solved (check angles?)
	 */

	G_debug(3, "line = %d", line);

	point_area(&(In[1]), field[1], (Points->x[0] + Points->x[1]) / 2,
		   (Points->y[0] + Points->y[1]) / 2, ACats);

	if ((ACats->n_cats > 0 && operator == OP_AND) ||
	    (ACats->n_cats == 0 && operator == OP_NOT)) {
	    int i;

	    /* Point is inside */
	    G_debug(3, "OK, write line, line ncats = %d area ncats = %d",
		    Cats->n_cats, ACats->n_cats);

	    Vect_reset_cats(OCats);

	    /* rewrite with all combinations of acat - bcat (-1 in cycle for null) */
	    /* TODO: put cats of input maps into different layers, i.e.
	     * preserve cat values, change layer number if needed */
	    for (i = -1; i < Cats->n_cats; i++) {	/* line cats */
		int j;

		if (i == -1 && Cats->n_cats > 0)
		    continue;	/* no need to make null */

		for (j = -1; j < ACats->n_cats; j++) {
		    if (j == -1 && ACats->n_cats > 0)
			continue;	/* no need to make null */

		    if (ofield[0] > 0)
			Vect_cat_set(OCats, ofield[0], ncat);

		    /* Attributes */
		    if (driver) {
			ATTR *at;

			sprintf(buf, "insert into %s values ( %d", Fi->table,
				ncat);
			db_set_string(&stmt, buf);

			/* cata */
			if (i >= 0) {
			    if (attr[0].columns) {
				at = find_attr(&(attr[0]), Cats->cat[i]);
				if (!at)
				    G_fatal_error(_("Attribute not found"));

				if (at->values)
				    db_append_string(&stmt, at->values);
				else
				    db_append_string(&stmt,
						     attr[0].null_values);
			    }
			    else {
				sprintf(buf, ", %d", Cats->cat[i]);
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
				at = find_attr(&(attr[1]), ACats->cat[j]);
				if (!at)
				    G_fatal_error(_("Attribute not found"));

				if (at->values)
				    db_append_string(&stmt, at->values);
				else
				    db_append_string(&stmt,
						     attr[1].null_values);
			    }
			    else {
				sprintf(buf, ", %d", ACats->cat[j]);
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

		    ncat++;
		}
	    }

	    /* Add all cats from imput vectors */
	    if (ofield[1] > 0) {
		for (i = 0; i < Cats->n_cats; i++) {
		    Vect_cat_set(OCats, ofield[1], Cats->cat[i]);
		}
	    }

	    if (ofield[2] > 0) {
		for (i = 0; i < ACats->n_cats; i++) {
		    Vect_cat_set(OCats, ofield[2], ACats->cat[i]);
		}
	    }

	    Vect_rewrite_line(Out, line, ltype, Points, OCats);
	}
	else {
	    Vect_delete_line(Out, line);
	}
    }

    return 0;
}
