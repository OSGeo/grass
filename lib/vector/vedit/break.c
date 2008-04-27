/**
   \file break.c

   \brief Vedit library - split/break lines

   This program is free software under the
   GNU General Public License (>=v2).
   Read the file COPYING that comes with GRASS
   for details.

   \author (C) 2007-2008 by the GRASS Development Team
   Martin Landa <landa.martin gmail.com>

   \date 2007-2008
*/

#include <math.h>
#include <grass/vedit.h>

/**
   \brief Split selected lines on given position
   
   \param[in] Map vector map
   \param[in] List list of selected lines
   \param[in] coord points location
   \param[in] List_updated list of rewritten features (or NULL)

   \return number of modified lines
   \return -1 on error
 */
int Vedit_split_lines(struct Map_info *Map, struct ilist *List,
		      struct line_pnts *coord, double thresh,
		      struct ilist *List_updated)
{
    int i, j, l;
    int type, line, seg, newline;
    int nlines_modified;
    double px, py, spdist, lpdist, dist;
    double *x, *y, *z;

    struct line_pnts *Points, *Points2;
    struct line_cats *Cats;
    struct ilist *List_in_box;

    nlines_modified = 0;

    Points = Vect_new_line_struct();
    Points2 = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();
    List_in_box = Vect_new_list();

    for (i = 0; i < List -> n_values; i++) {
	line = List -> value[i];
	
	if (!Vect_line_alive (Map, line))
	    continue;

        type = Vect_read_line(Map, Points, Cats, line);

	if (!(type & GV_LINES))
	    continue;

	x = Points -> x;
	y = Points -> y;
	z = Points -> z;

	for (j = 0; j < coord -> n_points; j++) {
	    seg = Vect_line_distance (Points, coord->x[j], coord->y[j], coord->z[j],
				      WITHOUT_Z,
				      &px, &py, NULL,
				      &dist, &spdist, &lpdist);

	    if (dist > thresh) {
		continue;
	    }

	    G_debug (3, "Vedit_split_lines(): line=%d, x=%f, y=%f, px=%f, py=%f, seg=%d, "
		     "dist=%f, spdist=%f, lpdist=%f",
		     line, coord->x[j], coord->y[j], px, py, seg, dist, spdist, lpdist);

	    if (spdist <= 0.0 ||
		spdist >= Vect_line_length(Points))
		continue;
	    
	    G_debug (3, "Vedit_split_lines(): line=%d", line);

	    /* copy first line part */
	    for (l = 0; l < seg; l++) {
		Vect_append_point(Points2,
				  x[l], y[l], z[l]);
	    }
	    
	    /* add last vertex */
	    Vect_append_point(Points2, px, py, 0.0);
	    
	    /* rewrite the line */
	    newline = Vect_rewrite_line (Map, line, type, Points2, Cats);
	    if (newline < 0) {
		return -1;
	    }
   	    if (List_updated)
		Vect_list_append (List_updated, newline);
	    Vect_reset_line (Points2);
	    
	    /* add given vertex */
	    Vect_append_point(Points2, px, py, 0.0);
	    
	    /* copy second line part */
	    for (l = seg; l < Points->n_points; l++) {
		Vect_append_point(Points2, 
				  x[l], y[l], z[l]);
	    }
	    
	    /* rewrite the line */
	    newline = Vect_write_line (Map, type, Points2, Cats);
	    if (newline < 0) {
		return -1;
	    }
   	    if (List_updated)
		Vect_list_append (List_updated, newline);
	    
	    nlines_modified++;
	} /* for each bounding box */
    } /* for each selected line */

    Vect_destroy_line_struct(Points);
    Vect_destroy_line_struct(Points2);
    Vect_destroy_cats_struct(Cats);
    Vect_destroy_list (List_in_box);

    return nlines_modified;
}

/**
   \brief Connect *two* lines
 
   The first line from the list is connected to the second one
   if necessary the second line is broken
 
   \			     \
   id1  \           ->	      \
                               \
   id2 ---------           ---------
 
   \param[in] Map vector map
   \param[in] List list of selected lines
   \param[in] thresh threshold value for connect

   \return  1 lines connected
   \return  0 lines not connected
   \return -1 on error
 */
