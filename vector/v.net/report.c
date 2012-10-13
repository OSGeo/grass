#include <stdlib.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>
#include "proto.h"

int report(struct Map_info *In, int afield, int nfield, int action)
{
    int i, j, line, nlines, ltype, node, nnodes;
    int cat_line, cat_node[2];

    struct line_cats *Cats, *Cats2;
    struct line_pnts *Points;
    struct bound_box box;

    double x, y, z;

    Cats = Vect_new_cats_struct();
    Cats2 = Vect_new_cats_struct();
    Points = Vect_new_line_struct();

    nlines = Vect_get_num_lines(In);

    if (action == TOOL_REPORT) {
	struct boxlist *List;

	List = Vect_new_boxlist(0);

	/* For all lines find categories for points on nodes */
	for (i = 1; i <= nlines; i++) {
	    ltype = Vect_read_line(In, NULL, Cats, i);
	    if (!(ltype & GV_LINES))
		continue;

	    cat_line = 0;
	    if (!Vect_cat_get(Cats, afield, &cat_line))
		G_warning(_("Line %d has no category"), i);

	    cat_node[0] = cat_node[1] = -1;
	    for (j = 0; j < 2; j++) {
		if (j == 0)
		    Vect_get_line_nodes(In, i, &node, NULL);
		else
		    Vect_get_line_nodes(In, i, NULL, &node);

		Vect_get_node_coor(In, node, &x, &y, &z);

		box.E = box.W = x;
		box.N = box.S = y;
		box.T = box.B = z;
		Vect_select_lines_by_box(In, &box, GV_POINT, List);
		
		nnodes = List->n_values;
		if (nnodes > 0) {
		    line = List->id[nnodes - 1]; /* last in list */
		    Vect_read_line(In, NULL, Cats, line);
		    Vect_cat_get(Cats, nfield, &(cat_node[j]));
		}

		if (nnodes == 0) {
		    /* this is ok, not every node needs to be 
		     * represented by a point */
		    G_debug(4, "No point here: %g %g %.g line category: %d",
			      x, y, z, cat_line);
		}
		else if (nnodes > 1)
		    G_warning(_("%d points found: %g %g %g line category: %d"),
			      nnodes, x, y, z, cat_line);
	    }
	    fprintf(stdout, "%d %d %d\n", cat_line, cat_node[0], cat_node[1]);
	}
    }
    else {			/* node report */
	int elem, nelem, type, k, l;
	struct ilist *List;

	List = Vect_new_list();


	for (i = 1; i <= nlines; i++) {
	    
	    if (Vect_get_line_type(In, i) != GV_POINT)
		continue;

	    Vect_read_line(In, Points, Cats, i);

	    box.E = box.W = Points->x[0];
	    box.N = box.S = Points->y[0];
	    box.T = box.B = Points->z[0];
	    
	    nnodes = Vect_select_nodes_by_box(In, &box, List);
	    
	    if (nnodes > 1) {
		G_warning(_("Duplicate nodes at x=%g y=%g z=%g "),
			  Points->x[0], Points->y[0], Points->z[0]);
	    }
	    if (nnodes > 0) {
		node = List->value[0];
		nelem = Vect_get_node_n_lines(In, node);

		/* Loop through all cats of point */
		for (j = 0; j < Cats->n_cats; j++) {
		    if (Cats->field[j] == nfield) {
			int count = 0;

			fprintf(stdout, "%d ", Cats->cat[j]);

			/* Loop through all lines */
			for (k = 0; k < nelem; k++) {
			    elem = abs(Vect_get_node_line(In, node, k));
			    type = Vect_read_line(In, NULL, Cats2, elem);
			    if (!(type & GV_LINES))
				continue;

			    /* Loop through all cats of line */
			    for (l = 0; l < Cats2->n_cats; l++) {
				if (Cats2->field[l] == afield) {
				    if (count > 0)
					fprintf(stdout, ",");

				    fprintf(stdout, "%d", Cats2->cat[l]);
				    count++;
				}
			    }
			}
			fprintf(stdout, "\n");
		    }
		}
	    }
	}
    }

    return 0;
}
