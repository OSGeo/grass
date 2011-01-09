/*!
  \file lib/vector/vedit/snap.c

  \brief Vedit library - snapping
  
  (C) 2007-2008 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2).  Read the file COPYING that comes with GRASS for details.
  
  \author Martin Landa <landa.martin gmail.com>
*/

#include <grass/vedit.h>

/*!
  \brief Snap given point to the nearest primitive
  
  \param Map pointer to Map_info
  \param line line to be excluded (point on line)
  \param x,y,z point on line to be snapped
  \param thresh snapping threshold (>0)
  \param vertex snap also to vertex (non-zero)
  
  \return 1 snapped
  \return 0 not snapped
*/
int Vedit_snap_point(struct Map_info *Map,
		     int line, double *x, double *y, double *z, double thresh,
		     int vertex)
{
    struct line_pnts *Points;

    int i, snapped;
    int line2snap, mindist_idx;
    double dist, mindist;

    snapped = 0;
    mindist_idx = -1;
    mindist = thresh;

    Points = Vect_new_line_struct();

    line2snap = Vect_find_line(Map, *x, *y, *z, -1, thresh, WITHOUT_Z, line);

    if (line2snap > 0) {
	Vect_read_line(Map, Points, NULL, line2snap);

	if (!Vect_line_alive(Map, line2snap)) {
	    Vect_destroy_line_struct(Points);
	    return snapped;
	}

	for (i = 0; i < Points->n_points; i++) {
	    if (i > 0 && i < Points->n_points - 1)
		if (!vertex)
		    continue;
	    dist = Vect_points_distance(*x, *y, *z,
					Points->x[i], Points->y[i],
					Points->z[i], WITHOUT_Z);

	    if (mindist >= dist) {
		mindist = dist;
		mindist_idx = i;
	    }
	}

	if (mindist_idx > -1) {
	    *x = Points->x[mindist_idx];
	    *y = Points->y[mindist_idx];
	    *z = Points->z[mindist_idx];
	    snapped = 1;
	}
    }

    G_debug(3, "Vedit_snap_point(): map=%s, line2snap=%d, snapped=%d",
	    Map->name, line2snap, snapped);

    Vect_destroy_line_struct(Points);

    return snapped;
}

/*!
  \brief Snap selected primitive to its nearest primitive
  
  \param Map pointer to Map_info
  \param BgMap,nbgmaps list of background maps used for snapping
  \param line line id to be snapped (if already written, otherwise -1)
  \param Points line geometry
  \param layer layer number
  \param thresh threshold value used for snapping (>0)
  \param to_vertex allow snapping also to vertex
  
  \return 1 line snapped
  \return 0 line not snapped
  \return -1 line is dead (if 'line' is > 0)
*/
int Vedit_snap_line(struct Map_info *Map, struct Map_info **BgMap,
		    int nbgmaps, int line, struct line_pnts *Points,
		    double thresh, int to_vertex)
{
    int i, npoints, node, rewrite;
    double *x, *y, *z;

    struct line_cats *Cats;

    Cats = Vect_new_cats_struct();

    G_debug(3, "Vedit_snap_line(): thresh=%g, to_vertex=%d", thresh,
	    to_vertex);

    if (line > 0 && !Vect_line_alive(Map, line))
	return -1;

    npoints = Points->n_points;
    x = Points->x;
    y = Points->y;
    z = Points->z;

    rewrite = 0;
    for (node = 0; node < npoints; node++) {
	if ((node > 0 && node < npoints - 1) && !to_vertex)
	    continue;

	if (Vedit_snap_point(Map, line, &x[node], &y[node], &z[node], thresh,
			     to_vertex)) {
	    rewrite = 1;
	}
	else {
	    /* check also background maps */
	    for (i = 0; i < nbgmaps; i++) {
		if (Vedit_snap_point
		    (BgMap[i], -1, &x[node], &y[node], &z[node], thresh,
		     to_vertex)) {
		    rewrite = 1;
		    break;	/* snapped, don't continue */
		}
	    }
	}
    }				/* for each line vertex */

    /* close boundaries or lines */
    if (!rewrite &&
	Vect_points_distance(x[0], y[0], z[0],
			     x[npoints - 1], y[npoints - 1], z[npoints - 1],
			     WITHOUT_Z) <= thresh) {
	x[npoints - 1] = x[0];
	y[npoints - 1] = y[0];
	z[npoints - 1] = z[0];

	rewrite = 1;
    }

    G_debug(3, "Vedit_snap_line(): line=%d, snapped=%d", line, rewrite);

    Vect_destroy_cats_struct(Cats);

    return rewrite;
}

/*!
  \brief Snap lines/boundaries
  
  \param Map pointer to Map_info
  \param BgMap,nbgmaps list of background maps used for snapping
  \param List list of lines to be snapped
  \param layer layer number
  \param thresh threshold value used for snapping (>0)
  \param to_vertex allow snapping also to vertex
  
  \return number of snapped lines
  \return -1 on error
*/
int Vedit_snap_lines(struct Map_info *Map, struct Map_info **BgMap,
		     int nbgmaps, struct ilist *List, double thresh,
		     int to_vertex)
{
    int i, line, type;
    int nlines_modified = 0;

    struct line_pnts *Points;
    struct line_cats *Cats;

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    for (i = 0; i < List->n_values; i++) {
	line = List->value[i];
	type = Vect_read_line(Map, Points, Cats, line);

	if (!(type & (GV_POINT | GV_LINES))) {
	    continue;
	}

	if (Vedit_snap_line(Map, BgMap, nbgmaps,
			    line, Points, thresh, to_vertex) == 1) {
	    if (Vect_rewrite_line(Map, line, type, Points, Cats) < 0) {
		return -1;
	    }

	    nlines_modified++;
	}
    }

    Vect_destroy_line_struct(Points);
    Vect_destroy_cats_struct(Cats);

    return nlines_modified;
}
