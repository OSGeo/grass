/*!
  \file lib/vector/vedit/break.c

  \brief Vedit library - split, break, connect lines
  
  (C) 2007-2008 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2).  Read the file COPYING that comes with GRASS for details.
  
  \author Martin Landa <landa.martin gmail.com>
*/

#include <math.h>
#include <grass/vedit.h>

static int connect_lines(struct Map_info *, int, int, int,
			 double, struct ilist *);

/*!
  \brief Split selected lines on given position
  
  \param Map pointer to Map_info
  \param List list of selected lines
  \param coord points location
  \param thresh threshold
  \param[out] List_updated list of rewritten features (or NULL)
  
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

    for (i = 0; i < List->n_values; i++) {
	line = List->value[i];

	if (!Vect_line_alive(Map, line))
	    continue;

	type = Vect_read_line(Map, Points, Cats, line);

	if (!(type & GV_LINES))
	    continue;

	x = Points->x;
	y = Points->y;
	z = Points->z;

	for (j = 0; j < coord->n_points; j++) {
	    seg =
		Vect_line_distance(Points, coord->x[j], coord->y[j],
				   coord->z[j], WITHOUT_Z, &px, &py, NULL,
				   &dist, &spdist, &lpdist);

	    if (dist > thresh) {
		continue;
	    }

	    G_debug(3, "Vedit_split_lines(): line=%d, x=%f, y=%f, px=%f, py=%f, seg=%d, "
		    "dist=%f, spdist=%f, lpdist=%f", line, coord->x[j],
		    coord->y[j], px, py, seg, dist, spdist, lpdist);
	    
	    if (spdist <= 0.0 || spdist >= Vect_line_length(Points))
		continue;

	    G_debug(3, "Vedit_split_lines(): line=%d", line);

	    /* copy first line part */
	    Vect_reset_line(Points2);
	    for (l = 0; l < seg; l++) {
		Vect_append_point(Points2, x[l], y[l], z[l]);
	    }

	    /* add last vertex */
	    Vect_append_point(Points2, px, py, 0.0);

	    /* rewrite the line */
            if (j == 0)
                newline = Vect_rewrite_line(Map, line, type, Points2, Cats);
            else
                newline = Vect_write_line(Map, type, Points2, Cats);
	    if (newline < 0) {
		return -1;
	    }
	    if (List_updated)
		Vect_list_append(List_updated, newline);
	    Vect_reset_line(Points2);

	    /* add given vertex */
	    Vect_append_point(Points2, px, py, 0.0);

	    /* copy second line part */
	    for (l = seg; l < Points->n_points; l++) {
		Vect_append_point(Points2, x[l], y[l], z[l]);
	    }

	    /* rewrite the line */
	    newline = Vect_write_line(Map, type, Points2, Cats);
	    if (newline < 0) {
		return -1;
	    }
	    if (List_updated)
		Vect_list_append(List_updated, newline);

	    nlines_modified++;
	}			/* for each bounding box */
    }				/* for each selected line */

    Vect_destroy_line_struct(Points);
    Vect_destroy_line_struct(Points2);
    Vect_destroy_cats_struct(Cats);
    Vect_destroy_list(List_in_box);

    return nlines_modified;
}

/*!
  \brief Connect lines in given threshold
  
  \code
        \	             \
   id1   \           ->	      \
                               \
   id2 ---------           -----+---
   \endcode

   If two lines are selected and <i>thresh</i> is -1, no limit is
   applied.

   \param Map pointer to Map_info
   \param List list of selected lines
   \param thresh threshold value

   \return number of modified lines
   \return -1 on error
 */
int Vedit_connect_lines(struct Map_info *Map, struct ilist *List,
			double thresh)
{
    int nlines_modified, connected;
    int i, j, node[2], n_nodes;
    int line, found;
    double x, y, z;

    struct ilist *List_exclude, *List_found;

    nlines_modified = 0;

    List_exclude = Vect_new_list();
    List_found = Vect_new_list();

    n_nodes = 2;

    /* collect lines to be modified */
    for (i = 0; i < List->n_values; i++) {
	line = List->value[i];

	if (!Vect_line_alive(Map, line))
	    continue;

	if (Vect_get_line_type(Map, line) & GV_POINTS)
	    continue;

	node[0] = node[1] = -1;
	Vect_get_line_nodes(Map, line, &(node[0]), &(node[1]));
	if (node[0] < 0 || node[1] < 0)
	    continue;

	connected = 0;
	Vect_reset_list(List_exclude);
	Vect_list_append(List_exclude, line);
	for (j = 0; j < n_nodes && !connected; j++) {
	    /* for each line node find lines in threshold */
	    Vect_get_node_coor(Map, node[j], &x, &y, &z);

	    do {
		/* find first nearest line */
		found = Vect_find_line_list(Map, x, y, z,
					    GV_LINES, thresh, WITHOUT_Z,
					    List_exclude, List_found);
		
		if (found > 0 && Vect_line_alive(Map, found)) {
		    /* try to connect lines (given node) */
		    G_debug(3, "Vedit_connect_lines(): lines=%d,%d", line, found);
		    if (connect_lines(Map, !j, line, found, thresh, List)) {
			G_debug(3, "Vedit_connect_lines(): lines=%d,%d -> connected",
				line, found);
			nlines_modified += 2;
			connected = 1;
		    }
		}
		
		Vect_list_append(List_exclude, found);
	    } while(List_found->n_values > 0 && !connected);
	}
    }
    
    Vect_destroy_list(List_exclude);
    Vect_destroy_list(List_found);

    return nlines_modified;
}

