
/***************************************************************
 *
 * MODULE:       v.net
 * 
 * AUTHOR(S):    Radim Blazek
 *               
 * PURPOSE:      Network maintenance 
 *               
 * COPYRIGHT:    (C) 2001 by the GRASS Development Team
 *
 *               This program is free software under the 
 *               GNU General Public License (>=v2). 
 *               Read the file COPYING that comes with GRASS
 *               for details.
 *
 **************************************************************/
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/Vect.h>
#include <grass/glocale.h>
#include "proto.h"

int report(struct Map_info *In, int afield, int nfield, int action)
{
    int i, j, k, line, ltype, nnodes;
    int cat_line, cat_node[2];
    struct line_cats *Cats, *Cats2;
    int node;
    double x, y, z;

    Cats = Vect_new_cats_struct();
    Cats2 = Vect_new_cats_struct();

    if (action == TOOL_REPORT) {
	/* For all lines find categories for points on nodes */
	for (i = 1; i <= Vect_get_num_lines(In); i++) {
	    ltype = Vect_read_line(In, NULL, Cats, i);
	    if (ltype != GV_LINE)
		continue;

	    cat_line = 0;
	    if (!Vect_cat_get(Cats, afield, &cat_line))
		G_warning(_("Line %d has no category"), i);

	    cat_node[0] = cat_node[1] = 0;
	    for (j = 0; j < 2; j++) {
		if (j == 0)
		    Vect_get_line_nodes(In, i, &node, NULL);
		else
		    Vect_get_line_nodes(In, i, NULL, &node);

		Vect_get_node_coor(In, node, &x, &y, &z);
		nnodes = 0;

		for (k = 0; k < Vect_get_node_n_lines(In, node); k++) {
		    line = abs(Vect_get_node_line(In, node, k));
		    ltype = Vect_read_line(In, NULL, Cats, line);
		    if (ltype != GV_POINT)
			continue;

		    Vect_cat_get(Cats, nfield, &(cat_node[j]));

		    nnodes++;
		}
		if (nnodes == 0)
		    G_warning(_("Point not found: %.3lf %.3lf %.3lf line category: %d"),
			      x, y, z, cat_line);
		else if (nnodes > 1)
		    G_warning(_("%d points found: %.3lf %.3lf %.3lf line category: %d"),
			      nnodes, x, y, z, cat_line);
	    }
	    fprintf(stdout, "%d %d %d\n", cat_line, cat_node[0], cat_node[1]);
	}
    }
    else {			/* node report */

	int nnodes, node;

	nnodes = Vect_get_num_nodes(In);

	for (node = 1; node <= nnodes; node++) {
	    int nelem, elem, type, i, j, k, l;

	    nelem = Vect_get_node_n_lines(In, node);

	    /* Loop through all points */
	    for (i = 0; i < nelem; i++) {
		elem = abs(Vect_get_node_line(In, node, i));
		type = Vect_read_line(In, NULL, Cats, elem);
		if (type != GV_POINT)
		    continue;

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
