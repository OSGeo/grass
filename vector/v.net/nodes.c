#include <stdlib.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>
#include "proto.h"

int nodes(struct Map_info *In, struct Map_info *Out, int add_cats, int nfield)
{
    int i, node, nnodes, line, nlines, count, type, add_point;
    double x, y, z;

    struct line_pnts *Points, *Pout;
    struct line_cats *Cats;
    struct boxlist *List;
    struct bound_box box;

    int cat;

    Points = Vect_new_line_struct();
    Pout = Vect_new_line_struct();

    Cats = Vect_new_cats_struct();
    List = Vect_new_boxlist(0);

    /* Rewrite all primitives to output file */
    cat = 0;
    while ((type = Vect_read_next_line(In, Points, Cats)) >= 0) {
	if (type == GV_POINT) {
	    /* Get max cat in input */
	    int j;

	    for (j = 0; j < Cats->n_cats; j++) {
		if (Cats->field[j] == nfield && Cats->cat[j] > cat) {
		    cat = Cats->cat[j];
		}
	    }
	}
	Vect_write_line(Out, type, Points, Cats);
    }
    cat++;

    /* Go through all nodes in old map and write a new point if missing */
    nnodes = Vect_get_num_nodes(In);
    count = 0;
    for (node = 1; node <= nnodes; node++) {

	nlines = Vect_get_node_n_lines(In, node);
	add_point = 0;
	for (i = 0; i < nlines; i++) {
	    line = abs(Vect_get_node_line(In, node, i));
	    type = Vect_read_line(In, NULL, NULL, line);
	    if (type & GV_LINES) {
		add_point = 1;
		break;
	    }
	}

	if (add_point) {
	    Vect_get_node_coor(In, node, &x, &y, &z);
	    box.E = box.W = x;
	    box.N = box.S = y;
	    box.T = box.B = z;
	    Vect_select_lines_by_box(In, &box, GV_POINT, List);
	    add_point = List->n_values == 0;
	}

	if (add_point) {	/* Write new point */
	    Vect_reset_line(Pout);
	    Vect_append_point(Pout, x, y, z);
	    Vect_reset_cats(Cats);
	    if (add_cats) {
		Vect_cat_set(Cats, nfield, cat++);
	    }
	    Vect_write_line(Out, GV_POINT, Pout, Cats);
	    count++;
	}
    }

    Vect_destroy_line_struct(Points);
    Vect_destroy_line_struct(Pout);
    Vect_destroy_cats_struct(Cats);
    Vect_destroy_boxlist(List);

    return count;
}
