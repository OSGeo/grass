#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/vector.h>

static struct line_pnts *Points_wall, *Points_roof, *Points_floor;
static struct line_cats *Cats_floor;

/**
  \brief Extrude 2D vector feature to 3D

  - point -> 3d line (vertical)
  - line  -> set of faces (each segment defines one face)
  - area  -> set of faces + kernel

  \param In input vector map
  \param[in,out] Out output vector map
  \param Cats categories
  \param Points points
  \param fdrast background raster map
  \param trace trace raster map values
  \param interpolation method
  \param objheight object height
  \param voffset vertical offset
  \param window raster region
  \param type feature type
  \param centroid number of centroid for area

  \return number of writen objects
*/
int extrude(struct Map_info *In, struct Map_info *Out,
            const struct line_cats *Cats, const struct line_pnts *Points,
            int fdrast, int trace, int interp_method, double objheight, double voffset,
            const struct Cell_head *window, int type, int centroid)
{
    int k;			/* Points->n_points */
    int nlines;
    
    double voffset_dem;         /* minimal offset */
    double voffset_curr;	/* offset of current point */
    double voffset_next;	/* offset of next point */

    nlines = 0;

    if (type != GV_POINT && Points->n_points < 2)
	return nlines; /* not enough points to face */

    if (!Points_wall) {
        Points_wall  = Vect_new_line_struct();
        Points_roof  = Vect_new_line_struct();
        Points_floor = Vect_new_line_struct();
        Cats_floor    = Vect_new_cats_struct();
    }
    else {
        Vect_reset_line(Points_wall);
        Vect_reset_line(Points_roof);
        Vect_reset_line(Points_floor);
        Vect_reset_cats(Cats_floor);
    }

    voffset_dem = 0.0;
    /* do not trace -> calculate minimum dem offset */
    if (fdrast >= 0 && !trace) {
	for (k = 0; k < Points->n_points; k++) {
	    voffset_curr = Rast_get_sample(fdrast, window, NULL,
                                           Points->y[k], Points->x[k], 0, /* north, east */
                                           interp_method);
	    if (Rast_is_d_null_value(&voffset_curr))
		continue; /* skip null values */

	    if (k == 0) {
		voffset_dem = voffset_curr;
	    }
	    else {
		if (voffset_curr < voffset_dem)
		    voffset_dem = voffset_curr;
	    }
	}
    }

    /* build walls, roof and floor */
    for (k = 0; ; k++) {
	voffset_curr = voffset_next = 0.0;

	if (fdrast >= 0 && trace) {
	    voffset_curr = Rast_get_sample(fdrast, window, NULL,
                                           Points->y[k], Points->x[k], 0, /* north, east */
                                           interp_method);

	    if (type != GV_POINT) {
		voffset_next = Rast_get_sample(fdrast, window, NULL,
                                               Points->y[k + 1],         /* north, east */
                                               Points->x[k + 1], 0,
                                               interp_method);
	    }
	}

	if (Rast_is_d_null_value(&voffset_curr) ||
	    Rast_is_d_null_value(&voffset_next)) {
            if (k >= Points->n_points - 2)
                break;
            else
                continue;
	}

        if (trace) {
            voffset_curr += voffset;
            voffset_next += voffset;
        }
        else {
            voffset_curr = voffset_dem + voffset;
            voffset_next = voffset_dem + voffset;
        }

	if (type == GV_POINT) {
	    /* point -> 3d line (vertical) */
	    Vect_append_point(Points_wall, Points->x[k], Points->y[k],
			      Points->z[k] + voffset_curr);
	    Vect_append_point(Points_wall, Points->x[k], Points->y[k],
			      Points->z[k] + objheight + (trace ? voffset_curr : 0.));
	}
	
        if (type & (GV_LINE | GV_AREA)) {
	    /* reset */
	    Vect_reset_line(Points_wall);

	    /* line/boundary segment -> face */
	    Vect_append_point(Points_wall, Points->x[k], Points->y[k],
			      Points->z[k] + voffset_curr);
	    Vect_append_point(Points_wall, Points->x[k + 1], Points->y[k + 1],
			      Points->z[k + 1] + voffset_next);
	    Vect_append_point(Points_wall, Points->x[k + 1], Points->y[k + 1],
			      Points->z[k + 1] + objheight + (trace ? voffset_next : 0.));
	    Vect_append_point(Points_wall, Points->x[k], Points->y[k],
			      Points->z[k] + objheight + (trace ? voffset_curr : 0.));
	    Vect_append_point(Points_wall, Points->x[k], Points->y[k],
			      Points->z[k] + voffset_curr);

	    Vect_write_line(Out, GV_FACE, Points_wall, Cats);
	    nlines++;

	    if (type == GV_AREA) {
		/* roof */
		Vect_append_point(Points_roof, Points->x[k], Points->y[k],
				  Points->z[k] + objheight + (trace ? voffset_curr : 0.));
                /* floor */
		Vect_append_point(Points_floor, Points->x[k], Points->y[k],
				  Points->z[k] + voffset_curr);
	    }
	}
        if (k >= Points->n_points - 2)
            break;
    }

    if (type == GV_POINT) {
	Vect_write_line(Out, GV_LINE, Points_wall, Cats);
    }
    else if (type == GV_AREA && Points_roof->n_points > 3) {
        /* close roof and floor */
	Vect_append_point(Points_roof,
			  Points_roof->x[0], Points_roof->y[0],
			  Points_roof->z[0]);
	Vect_append_point(Points_floor,
			  Points_floor->x[0], Points_floor->y[0],
			  Points_floor->z[0]);
        /* write roof and floor */
        Vect_write_line(Out, GV_FACE, Points_roof, Cats);
        Vect_write_line(Out, GV_FACE, Points_floor, Cats);
	nlines += 2;

	if (centroid > 0) {
	    /* centroid -> kernel */
            Vect_read_line(In, Points_floor, Cats_floor, centroid);
	    Points_floor->z[0] = Points_roof->z[0] / 2.0; /* TODO: do it better */
	    Vect_write_line(Out, GV_KERNEL, Points_floor, Cats_floor);
	    nlines++;
	}
    }
    
    return nlines;
}
