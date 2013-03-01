/* ****************************************************************************
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

/* compare category structures
 * return 0 identical
 * return 1 not identical
 */
static int compare_cats(struct line_cats *ACats, struct line_cats *BCats)
{
    int i, j;

    if (ACats->n_cats == 0 || BCats->n_cats == 0) {
	if (ACats->n_cats == 0 && BCats->n_cats == 0)
	    return 0;

	if (ACats->n_cats == 0 && BCats->n_cats > 0)
	    return 1;

	if (ACats->n_cats > 0 && BCats->n_cats == 0)
	    return 1;
    }

    for (i = 0; i < ACats->n_cats; i++) {
	int found = 0;

	for (j = 0; j < BCats->n_cats; j++) {
	    if (ACats->cat[i] == BCats->cat[j] &&
	        ACats->field[i] == BCats->field[j]) {
		found = 1;
		break;
	    }
	}
	if (!found)
	    return 1;
    }
    
    return 0;
}

/* merge a given line with all other lines of the same type and 
 * with the same categories */
static int merge_line(struct Map_info *Map, int line,
		      struct line_pnts *MPoints)
{
    int nlines, i, first, last, next_line, curr_line;
    int merged = 0, newl = 0;
    int next_node, direction, node_n_lines, type, ltype, lines_type;
    static struct ilist *List = NULL;
    static struct line_pnts *Points = NULL;
    static struct line_cats *MCats = NULL, *Cats = NULL;
    type = GV_LINE;

    nlines = Vect_get_num_lines(Map);

    if (!Points)
	Points = Vect_new_line_struct();
    if (!Cats)
	Cats = Vect_new_cats_struct();
    if (!MCats)
	MCats = Vect_new_cats_struct();
    if (!List)
	List = Vect_new_list();

    Vect_reset_line(Points);
    Vect_reset_cats(Cats);
    Vect_reset_cats(MCats);
    Vect_reset_list(List);

    if (!Vect_line_alive(Map, line))
	return 0;

    ltype = Vect_get_line_type(Map, line);

    if (!(ltype & type))
	return 0;
	
    Vect_read_line(Map, NULL, MCats, line);

    /* special cases:
     *  - loop back to start boundary via several other boundaries
     *  - one boundary forming closed loop
     *  - node with 3 entries but only 2 boundaries, one of them connecting twice,
     *    the other one must then be topologically incorrect in case of boundary */

    /* go backward as long as there is only one other line/boundary at the current node */
    G_debug(3, "go backward");
    Vect_get_line_nodes(Map, line, &next_node, NULL);

    first = -line;
    while (1) {
	node_n_lines = Vect_get_node_n_lines(Map, next_node);
	/* count lines/boundaries at this node */
	lines_type = 0;
	next_line = first;
	for (i = 0; i < node_n_lines; i++) {
	    curr_line = Vect_get_node_line(Map, next_node, i);
	    if ((Vect_get_line_type(Map, abs(curr_line)) & GV_LINES))
		lines_type++;
	    if ((Vect_get_line_type(Map, abs(curr_line)) == ltype)) {
		if (abs(curr_line) != abs(first)) {
		    Vect_read_line(Map, NULL, Cats, abs(curr_line));
		    
		    /* catgories must be identical */
		    if (compare_cats(MCats, Cats) == 0)
			next_line = curr_line;
		}
	    }
	}
	if (lines_type == 2 && abs(next_line) != abs(first) &&
	    abs(next_line) != line) {
	    first = next_line;

	    if (first < 0) {
		Vect_get_line_nodes(Map, -first, &next_node, NULL);
	    }
	    else {
		Vect_get_line_nodes(Map, first, NULL, &next_node);
	    }
	}
	else
	    break;
    }

    /* go forward as long as there is only one other line/boundary at the current node */
    G_debug(3, "go forward");

    /* reverse direction */
    last = -first;

    if (last < 0) {
	Vect_get_line_nodes(Map, -last, &next_node, NULL);
    }
    else {
	Vect_get_line_nodes(Map, last, NULL, &next_node);
    }

