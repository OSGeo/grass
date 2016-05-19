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
	successflag = globals->method(globals);
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
		successflag = globals->method(globals);
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

    return successflag;
}
