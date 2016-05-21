/* PURPOSE:      Develop the image segments */


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

/* standard gauss funtion:
 * a * exp(-(x - m)^2 / (2 * stddev^2)
 * a is not needed because the sum of weights is calculated for each 
 * sampling window
 * (x - m)^2 is the squared difference = diff2
 * stddev^2 is the variance
 * this code can be further simplified, e.g. by supplying 2 * var instead 
 * of var
 * 
 * the standard deviation is the bandwidth
 * */
 
static double gauss_kernel(double diff2, double var)
{
    return exp(-diff2 / (2 * var));
}


int mean_shift(struct globals *globals)
{
    int row, col, t;
    int n_changes;
    double alpha2;
    struct ngbr_stats Rin, Rout;
    double diff2;

    G_fatal_error(_("Mean shift is not yet implemented"));
    return FALSE;


    Rin.mean = G_malloc(globals->datasize);
    Rout.mean = G_malloc(globals->datasize);

    /* TODO: need another segment structure holding output
     * for mean shift, output of the previous iteration becomes input of the next iteration
     * initially, input and output are original band values */

    alpha2 = globals->alpha * globals->alpha;
    
    t = 0;
    n_changes = 1;
    while (t < globals->end_t && n_changes > 0) {

	G_message(_("Processing pass %d..."), ++t);

	n_changes = 0;
	globals->candidate_count = 0;
	flag_clear_all(globals->candidate_flag);

	/* Set candidate flag to true/1 for all non-NULL cells */
	for (row = globals->row_min; row < globals->row_max; row++) {
	    for (col = globals->col_min; col < globals->col_max; col++) {
		if (!(FLAG_GET(globals->null_flag, row, col))) {
		    
		    FLAG_SET(globals->candidate_flag, row, col);
		    globals->candidate_count++;
		}
	    }
	}

	G_debug(4, "Starting to process %ld candidate cells",
		globals->candidate_count);

	/*process candidate cells */
	G_percent_reset();
	for (row = globals->row_min; row < globals->row_max; row++) {
	    G_percent(row - globals->row_min,
	              globals->row_max - globals->row_min, 4);
	    for (col = globals->col_min; col < globals->col_max; col++) {
		if (!(FLAG_GET(globals->candidate_flag, row, col)))
		    continue;

		/* get current band values */
		Segment_get(&globals->bands_seg, (void *)Rin.mean,
			    row, col);

		/* adapt initial spatial and range bandwiths */

		/* calculate new band values */
		
		/* if the squared difference between old and new band values 
		 * is larger than alpha2, then increase n_changes */
		
		diff2 = (globals->calculate_similarity)(&Rin, &Rout, globals);
		if (diff2 > alpha2)
		    n_changes++;
	    }
	}
    }
    if (n_changes > 1)
	G_message(_("Mean shift stopped at %d due to reaching max iteration limit, more changes may be possible"), t);
    else
	G_message(_("Mean shift converged after %d iterations"), t);
    
    /* identify connected components */
    
    return TRUE;
}