int Vedit_connect_lines (struct Map_info *Map, struct ilist *List,
			 double thresh)
{
    int nlines_modified, newline, intersection, linep, nlines_to_modify;
    int i, idx, pnt_idx[2];
    int line[2], type[2], seg[2];
    double px[2], py[2], pz[2], dist[2], spdist[2], lpdist[2];
    double nx[2], ny[2], nz[2];
    char   n_on_line[2], connected;
    double angle, dseg, dist_connect;

    struct ilist *List_break, *List_updated;

    struct line_pnts *Points[2];
    struct line_cats *Cats[2];

    nlines_modified  = 0;
    nlines_to_modify = 2; /* modify *two* lines at the time */

    Points[0] = Points[1] = NULL;
    Cats[0]   = Cats[1]   = NULL;

    List_break = Vect_new_list ();
    List_updated = Vect_new_list();

    for (linep = 0; linep < List -> n_values; linep += 2) {
	connected = 0;
	/* read the lines */
	for (i = 0; i < nlines_to_modify; i++) {
	    line[i] = List -> value[linep + i];

	    if (!Vect_line_alive (Map, line[i])) {
		line[i] = -1;
		break;
	    }
	    if (!Points[i])
		Points[i] = Vect_new_line_struct();
	    if (!Cats[i])
		Cats[i] = Vect_new_cats_struct();

	    type[i] = Vect_read_line(Map, Points[i], Cats[i], line[i]);

	    if (!(type[i] & GV_LINES)) {
		line[i] = -1;
		break;
	    }
	}

	if (line[0] == -1 || line[1] == -1)
	    continue;

	/* check lines intersection 
	   if (Vect_line_check_intersection (Points[0], Points[1], WITHOUT_Z))
	   return 0;
	*/

	/* find node to connect */
	for (i = 0; i < nlines_to_modify; i++) {
	    if (i == 0)
		idx = 0;
	    else
		idx = Points[0] -> n_points - 1;

	    seg[i] = Vect_line_distance (Points[1], Points[0]->x[idx], Points[0]->y[idx], Points[0]->z[idx],
					 WITHOUT_Z,
					 &px[i], &py[i], &pz[i],
					 &dist[i], &spdist[i], &lpdist[i]);
	}

	idx        = (dist[0] <= dist[1]) ? 0 : 1;
	pnt_idx[0] = (idx == 0) ? 0 : Points[0] -> n_points - 1; /* which node to connect */

	angle =
	    M_PI / 2 -
	    acos (fabs (Points[0] -> x[0] - Points[0] -> x[Points[0] -> n_points - 1]) /
		  Vect_line_length (Points[0])) -
	    asin (fabs (Points[1] -> y[seg[idx] - 1] - py[idx]) / spdist[idx]);
	
	dseg = dist[idx] * tan (angle);

	/* compute connect points */
	for (i = 0; i < nlines_to_modify; i++) {
	    if (0 == Vect_point_on_line (Points[1], lpdist[idx] + dseg, &nx[i], &ny[i], &nz[i], NULL, NULL)) {
		n_on_line[i] = 0;
	    }
	    else {
		n_on_line[i] = 1;
	    }
	    dseg *= -1.0;
	}
    
	if (!n_on_line[0] || !n_on_line[1]) {
	    G_warning (_("Unable to connect lines %d,%d"), line[0], line[1]);
	    continue;
	}

	/* choose connect point */
	if (idx == 0)
	    pnt_idx[1] = Points[0] -> n_points - 1;
	else
	    pnt_idx[1] = 0;
	
	if (Vect_points_distance (nx[0], ny[0], nz[0],
				  Points[0] -> x[pnt_idx[1]], Points[0] -> y[pnt_idx[1]],
				  Points[0] -> z[pnt_idx[1]],
				  WITHOUT_Z) >
	    Vect_points_distance (nx[1], ny[1], nz[1],
				  Points[0] -> x[pnt_idx[1]], Points[0] -> y[pnt_idx[1]],
				  Points[0] -> z[pnt_idx[1]],
				  WITHOUT_Z))
	    pnt_idx[1] = 0;
	else
	    pnt_idx[1] = 1;
	
	dist_connect = Vect_points_distance (nx[pnt_idx[1]], ny[pnt_idx[1]], nz[pnt_idx[1]],
					     Points[0] -> x[pnt_idx[0]], Points[0] -> y[pnt_idx[0]],
					     Points[0] -> z[pnt_idx[0]],
					     WITHOUT_Z);

	G_debug (3, "Vedit_connect_lines(): dist=%f/%f -> pnt_idx=%d -> "
		 "x=%f, y=%f / dist_connect=%f (thresh=%f)",
		 dist[0], dist[1], pnt_idx[0], nx[pnt_idx[1]], ny[pnt_idx[1]], dist_connect, thresh);

	if (thresh >= 0.0 && dist_connect > thresh) {
	    continue;
	}
    
	/* modify the first line */
	Vect_reset_list (List_updated);
	intersection = Vect_line_check_intersection (Points[0], Points[1], WITHOUT_Z);

	if (!intersection) {
	    if (pnt_idx[0] == 0)
		Vect_line_insert_point (Points[0], 0, nx[pnt_idx[1]], ny[pnt_idx[1]], 0.0);
	    else
		Vect_append_point (Points[0], nx[pnt_idx[1]], ny[pnt_idx[1]], 0.0);
	    
	    /* rewrite the first line */
	    newline = Vect_rewrite_line (Map, line[0], type[0], Points[0], Cats[0]);
	    if (newline < 0) {
		return -1;
	    }
	    Vect_list_append (List_updated, newline);
	    connected = 1;
	}
	else
	    Vect_list_append (List_updated, line[0]);
	
	/* break the second line */
	Vect_reset_list (List_break);
	if (!intersection ||
	    (intersection &&
	     Vect_points_distance (nx[pnt_idx[1]], ny[pnt_idx[1]], nz[pnt_idx[1]],
				   Points[0] -> x[pnt_idx[0]], Points[0] -> y[pnt_idx[0]],
				   Points[0] -> z[pnt_idx[0]],
				   WITHOUT_Z) <= 0.0)) {
	    Vect_list_append (List_break, line[1]);
	    struct line_pnts *coord = Vect_new_line_struct();
	    Vect_append_point(coord, nx[pnt_idx[1]], ny[pnt_idx[1]], nz[pnt_idx[1]]);
	    Vedit_split_lines(Map, List_break, 
			      coord, 1e-1, 
			      List_updated);
	    Vect_destroy_line_struct(coord);
	    /* snap lines */
	    Vect_snap_lines_list (Map, List_updated, 1e-1, NULL, NULL);
	    connected = 1;
	}
	
	if (connected)
	    nlines_modified += 2;

    } /* for each line pair */

    /* destroy structures */
    for (i = 0; i < nlines_to_modify; i++) {
	if (Points[i])
	    Vect_destroy_line_struct(Points[i]);
	if (Cats[i])
	    Vect_destroy_cats_struct(Cats[i]);
    }
    Vect_destroy_list (List_break);
    Vect_destroy_list (List_updated);

    return nlines_modified == 2 ? 1 : 0;
}