int connect_lines(struct Map_info *Map, int first, int line_from, int line_to,
		  double thresh, struct ilist *List)
{
    int line_new;
    int type_from, type_to;
    int n_points, seg, is;
    double x, y, px, py, x1, y1;
    double dist, spdist, lpdist, length, dist_p;
    double angle_t, angle_f, angle;

    struct line_pnts *Points_from, *Points_to, *Points_final;
    struct line_cats *Cats_from, *Cats_to;

    Points_from = Vect_new_line_struct();
    Points_to = Vect_new_line_struct();
    Points_final = Vect_new_line_struct();
    Cats_from = Vect_new_cats_struct();
    Cats_to = Vect_new_cats_struct();

    type_from = Vect_read_line(Map, Points_from, Cats_from, line_from);
    type_to = Vect_read_line(Map, Points_to, Cats_to, line_to);

    line_new = 0;
    if (!(type_from & GV_LINES) || !(type_to & GV_LINES))
	line_new = -1;

    if (line_new > -1) {
	n_points = Points_from->n_points - 1;

	if (first) {
	    x = Points_from->x[0];
	    y = Points_from->y[0];
	}
	else {
	    x = Points_from->x[n_points];
	    y = Points_from->y[n_points];
	}
	seg = Vect_line_distance(Points_to, x, y, 0.0, WITHOUT_Z,
				 &px, &py, NULL, &dist, &spdist, &lpdist);
	
	if (seg > 0 && dist > 0.0 && (thresh < 0. || dist <= thresh)) {
	    /* lines in threshold */
	    if (first)
		length = 0;
	    else
		length = Vect_line_length(Points_from);

	    if (Vect_point_on_line(Points_from, length,
				   NULL, NULL, NULL, &angle_f, NULL) > 0) {
		if (Vect_point_on_line(Points_to, lpdist,
				       NULL, NULL, NULL, &angle_t,
				       NULL) > 0) {
		    angle = angle_t - angle_f;
		    dist_p = fabs(dist / sin(angle));
		    
		    if (first) {
			if (angle_f < 0)
			    angle_f -= M_PI;
			else
			    angle_f += M_PI;
		    }

		    x1 = x + dist_p * cos(angle_f);
		    y1 = y + dist_p * sin(angle_f);

		    length = Vect_line_length(Points_to);
		    Vect_line_insert_point(Points_to, seg, x1, y1, 0.);
		    if (fabs(Vect_line_length(Points_to) - length) < length * 1e-3) {
			/* lines connected -> split line_to */
			/* update line_from */
			if (first) {
			    Points_from->x[0] = x1;
			    Points_from->y[0] = y1;
			}
			else {
			    Points_from->x[n_points] = x1;
			    Points_from->y[n_points] = y1;
			}
			
			line_new = Vect_rewrite_line(Map, line_from, type_from,
						     Points_from, Cats_from);
			/* Vect_list_append(List, line_new); */
			
			/* update line_to  -- first part */
			Vect_reset_line(Points_final);
			for (is = 0; is < seg; is++) {
			    Vect_append_point(Points_final, Points_to->x[is],
					  Points_to->y[is],
					      Points_to->z[is]);
			}
			Vect_append_point(Points_final, x1, y1, 0.0);
			line_new = Vect_rewrite_line(Map, line_to, type_to,
						     Points_final, Cats_to);
			/* Vect_list_append(List, line_new); */
			
			/* write second part */
			Vect_reset_line(Points_final);
			Vect_append_point(Points_final, x1, y1, 0.0);
			for (is = seg; is < Points_to->n_points; is++) {
			    Vect_append_point(Points_final, Points_to->x[is],
					      Points_to->y[is],
					      Points_to->z[is]);
			}
			
			/* rewrite first part */
			line_new = Vect_write_line(Map, type_to,
						   Points_final, Cats_to);
			/* Vect_list_append(List, line_new); */
		    }
		}
	    }
	}
    }

    Vect_destroy_line_struct(Points_from);
    Vect_destroy_line_struct(Points_to);
    Vect_destroy_line_struct(Points_final);
    Vect_destroy_cats_struct(Cats_from);
    Vect_destroy_cats_struct(Cats_to);

    return line_new > 0 ? 1 : 0;
}
