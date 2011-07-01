#include <stdlib.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>
#include "proto.h"

int nodes(struct Map_info *In, struct Map_info *Out, int add_cats, int nfield)
{
    int i, node, nnodes, line, nlines, count, type, found;
    double x, y, z;

    struct line_pnts *Points, *Pout;
    struct line_cats *Cats;

    int cat;

    Points = Vect_new_line_struct();
    Pout = Vect_new_line_struct();

    Cats = Vect_new_cats_struct();

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
	int has_lines = 0;

	nlines = Vect_get_node_n_lines(In, node);
	found = 0;
	for (i = 0; i < nlines; i++) {
	    line = abs(Vect_get_node_line(In, node, i));
	    type = Vect_read_line(In, NULL, NULL, line);
	    if (type == GV_POINT) {
		found = 1;
	    }
	    if (type & GV_LINES) {
		has_lines = 1;
	    }
	}
	if (has_lines && !found) {	/* Write new point */
	    Vect_reset_line(Pout);
	    Vect_get_node_coor(In, node, &x, &y, &z);
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

    return count;
}
