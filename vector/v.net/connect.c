
/***************************************************************
 *
 * MODULE:       v.net
 * 
 * AUTHOR(S):    Martin Landa <landa.martin gmail.com>
 *               
 * PURPOSE:      Network maintenance
 *               
 * COPYRIGHT:    (C) 2007 by the GRASS Development Team
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

/**
 * \brief Consolidate network arcs (edge) based on given point vector map (nodes)
 *
 * If there is no connection between network edge and point, new edge
 * is added, the line broken, and new point added to nfield layer
 *
 * \param[in] In,Points input vector maps
 * \param[in] Out output vector map
 * \param[in] nfield nodes layer 
 * \param[in] thresh threshold value to find neareast line
 *
 * \return number of new arcs
 */
int connect_arcs(struct Map_info *In, struct Map_info *Pnts,
		 struct Map_info *Out, int nfield, double thresh)
{
    int narcs;
    int type, line, seg, i, ltype;
    double px, py, pz, spdist, dist;

    struct line_pnts *Points, *Pline, *Pout;
    struct line_cats *Cats, *Cline;

    narcs = 0;

    Points = Vect_new_line_struct();
    Pline = Vect_new_line_struct();
    Pout = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();
    Cline = Vect_new_cats_struct();

    /* rewrite all primitives to output file */
    Vect_copy_map_lines(In, Out);
    Vect_build_partial(Out, GV_BUILD_BASE);

    /* go thorough all points in point map and write a new arcs if missing */
    while ((type = Vect_read_next_line(Pnts, Points, Cats)) >= 0) {
	if (type != GV_POINT)
	    continue;

	/* find the nearest line in given threshold */
	line = Vect_find_line(Out,
			      Points->x[0], Points->y[0], Points->z[0],
			      GV_LINES, thresh, WITHOUT_Z, 0);

	if (line < 1 || !Vect_line_alive(Out, line))
	    continue;

	ltype = Vect_read_line(Out, Pline, Cline, line);

	/* find point on the line */
	seg = Vect_line_distance(Pline,
				 Points->x[0], Points->y[0], Points->z[0],
				 WITHOUT_Z, &px, &py, &pz, &dist, &spdist,
				 NULL);

	/* break the line */
	Vect_reset_line(Pout);
	for (i = 0; i < seg; i++) {
	    Vect_append_point(Pout, Pline->x[i], Pline->y[i], Pline->z[i]);
	}
	Vect_append_point(Pout, px, py, pz);
	Vect_rewrite_line(Out, line, ltype, Pout, Cline);

	Vect_reset_line(Pout);
	Vect_append_point(Pout, px, py, pz);
	for (i = seg; i < Pline->n_points; i++) {
	    Vect_append_point(Pout, Pline->x[i], Pline->y[i], Pline->z[i]);
	}
	Vect_write_line(Out, ltype, Pout, Cline);

	if (dist > 0.0) {
	    /* write new arc */
	    Vect_reset_line(Pout);
	    Vect_append_point(Pout, px, py, pz);
	    Vect_append_point(Pout, Points->x[0], Points->y[0], Points->z[0]);
	    Vect_write_line(Out, ltype, Pout, Cline);
	}

	/* add points to 'nfield' layer */
	for (i = 0; i < Cats->n_cats; i++) {
	    Cats->field[i] = nfield;	/* all points to 'nfield' layer */
	}
	Vect_write_line(Out, type, Points, Cats);

	narcs++;
    }

    Vect_destroy_line_struct(Points);
    Vect_destroy_line_struct(Pline);
    Vect_destroy_line_struct(Pout);
    Vect_destroy_cats_struct(Cats);
    Vect_destroy_cats_struct(Cline);

    return narcs;
}
