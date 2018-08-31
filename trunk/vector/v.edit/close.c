#include <string.h>
#include "global.h"

/*!
   \brief Close lines (boundaries)
   
   Using threshold distance (-1 for no limit)
   
   \param[in] Map vector map
   \param[in] ltype vector feature type (line | boundary)
   \param[in] thresh threshold distance

   \return number of modified features
*/
int close_lines(struct Map_info *Map, int ltype, double thresh)
{
    int nlines, line, type, nlines_modified, newline;
    int npoints;
    double *x, *y, *z;
    double dist;

    struct line_pnts *Points;
    struct line_cats *Cats;

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    nlines_modified = 0;

    Vect_build_partial(Map, GV_BUILD_BASE);
    nlines = Vect_get_num_lines(Map);

    for (line = 1; line <= nlines; line++) {
	if (!Vect_line_alive(Map, line))
	    continue;

	type = Vect_read_line(Map, Points, Cats, line);

	if (!(type & ltype))
	    continue;

	npoints = Points->n_points - 1;
	x = Points->x;
	y = Points->y;
	z = Points->z;

	dist = Vect_points_distance(x[npoints], y[npoints], z[npoints],
				    x[0], y[0], z[0], WITHOUT_Z);

	if (dist > 0 && (thresh < 0.0 || dist <= thresh)) {
	    Vect_line_delete_point(Points, npoints);
	    Vect_append_point(Points, x[0], y[0], z[0]);

	    newline = Vect_rewrite_line(Map, line, type, Points, Cats);
	    if (newline < 0) {
		G_warning(_("Unable to rewrite line %d"), line);
		return -1;
	    }
	    nlines_modified++;
	}
    }

    Vect_destroy_line_struct(Points);
    Vect_destroy_cats_struct(Cats);

    return nlines_modified;
}
