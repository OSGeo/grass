/* PURPOSE:      Develop the image segments */

/* Currently only region growing is implemented */

#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <time.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/raster.h>
#include <grass/segment.h>	/* segmentation library */
#include <grass/rbtree.h>	/* Red Black Tree library functions */
#include "iseg.h"

int create_isegs(struct globals *globals)
{
    int row, col;
    int successflag = 1;
    int have_bound, rid;
    CELL current_bound, bounds_val;

    if (globals->bounds_map == NULL) {
	/* just one time through loop */
	successflag = globals->method_fn(globals);
    }
    else {
	/* outer processing loop for polygon constraints */
	for (current_bound = globals->lower_bound;
	     current_bound <= globals->upper_bound; current_bound++) {

	    G_debug(1, "current_bound = %d", current_bound);

	    have_bound = 0;

	    /* get min/max row/col to narrow the processing window */
	    globals->row_min = globals->nrows;
	    globals->row_max = 0;
	    globals->col_min = globals->ncols;
	    globals->col_max = 0;
	    for (row = 0; row < globals->nrows; row++) {
		for (col = 0; col < globals->ncols; col++) {
		    FLAG_SET(globals->null_flag, row, col);
		    Segment_get(&globals->bounds_seg, &bounds_val,
				row, col);

		    if (!Rast_is_c_null_value(&bounds_val)
		        && bounds_val == current_bound) {

			Segment_get(&globals->rid_seg, &rid, row, col);
			if (!Rast_is_c_null_value(&rid)) {
			    have_bound = 1;

			    FLAG_UNSET(globals->null_flag, row, col);

			    if (globals->row_min > row)
				globals->row_min = row;
			    if (globals->row_max < row)
				globals->row_max = row;
			    if (globals->col_min > col)
				globals->col_min = col;
			    if (globals->col_max < col)
				globals->col_max = col;
			}
		    }
		}
	    }
	    globals->row_max++;
	    globals->col_max++;

	    if (have_bound)
		successflag = globals->method_fn(globals);
	}    /* end outer loop for processing polygons */

	/* restore NULL flag */
	flag_clear_all(globals->null_flag);
	for (row = 0; row < globals->nrows; row++) {
	    for (col = 0; col < globals->ncols; col++) {
		Segment_get(&globals->rid_seg, &rid, row, col);
		if (Rast_is_c_null_value(&rid))
		    FLAG_SET(globals->null_flag, row, col);
	    }
	}
    }

    if (globals->method == ORM_RG) {
	/* renumber */
	int i, *new_id, max_id;

	G_debug(1, "Largest assigned ID: %d", globals->max_rid);

	new_id = G_malloc((globals->max_rid + 1) * sizeof(int));
	
	for (i = 0; i <= globals->max_rid; i++)
	    new_id[i] = 0;

	for (row = 0; row < globals->nrows; row++) {
	    for (col = 0; col < globals->ncols; col++) {
		Segment_get(&globals->rid_seg, &rid, row, col);
		if (!Rast_is_c_null_value(&rid))
		    new_id[rid]++;
	    }
	}

	max_id = 0;
	for (i = 0; i <= globals->max_rid; i++) {
	    if (new_id[i] > 0) {
		max_id++;
		new_id[i] = max_id;
	    }
	}
	globals->max_rid = max_id;
	G_debug(1, "Largest renumbered ID: %d", globals->max_rid);
	
	globals->new_id = new_id;
    }

    return successflag;
}

void find_four_neighbors(int p_row, int p_col,
			        int neighbors[8][2])
{
    /* north */
    neighbors[0][0] = p_row - 1;
    neighbors[0][1] = p_col;

    /* east */
    neighbors[1][0] = p_row;
    neighbors[1][1] = p_col + 1;

    /* south */
    neighbors[2][0] = p_row + 1;
    neighbors[2][1] = p_col;

    /* west */
    neighbors[3][0] = p_row;
    neighbors[3][1] = p_col - 1;

    return;
}

void find_eight_neighbors(int p_row, int p_col,
			         int neighbors[8][2])
{
    /* get the 4 orthogonal neighbors */
    find_four_neighbors(p_row, p_col, neighbors);

    /* get the 4 diagonal neighbors */
    /* north-west */
    neighbors[4][0] = p_row - 1;
    neighbors[4][1] = p_col - 1;

    /* north-east */
    neighbors[5][0] = p_row - 1;
    neighbors[5][1] = p_col + 1;

    /* south-west */
    neighbors[6][0] = p_row + 1;
    neighbors[6][1] = p_col - 1;

    /* south-east */
    neighbors[7][0] = p_row + 1;
    neighbors[7][1] = p_col + 1;

    return;
}

/* similarity / distance between two points based on their input raster values */
/* assumes first point values already saved in files->bands_seg - only run Segment_get once for that value... */
/* TODO: Segment_get already happened for a[] values in the main function.  Could remove a[] from these parameters */
double calculate_euclidean_similarity(struct ngbr_stats *Ri,
                                      struct ngbr_stats *Rk,
				      struct globals *globals)
{
    double val = 0., diff;
    int n = globals->nbands - 1;

    /* squared euclidean distance, sum the square differences for each dimension */
    do {
	diff = Ri->mean[n] - Rk->mean[n];
	    
	val += diff * diff;
    } while (n--);

    /* the return value should always be in the range 0 - 1 */
    if (val <= 0)
	return 0.;

    val /= globals->max_diff;

#ifdef _OR_SHAPE_
    if (globals->shape_weight < 1)
	val = val * globals->shape_weight + (1 - globals->shape_weight) *
	      calculate_shape(rsi, rsk, nshared, globals);
#endif

    return val;
}

double calculate_manhattan_similarity(struct ngbr_stats *Ri,
                                      struct ngbr_stats *Rk,
				      struct globals *globals)
{
    double val = 0.;
    int n = globals->nbands - 1;

    /* squared manhattan distance, sum the differences for each dimension */
    do {
	val += fabs(Ri->mean[n] - Rk->mean[n]);
    } while (n--);

    /* the return value should always be in the range 0 - 1 */
    if (val <= 0)
	return 0.;

    val /= globals->max_diff;

#ifdef _OR_SHAPE_
    if (globals->shape_weight < 1)
	val = val * globals->shape_weight + (1 - globals->shape_weight) *
	      calculate_shape(rsi, rsk, nshared, globals);
#endif

    return val;
}