    Vect_reset_list(List);
    while (1) {
	G_ilist_add(List, last);
	node_n_lines = Vect_get_node_n_lines(Map, next_node);
	lines_type = 0;
	next_line = last;
	for (i = 0; i < node_n_lines; i++) {
	    curr_line = Vect_get_node_line(Map, next_node, i);
	    if ((Vect_get_line_type(Map, abs(curr_line)) & GV_LINES))
		lines_type++;
	    if ((Vect_get_line_type(Map, abs(curr_line)) == ltype)) {
		if (abs(curr_line) != abs(last)) {
		    Vect_read_line(Map, NULL, Cats, abs(curr_line));
		    
		    if (compare_cats(MCats, Cats) == 0)
			next_line = curr_line;
		}
	    }
	}

	if (lines_type == 2 && abs(next_line) != abs(last) &&
	    abs(next_line) != abs(first)) {
	    last = next_line;

	    if (last < 0) {
		Vect_get_line_nodes(Map, -last, &next_node, NULL);
	    }
	    else {
		Vect_get_line_nodes(Map, last, NULL, &next_node);
	    }
	}
	else
	    break;
    }

    /* merge lines */
    G_debug(3, "merge %d lines", List->n_values);
    Vect_reset_line(MPoints);

    for (i = 0; i < List->n_values; i++) {
	Vect_reset_line(Points);
	Vect_read_line(Map, Points, Cats, abs(List->value[i]));
	direction = (List->value[i] < 0 ? GV_BACKWARD : GV_FORWARD);
	Vect_append_points(MPoints, Points, direction);
	MPoints->n_points--;
	Vect_delete_line(Map, abs(List->value[i]));
    }
    MPoints->n_points++;
    merged += List->n_values;
    newl++;

    return merged;
}

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

int line_area(struct Map_info *In, int *field, struct Map_info *Tmp,
	      struct Map_info *Out, struct field_info *Fi,
	      dbDriver * driver, int operator, int *ofield,
	      ATTRIBUTES * attr, struct ilist *BList)
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
    Vect_break_lines_list(Tmp, NULL, BList, GV_LINE | GV_BOUNDARY, NULL);

    /*
    G_message(_("Merging lines..."));
    Vect_merge_lines(Tmp, GV_LINE, NULL, NULL);
    */

    nlines = Vect_get_num_lines(Tmp);

    /* Warning!: cleaning process (break) creates new vertices which are usually slightly 
     * moved (RE), to compare such new vertex with original input is a problem?
     * 
     * TODO?: would it be better to copy centroids also and query output map? 
     */

    /* Check if the line is inside or outside binput area */
    G_message(_("Selecting lines..."));
    ncat = 1;
    for (line = 1; line <= nlines; line++) {
	int ltype;

	G_percent(line, nlines, 1);	/* must be before any continue */

	if (!Vect_line_alive(Tmp, line))
	    continue;

	ltype = Vect_read_line(Tmp, Points, Cats, line);

	if (ltype == GV_BOUNDARY) {	/* No more needed */
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
	 * If the line overlaps the boundary, the result may be both outside and inside
	 * this should be solved (check angles?)
	 * This should not happen if Vect_break_lines_list() works correctly
	 */

	/* merge here */
	merge_line(Tmp, line, Points);

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

	    if (ofield[0]  > 0) {
		/* rewrite with all combinations of acat - bcat (-1 in cycle for null) */
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
	    }

	    /* Add cats from input vectors */
	    if (ofield[1] > 0 && field[0] > 0) {
		for (i = 0; i < Cats->n_cats; i++) {
		    if (Cats->field[i] == field[0])
			Vect_cat_set(OCats, ofield[1], Cats->cat[i]);
		}
	    }

	    if (ofield[2] > 0 && field[1] > 0 && ofield[1] != ofield[2]) {
		for (i = 0; i < ACats->n_cats; i++) {
		    if (Cats->field[i] == field[1])
			Vect_cat_set(OCats, ofield[2], ACats->cat[i]);
		}
	    }

	    Vect_write_line(Out, ltype, Points, OCats);
	}
    }

    return 0;
}
