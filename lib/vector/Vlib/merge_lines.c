/*!
   \file lib/vector/Vlib/merge_lines.c

   \brief Vector library - clean geometry (merge lines/boundaries)

   Higher level functions for reading/writing/manipulating vectors.

   (C) 2001-2009 by the GRASS Development Team

   This program is free software under the 
   GNU General Public License (>=v2). 
   Read the file COPYING that comes with GRASS
   for details.

   \author Markus Metz
 */

#include <stdlib.h>
#include <math.h>
#include <grass/vector.h>
#include <grass/glocale.h>

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

/*!
   \brief Merge lines or boundaries in vector map.

   Merges lines specified by type in vector map.
   Useful for generalization and smoothing.
   Adjacent boundaries are merged as long as topology is maintained.
   Adjacent lines are merged as long as there are exactly two different 
   lines connected at a given node.
   Categories are added up when merging.
   GV_BUILD_BASE as topo build level is sufficient, areas need not be built.

   \param Map input vector map 
   \param type feature type
   \param[out] Err vector map where merged lines/boundaries will be written or NULL
   \param new_lines  pointer to where number of new lines/boundaries is stored or NULL

   \return number of merged lines/boundaries
 */

int Vect_merge_lines(struct Map_info *Map, int type, int *new_lines,
		     struct Map_info *Err)
{
    int line, nlines, i, first, last, next_line, curr_line;
    int merged = 0, newl = 0;
    int next_node, direction, node_n_lines, ltype, lines_type;
    struct Plus_head *Plus;
    struct ilist *List;
    struct line_pnts *MPoints, *Points;
    struct line_cats *MCats, *Cats;
    struct P_line *Line;

    type &= GV_LINES;

    if (!(type & GV_LINES)) {
	G_warning
	    ("Merging is done with lines or boundaries only, not with other types");
	return 0;
    }

    Plus = &(Map->plus);
    nlines = Vect_get_num_lines(Map);

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();
    MPoints = Vect_new_line_struct();
    MCats = Vect_new_cats_struct();
    List = Vect_new_list();

    for (line = 1; line <= nlines; line++) {
	G_percent(line, nlines, 2);

	if (!Vect_line_alive(Map, line))
	    continue;

	Line = Plus->Line[line];
	ltype = Line->type;

	if (!(ltype & type))
	    continue;
	    
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
		if ((Plus->Line[abs(curr_line)]->type & GV_LINES))
		    lines_type++;
		if ((Plus->Line[abs(curr_line)]->type == ltype)) {
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
		if ((Plus->Line[abs(curr_line)]->type & GV_LINES))
		    lines_type++;
		if ((Plus->Line[abs(curr_line)]->type == ltype)) {
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
	if (List->n_values > 1) {
	    G_debug(3, "merge %d lines", List->n_values);
	    Vect_reset_line(MPoints);

	    for (i = 0; i < List->n_values; i++) {
		Vect_reset_line(Points);
		Vect_read_line(Map, Points, Cats, abs(List->value[i]));
		direction = (List->value[i] < 0 ? GV_BACKWARD : GV_FORWARD);
		Vect_append_points(MPoints, Points, direction);
		MPoints->n_points--;
		if (Err) {
		    /* write out lines/boundaries to be merged */
		    Vect_write_line(Err, ltype, Points, Cats);
		}
		Vect_delete_line(Map, abs(List->value[i]));
	    }
	    MPoints->n_points++;
	    Vect_write_line(Map, ltype, MPoints, MCats);
	    merged += List->n_values;
	    newl++;
	}

	/* nlines = Vect_get_num_lines(Map); */
    }

    G_verbose_message(_("%d boundaries merged"), merged);
    G_verbose_message(_("%d new boundaries"), newl);

    if (new_lines)
	*new_lines = newl;

    Vect_destroy_line_struct(Points);
    Vect_destroy_cats_struct(Cats);
    Vect_destroy_line_struct(MPoints);
    Vect_destroy_cats_struct(MCats);
    Vect_destroy_list(List);

    return merged;
}
