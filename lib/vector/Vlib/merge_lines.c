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
    int line, nlines, i, c, first, last, next_line, curr_line;
    int merged = 0, newl = 0;
    int next_node, direction, node_n_lines, same_type;
    struct Plus_head *Plus;
    struct ilist *List;
    struct line_pnts *MPoints, *Points;
    struct line_cats *MCats, *Cats;
    struct P_line *Line;

    if ((type & GV_BOUNDARY) && (type & GV_LINE)) {
	G_warning
	    ("Merging is done only with either lines or boundaries, not both types at the same time");
	return 0;
    }
    if (!(type & GV_BOUNDARY) && !(type & GV_LINE)) {
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

	if (!(Line->type & type))
	    continue;

	/* special cases:
	 *  - loop back to start boundary via several other boundaries
	 *  - one boundary forming closed loop
	 *  - node with 3 entries but only 2 boundaries, one of them connecting twice,
	 *    the other one must then be topologically incorrect in case of boundary */

	/* go backward as long as there is only one other line/boundary at the current node */
	G_debug(3, "go backward");
	next_node = Line->N1;
	first = -line;
	while (1) {
	    node_n_lines = Vect_get_node_n_lines(Map, next_node);
	    same_type = 0;
	    next_line = first;
	    for (i = 0; i < node_n_lines; i++) {
		curr_line = Vect_get_node_line(Map, next_node, i);
		if ((Plus->Line[abs(curr_line)]->type & type)) {
		    same_type++;
		    if (abs(curr_line) != abs(first))
			next_line = curr_line;
		}
	    }
	    if (same_type == 2 && abs(next_line) != abs(first) &&
		abs(next_line) != line) {
		first = next_line;

		if (first < 0)
		    next_node = Plus->Line[-first]->N1;
		else
		    next_node = Plus->Line[first]->N2;
	    }
	    else
		break;
	}

	/* go forward as long as there is only one other line/boundary at the current node */
	G_debug(3, "go forward");

	/* reverse direction */
	last = -first;

	if (last < 0)
	    next_node = Plus->Line[-last]->N1;
	else
	    next_node = Plus->Line[last]->N2;

	Vect_reset_list(List);
	while (1) {
	    Vect_list_append(List, last);
	    node_n_lines = Vect_get_node_n_lines(Map, next_node);
	    same_type = 0;
	    next_line = last;
	    for (i = 0; i < node_n_lines; i++) {
		curr_line = Vect_get_node_line(Map, next_node, i);
		if ((Plus->Line[abs(curr_line)]->type & type)) {
		    same_type++;
		    if (abs(curr_line) != abs(last))
			next_line = curr_line;
		}
	    }

	    if (same_type == 2 && abs(next_line) != abs(last) &&
		abs(next_line) != abs(first)) {
		last = next_line;

		if (last < 0)
		    next_node = Plus->Line[-last]->N1;
		else
		    next_node = Plus->Line[last]->N2;
	    }
	    else
		break;
	}

	/* merge lines */
	if (List->n_values > 1) {
	    G_debug(3, "merge %d lines", List->n_values);
	    Vect_reset_line(MPoints);
	    Vect_reset_cats(MCats);

	    for (i = 0; i < List->n_values; i++) {
		Vect_reset_line(Points);
		Vect_reset_cats(Cats);
		Vect_read_line(Map, Points, Cats, abs(List->value[i]));
		direction = (List->value[i] < 0 ? GV_BACKWARD : GV_FORWARD);
		Vect_append_points(MPoints, Points, direction);
		MPoints->n_points--;
		for (c = 0; c < Cats->n_cats; c++) {
		    Vect_cat_set(MCats, Cats->field[c], Cats->cat[c]);
		}
		if (Err) {
		    /* write out lines/boundaries to be merged */
		    Vect_write_line(Err, type, Points, Cats);
		}
		Vect_delete_line(Map, abs(List->value[i]));
	    }
	    MPoints->n_points++;
	    Vect_write_line(Map, type, MPoints, MCats);
	    merged += List->n_values;
	    newl++;
	}

	nlines = Vect_get_num_lines(Map);
    }

    G_verbose_message(_("%d boundaries merged"), merged);
    G_verbose_message(_("%d new boundaries"), newl);

    if (new_lines)
	*new_lines = newl;

    return merged;
}
