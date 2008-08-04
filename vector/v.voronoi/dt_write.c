#include <stdio.h>
#include <math.h>
#include <float.h>
#include <grass/gis.h>
#include <grass/Vect.h>
#include <grass/display.h>
#include "sw_defs.h"
#include "defs.h"
#include "write.h"

int write_triple(struct Site *s1, struct Site *s2, struct Site *s3)
{
    int i;
    int node;
    static struct line_pnts *Points = NULL;
    static struct line_cats *Cats = NULL;
    struct Site *sa, *sb;


    if (!Points) {
	Points = Vect_new_line_struct();
	Cats = Vect_new_cats_struct();
    }

    if (triangulate) {
	for (i = 0; i < 3; i++) {
	    switch (i) {
	    case 0:
		sa = s1;
		sb = s2;
		break;
	    case 1:
		sa = s2;
		sb = s3;
		break;
	    case 2:
		sa = s3;
		sb = s1;
		break;
	    }

	    /* Look if the line already exists */
	    node =
		Vect_find_node(&Out, sa->coord.x, sa->coord.y, 0.0, 0.0, 0);

	    if (node > 0) {	/* node found */
		int j, nlines;
		int found = 0;
		double x, y, z;

		nlines = Vect_get_node_n_lines(&Out, node);

		for (j = 0; j < nlines; j++) {
		    int line, node2;

		    line = Vect_get_node_line(&Out, node, j);

		    if (line > 0)
			Vect_get_line_nodes(&Out, line, NULL, &node2);
		    else
			Vect_get_line_nodes(&Out, abs(line), &node2, NULL);

		    Vect_get_node_coor(&Out, node2, &x, &y, &z);

		    if (x == sb->coord.x && y == sb->coord.y) {
			found = 1;
			break;
		    }
		}

		if (found)
		    continue;	/* next segment */
	    }

	    /* Not found, write it */
	    Vect_reset_line(Points);
	    if (mode3d) {
		G_debug(3, "sa->coord.z: %f", sa->coord.z);
		Vect_append_point(Points, sa->coord.x, sa->coord.y,
				  sa->coord.z);
		Vect_append_point(Points, sb->coord.x, sb->coord.y,
				  sb->coord.z);
	    }
	    else {
		Vect_append_point(Points, sa->coord.x, sa->coord.y, 0.0);
		Vect_append_point(Points, sb->coord.x, sb->coord.y, 0.0);
	    }
	    Vect_write_line(&Out, Type, Points, Cats);
	}
    }

    return 0;
}
