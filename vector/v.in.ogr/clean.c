#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>

/* The OSM topological model differs from the GRASS topological model. 
 * OSM topologically correct connections of lines can be on all vertices of the line.
 * -> split lines at all vertices where another line has the same vertex */
void convert_osm_lines(struct Map_info *Map, double snap)
{
    int line, nlines;
    struct line_pnts *Points = Vect_new_line_struct();
    struct line_pnts *NPoints = Vect_new_line_struct();
    struct line_cats *Cats = Vect_new_cats_struct();
    double x, y, z, nx, ny, dx, dy, snap2, dist, tdist;
    int with_z = Vect_is_3d(Map);
    int i, last_i, j, nline;
    int line_split, n_splits;

    G_message(_("Converting OSM lines..."));
    
    n_splits = 0;
    if (snap < 0)
	snap = 0;

    snap2 = snap * snap;

    nlines = Vect_get_num_lines(Map);
    G_percent(0, nlines, 5);
    for (line = 1; line <= nlines; line++) {
	G_percent(line, nlines, 5);
	if (Vect_get_line_type(Map, line) != GV_LINE)
	    continue;
	
	Vect_read_line(Map, Points, Cats, line);
	line_split = 0;
	last_i = 0;
	for (i = 1; i < Points->n_points - 1; i++) {
	    x = Points->x[i];
	    y = Points->y[i];
	    z = Points->z[i];
	    nline = Vect_find_line(Map, x, y, z, GV_LINE, snap, with_z, line);
	    if (nline > 0) {
		/* intersection with other line */
		Vect_read_line(Map, NPoints, NULL, nline);

		dx = NPoints->x[0] - x;
		dy = NPoints->y[0] - y;
		dist = dx * dx + dy * dy;
		nx = NPoints->x[0];
		ny = NPoints->y[0];
		for (j = 1; j < NPoints->n_points; j++) {
		    dx = NPoints->x[j] - x;
		    dy = NPoints->y[j] - y;
		    tdist = dx * dx + dy * dy;
		    if (dist > tdist) {
			dist = tdist;
			nx = NPoints->x[j];
			ny = NPoints->y[j];
		    } 
		}

		if (dist <= snap2) {
		    if (line_split == 0)
			Vect_delete_line(Map, line);
		    line_split++;
		    Points->x[i] = nx;
		    Points->y[i] = ny;
		    Vect_reset_line(NPoints);
		    for (j = last_i; j <= i; j++)
			Vect_append_point(NPoints, Points->x[j], Points->y[j], Points->z[j]);
		    Vect_write_line(Map, GV_LINE, NPoints, Cats);
		    last_i = i;
		    n_splits++;
		}
	    }
	    if (last_i != i) {
		/* self-intersection */
		dx = Points->x[0] - x;
		dy = Points->y[0] - y;
		dist = dx * dx + dy * dy;
		nx = Points->x[0];
		ny = Points->y[0];
		for (j = 1; j < Points->n_points; j++) {
		    if (j == i)
			continue;
		    dx = Points->x[j] - x;
		    dy = Points->y[j] - y;
		    tdist = dx * dx + dy * dy;
		    if (dist > tdist) {
			dist = tdist;
			nx = Points->x[j];
			ny = Points->y[j];
		    } 
		}
		if (dist <= snap2) {
		    if (line_split == 0)
			Vect_delete_line(Map, line);
		    line_split++;
		    Points->x[i] = nx;
		    Points->y[i] = ny;
		    Vect_reset_line(NPoints);
		    for (j = last_i; j <= i; j++)
			Vect_append_point(NPoints, Points->x[j], Points->y[j], Points->z[j]);
		    Vect_write_line(Map, GV_LINE, NPoints, Cats);
		    last_i = i;
		    n_splits++;
		}
	    }
	}
	if (line_split) {
	    Vect_reset_line(NPoints);
	    for (j = last_i; j < Points->n_points; j++)
		Vect_append_point(NPoints, Points->x[j], Points->y[j], Points->z[j]);
	    Vect_write_line(Map, GV_LINE, NPoints, Cats);
	}
    }
    if (n_splits)
	G_verbose_message(_("Number of OSM line splits: %d"), n_splits);
    G_message(_("Merging lines..."));
    Vect_merge_lines(Map, GV_LINE, NULL, NULL);
    Vect_destroy_cats_struct(Cats);
    Vect_destroy_line_struct(Points);
    Vect_destroy_line_struct(NPoints);
}
