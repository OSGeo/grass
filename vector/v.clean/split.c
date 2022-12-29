/****************************************************************
 *
 * MODULE:       v.clean
 * 
 * AUTHOR(S):    Markus Metz
 *               
 * PURPOSE:      Split lines - helper tool for breaking lines
 *               
 * COPYRIGHT:    (C) 2012 by the GRASS Development Team
 *
 *               This program is free software under the 
 *               GNU General Public License (>=v2). 
 *               Read the file COPYING that comes with GRASS
 *               for details.
 *
 ***************************************************************/
#include <stdlib.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>

int split_line(struct Map_info *Map, int otype, struct line_pnts *Points,
	       struct line_cats *Cats, struct Map_info *Err, double split_distance);

/* split lines
 * threshold is determined automatically
 * returns number of split points */
int split_lines(struct Map_info *Map, int otype, struct Map_info *Err)
{
    int line, nlines, n_split_lines, n_splits_total, type;
    struct line_pnts *Points;
    struct line_cats *Cats;
    double area_size, split_distance;
    struct bound_box box;
    
    nlines = Vect_get_num_lines(Map);
    n_split_lines = 0;
    for (line = 1; line <= nlines; line++) {
	type = Vect_get_line_type(Map, line);
	if ((type & otype) && (type & GV_LINES))
	    n_split_lines++;
    }

    if (n_split_lines < 50)
	return 0;

    Vect_get_map_box(Map, &box);
    area_size = sqrt((box.E - box.W) * (box.N - box.S));

    split_distance = area_size / log(n_split_lines);
    /* divisor is the handle: increase divisor to decrease split_distance
     * see also v.in.ogr */
    split_distance = split_distance / 16.;
    G_debug(1, "area size: %f", area_size);
    G_debug(1, "split distance: %f", split_distance);

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();
    n_splits_total = 0;
    for (line = 1; line <= nlines; line++) {
	int n_splits;

	type = Vect_get_line_type(Map, line);
	if (!((type & otype) && (type & GV_LINES)))
	    continue;
	    
	Vect_read_line(Map, Points, Cats, line);
	
	/* can't split boundaries with only 2 vertices */
	if (Points->n_points < 3)
	    continue;

	n_splits = split_line(Map, type, Points, Cats, Err, split_distance);
	
	if (n_splits)
	    Vect_delete_line(Map, line);
	
	n_splits_total += n_splits;
    }
    
    Vect_destroy_line_struct(Points);
    Vect_destroy_cats_struct(Cats);

    G_verbose_message(_("Line splits: %d"), n_splits_total);

    return n_splits_total;    
}

/* split a line using split_distance
 * returns number of split points */
int split_line(struct Map_info *Map, int otype, struct line_pnts *Points,
	       struct line_cats *Cats, struct Map_info *Err, double split_distance)
{
    int i, n_segs = 0;
    double dist = 0., seg_dist, dx, dy;
    struct line_pnts *OutPoints;

    /* don't write zero length boundaries */
    Vect_line_prune(Points);
    if (Points->n_points < 2)
	return 0;

    /* can't split boundaries with only 2 vertices */
    if (Points->n_points == 2) {
	/* Vect_write_line(Map, otype, Points, Cats); */
	return 0;
    }

    OutPoints = Vect_new_line_struct();
    Vect_append_point(OutPoints, Points->x[0], Points->y[0], Points->z[0]);
    Vect_append_point(OutPoints, Points->x[1], Points->y[1], Points->z[1]);
    dx = Points->x[1] - Points->x[0];
    dy = Points->y[1] - Points->y[0];
    dist = sqrt(dx * dx + dy * dy);

    /* trying to keep line length smaller than split_distance
     * alternative, less code: write line as soon as split_distance is exceeded */
    for (i = 2; i < Points->n_points; i++) {
	dx = Points->x[i] - Points->x[i - 1];
	dy = Points->y[i] - Points->y[i - 1];
	seg_dist = sqrt(dx * dx + dy * dy);
	dist += seg_dist;
	if (dist > split_distance) {
	    Vect_write_line(Map, otype, OutPoints, Cats);
	    Vect_reset_line(OutPoints);
	    dist = seg_dist;
	    Vect_append_point(OutPoints, Points->x[i - 1], Points->y[i - 1],
			      Points->z[i - 1]);
	    n_segs++;
	}
	Vect_append_point(OutPoints, Points->x[i], Points->y[i],
			  Points->z[i]);
    }
    /* write out remaining line points only when original line was split */
    if (n_segs)
	Vect_write_line(Map, otype, OutPoints, Cats);

    Vect_destroy_line_struct(OutPoints);

    return n_segs;
}
