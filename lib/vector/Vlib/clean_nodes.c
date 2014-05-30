/*!
   \file lib/vector/Vlib/clean_nodes.c

   \brief Vector library - Clean boundaries at nodes

   Higher level functions for reading/writing/manipulating vectors.

   (C) 2001-2009 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2).  Read the file COPYING that comes with GRASS for details.

   \author Radim Blazek
 */

#include <stdlib.h>
#include <grass/vector.h>
#include <grass/glocale.h>

/*!
   \brief Clean small angles at nodes.

   It may happen that even if the angle between 2 boundaries at node
   is very small, the calculated angle is 0 because of representation
   error. The map must be built at least on level GV_BUILD_BASE

   \param Map input map
   \param otype feature type
   \param[out] Err vector map where error line segments are written

   \return number of line modifications
 */
int
Vect_clean_small_angles_at_nodes(struct Map_info *Map, int otype,
				 struct Map_info *Err)
{
    int node, nnodes;
    int nmodif = 0;
    struct line_pnts *Points;
    struct line_cats *SCats, *LCats, *OCats;

    Points = Vect_new_line_struct();
    SCats = Vect_new_cats_struct();
    LCats = Vect_new_cats_struct();
    OCats = Vect_new_cats_struct();

    nnodes = Vect_get_num_nodes(Map);
    for (node = 1; node <= Vect_get_num_nodes(Map); node++) {
	int i, nlines;

	if (node <= nnodes)
	    G_percent(node, nnodes, 1);
	G_debug(3, "node = %d", node);
	if (!Vect_node_alive(Map, node))
	    continue;

	while (1) {
	    float angle1 = -100;
	    int line1 = -999;	/* value not important, just for debug */
	    int clean = 1;

	    nlines = Vect_get_node_n_lines(Map, node);
	    G_debug(3, "nlines = %d", nlines);

	    for (i = 0; i < nlines; i++) {
		struct P_line *Line;
		int line2;
		float angle2;

		line2 = Vect_get_node_line(Map, node, i);
		Line = Map->plus.Line[abs(line2)];
		if (!Line)
		    continue;
		G_debug(4, "  type = %d", Line->type);
		if (!(Line->type & (otype & GV_LINES)))
		    continue;

		angle2 = Vect_get_node_line_angle(Map, node, i);
		if (angle2 == -9.0)
		    continue;	/* Degenerated line */

		G_debug(4, "  line1 = %d angle1 = %e line2 = %d angle2 = %e",
			line1, angle1, line2, angle2);

		if (angle2 == angle1) {
		    int j;
		    double length1, length2;
		    int short_line;	/* line with shorter end segment */
		    int long_line;	/* line with longer end segment */
		    int new_short_line = 0;	/* line number of short line after rewrite */
		    int short_type, long_type, type;
		    double x, y, z, nx, ny, nz;

		    G_debug(4, "  identical angles -> clean");

		    /* Length of end segments for both lines */
		    Vect_read_line(Map, Points, NULL, abs(line1));
		    if (line1 > 0) {
			length1 =
			    Vect_points_distance(Points->x[0], Points->y[0],
						 0.0, Points->x[1],
						 Points->y[1], 0.0, 0);
		    }
		    else {
			int np;

			np = Points->n_points;
			length1 =
			    Vect_points_distance(Points->x[np - 1],
						 Points->y[np - 1], 0.0,
						 Points->x[np - 2],
						 Points->y[np - 2], 0.0, 0);
		    }

		    Vect_read_line(Map, Points, NULL, abs(line2));
		    if (line2 > 0) {
			length2 =
			    Vect_points_distance(Points->x[0], Points->y[0],
						 0.0, Points->x[1],
						 Points->y[1], 0.0, 0);
		    }
		    else {
			int np;

			np = Points->n_points;
			length2 =
			    Vect_points_distance(Points->x[np - 1],
						 Points->y[np - 1], 0.0,
						 Points->x[np - 2],
						 Points->y[np - 2], 0.0, 0);
		    }

		    G_debug(4, "  length1 = %f length2 = %f", length1,
			    length2);

		    if (length1 < length2) {
			short_line = line1;
			long_line = line2;
		    }
		    else {
			short_line = line2;
			long_line = line1;
		    }

		    /* Remove end segment from short_line */
		    short_type =
			Vect_read_line(Map, Points, SCats, abs(short_line));

		    if (short_line > 0) {
			x = Points->x[1];
			y = Points->y[1];
			z = Points->z[1];
			Vect_line_delete_point(Points, 0);	/* first */
		    }
		    else {
			x = Points->x[Points->n_points - 2];
			y = Points->y[Points->n_points - 2];
			z = Points->z[Points->n_points - 2];
			Vect_line_delete_point(Points, Points->n_points - 1);	/* last */
		    }

		    /* It may happen that it is one line: node could be deleted,
		     * in that case we have to read the node coords first */
		    Vect_get_node_coor(Map, node, &nx, &ny, &nz);

		    if (Points->n_points > 1) {
			new_short_line =
			    Vect_rewrite_line(Map, abs(short_line),
					      short_type, Points, SCats);
		    }
		    else {
			Vect_delete_line(Map, abs(short_line));
		    }

		    /* It may happen that it is one line, in that case we have to take the new
		     * short line as long line, orientation is not changed */
		    if (abs(line1) == abs(line2)) {
			if (long_line > 0)
			    long_line = new_short_line;
			else
			    long_line = -new_short_line;
		    }

		    /* Add new line (must be before rewrite of long_line otherwise node could be deleted) */
		    long_type =
			Vect_read_line(Map, NULL, LCats, abs(long_line));

		    Vect_reset_cats(OCats);
		    for (j = 0; j < SCats->n_cats; j++) {
			Vect_cat_set(OCats, SCats->field[j], SCats->cat[j]);
		    }
		    for (j = 0; j < LCats->n_cats; j++) {
			Vect_cat_set(OCats, LCats->field[j], LCats->cat[j]);
		    }

		    if (long_type == GV_BOUNDARY || short_type == GV_BOUNDARY) {
			type = GV_BOUNDARY;
		    }
		    else {
			type = GV_LINE;
		    }

		    Vect_reset_line(Points);
		    Vect_append_point(Points, nx, ny, nz);
		    Vect_append_point(Points, x, y, z);
		    Vect_write_line(Map, type, Points, OCats);

		    if (Err) {
			Vect_write_line(Err, type, Points, OCats);
		    }

		    /* Snap long_line to the new short_line end */
		    long_type =
			Vect_read_line(Map, Points, LCats, abs(long_line));
		    if (long_line > 0) {
			Points->x[0] = x;
			Points->y[0] = y;
			Points->z[0] = z;
		    }
		    else {
			Points->x[Points->n_points - 1] = x;
			Points->y[Points->n_points - 1] = y;
			Points->z[Points->n_points - 1] = z;
		    }
		    Vect_line_prune(Points);
		    if (Points->n_points > 1) {
			Vect_rewrite_line(Map, abs(long_line), long_type,
					  Points, LCats);
		    }
		    else {
			Vect_delete_line(Map, abs(long_line));
		    }

		    nmodif += 3;
		    clean = 0;

		    break;
		}

		line1 = line2;
		angle1 = angle2;
	    }

	    if (clean || !Vect_node_alive(Map, node))
		break;
	}
    }
    G_verbose_message(_("Modifications: %d"), nmodif);

    return (nmodif);
}
