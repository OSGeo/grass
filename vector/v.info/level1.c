#include <grass/glocale.h>

#include "local_proto.h"

/* code taken from Vect_build_nat() */
int level_one_info(struct Map_info *Map)
{
    struct Plus_head *plus;
    int type, first = 1;
    off_t offset;
    struct line_pnts *Points;
    struct line_cats *Cats;
    struct bound_box box;

    int n_primitives, n_points, n_lines, n_boundaries, n_centroids;
    int n_faces, n_kernels;

    G_debug(1, "Count vector objects for level 1");

    plus = &(Map->plus);

    n_primitives = n_points = n_lines = n_boundaries = n_centroids = 0;
    n_faces = n_kernels = 0;

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();
    
    Vect_rewind(Map);
    G_message(_("Topology not available for vector map <%s>. "
                "Registering primitives..."), Vect_get_full_name(Map));
    while (1) {
	/* register line */
	type = Vect_read_next_line(Map, Points, Cats);

	/* Note: check for dead lines is not needed, because they are skipped by V1_read_next_line_nat() */
	if (type == -1) {
	    G_warning(_("Unable to read vector map"));
	    return 0;
	}
	else if (type == -2) {
	    break;
	}

	/* count features */
	n_primitives++;
	
	if (type & GV_POINT)  /* probably most common */
	    n_points++;
	else if (type & GV_LINE)
	    n_lines++;
	else if (type & GV_BOUNDARY)
	    n_boundaries++;
	else if (type & GV_CENTROID)
	    n_centroids++;
	else if (type & GV_KERNEL)
	    n_kernels++;
	else if (type & GV_FACE)
	    n_faces++;

	offset = Map->head.last_offset;

	G_debug(3, "Register line: offset = %lu", (unsigned long)offset);
	dig_line_box(Points, &box);
	if (first == 1) {
	    Vect_box_copy(&(plus->box), &box);
	    first = 0;
	}
	else
	    Vect_box_extend(&(plus->box), &box);

	/* can't print progress, unfortunately */
	/* what about using G_clicker() then. or, check file 
	   size with stat.st_size and make a rough guess? */
/*
	if (G_verbose() > G_verbose_min() && i % 1000 == 0) {
	    if (format == G_INFO_FORMAT_PLAIN)
		fprintf(stderr, "%d..", i);
	    else
		fprintf(stderr, "%11d\b\b\b\b\b\b\b\b\b\b\b", i);
	}
	i++;
*/
    }

    /* save result in plus */
    plus->n_lines = n_primitives;
    plus->n_plines = n_points;
    plus->n_llines = n_lines;
    plus->n_blines = n_boundaries;
    plus->n_clines = n_centroids;
    plus->n_klines = n_kernels;
    plus->n_flines = n_faces;

    return 1;
}
