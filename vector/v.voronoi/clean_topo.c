#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>
#include "sw_defs.h"
#include "defs.h"

int clean_topo(void)
{
    int type, line, nlines;
    int area, nareas;
    int verbose;
    int err_boundaries, err_centr_out, err_centr_dupl, err_nocentr;
    struct bound_box box;
    double snap_thresh;

    /* cleaning part 1: count errors */
    G_message(_("Searching for topology errors..."));
    verbose = G_verbose();
    G_set_verbose(0);
    Vect_build_partial(&Out, GV_BUILD_CENTROIDS);
    G_set_verbose(verbose);
    err_boundaries = err_centr_out = err_centr_dupl = err_nocentr = 0;
    nlines = Vect_get_num_lines(&Out);
    for (line = 1; line <= nlines; line++) {

	if (!Vect_line_alive(&Out, line))
	    continue;

	type = Vect_get_line_type(&Out, line);
	if (type == GV_BOUNDARY) {
	    int left, right;

	    Vect_get_line_areas(&Out, line, &left, &right);

	    if (left == 0 || right == 0) {
		G_debug(3, "line = %d left = %d right = %d", line, 
			left, right);
		err_boundaries++;
	    }
	}
	if (type == GV_CENTROID) {
	    area = Vect_get_centroid_area(&Out, line);
	    if (area == 0)
		err_centr_out++;
	    else if (area < 0)
		err_centr_dupl++;
	}
    }

    err_nocentr = 0;
    nareas = Vect_get_num_areas(&Out);
    for (area = 1; area <= nareas; area++) {
	if (!Vect_area_alive(&Out, area))
	    continue;
	line = Vect_get_area_centroid(&Out, area);
	if (line == 0)
	    err_nocentr++;
    }

    /* cleaning part 2: snap */
    /* TODO: adjust snapping treshold to ULP */
    Vect_get_map_box(&Out, &box);
    snap_thresh = fabs(box.W);
    if (snap_thresh < fabs(box.E))
	snap_thresh = fabs(box.E);
    if (snap_thresh < fabs(box.N))
	snap_thresh = fabs(box.N);
    if (snap_thresh < fabs(box.S))
	snap_thresh = fabs(box.S);
    snap_thresh = d_ulp(snap_thresh);
    
    if (err_nocentr || err_centr_dupl || err_centr_out) {
	int nmod;

	G_important_message(_("Cleaning output topology"));
	Vect_snap_lines(&Out, GV_BOUNDARY, snap_thresh, NULL);
	do {
	    Vect_break_lines(&Out, GV_BOUNDARY, NULL);
	    Vect_remove_duplicates(&Out, GV_BOUNDARY, NULL);
	    nmod =
		Vect_clean_small_angles_at_nodes(&Out, GV_BOUNDARY, NULL);
	} while (nmod > 0);

	G_message(_("Removing dangles..."));
	Vect_remove_dangles(&Out, GV_BOUNDARY, -1.0, NULL);
	G_message(_("Removing bridges..."));
	Vect_remove_bridges(&Out, NULL, NULL, NULL);

	err_boundaries = 0;
	nlines = Vect_get_num_lines(&Out);
	for (line = 1; line <= nlines; line++) {

	    if (!Vect_line_alive(&Out, line))
		continue;

	    type = Vect_get_line_type(&Out, line);
	    if (type == GV_BOUNDARY) {
		int left, right;

		Vect_get_line_areas(&Out, line, &left, &right);

		if (left == 0 || right == 0) {
		    G_debug(3, "line = %d left = %d right = %d", line, 
			    left, right);
		    err_boundaries++;
		}
	    }
	}
    }
    /* cleaning part 3: remove remaining incorrect boundaries */
    if (err_boundaries) {
	G_important_message(_("Removing incorrect boundaries from output"));
	nlines = Vect_get_num_lines(&Out);
	for (line = 1; line <= nlines; line++) {

	    if (!Vect_line_alive(&Out, line))
		continue;

	    type = Vect_get_line_type(&Out, line);
	    if (type == GV_BOUNDARY) {
		int left, right;

		Vect_get_line_areas(&Out, line, &left, &right);

		/* &&, not ||, no typo */
		if (left == 0 && right == 0) {
		    G_debug(3, "line = %d left = %d right = %d", line, 
			    left, right);
		    Vect_delete_line(&Out, line);
		}
	    }
	}
    }

    /* this is slow:
    if (in_area) {
	if (Type == GV_LINE)
	    G_message(_("Merging lines ..."));
	else
	    G_message(_("Merging boundaries ..."));
	Vect_merge_lines(&Out, Type, NULL, NULL);
    }
    */

    return 1;
}
