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
#include "pavl.h"
#include "iseg.h"

int remove_small_clumps(struct globals *globals);

/* standard gauss function:
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
    int row, col, t, n;
    int mwrow, mwrow1, mwrow2, mwnrows, mwcol, mwcol1, mwcol2, mwncols, radiusc;
    double hspat, hspec, hspat2, hspec2, sigmaspat2, sigmaspec2;
    double hspecad, hspecad2;
    double ka2;
    double w, wsum;
    LARGEINT n_changes;
    double alpha2, maxdiff2;
    struct ngbr_stats Rin, Rout, Rn;
    double diff, diff2;
    SEGMENT *seg_tmp;
    double mindiff, mindiffzero, mindiffavg, mindiffzeroavg;
    double avgdiff, avgdiffavg;
    LARGEINT nvalid, count;
    int do_gauss = 0, do_adaptive, do_progressive;

    Rin.mean = G_malloc(globals->datasize);
    Rout.mean = G_malloc(globals->datasize);
    Rn.mean = G_malloc(globals->datasize);

    alpha2 = globals->alpha * globals->alpha;
    do_adaptive = globals->ms_adaptive;
    do_progressive = globals->ms_progressive;
    
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

    /* spatial bandwidth */
    hspat = globals->hs;
    if (hspat < 1) {
	hspat = 1.5;
	globals->hs = hspat;
    }

    hspat2 = hspat * hspat;
    sigmaspat2 = hspat2 / 9.;
    radiusc = hspat;	/* radius in cells truncated to integer */
    mwnrows = mwncols = radiusc * 2 + 1;

    /* estimate spectral bandwidth for given spatial bandwidth */
    mindiffavg = mindiffzeroavg = 0;
    avgdiffavg = 0;
    nvalid = 0;

    G_message(_("Estimating spectral bandwidth for spatial bandwidth %g..."), hspat);
    G_percent_reset();
    for (row = globals->row_min; row < globals->row_max; row++) {
	G_percent(row - globals->row_min,
		  globals->row_max - globals->row_min, 4);

	mwrow1 = row - radiusc;
	mwrow2 = mwrow1 + mwnrows;
	if (mwrow1 < globals->row_min)
	    mwrow1 = globals->row_min;
	if (mwrow2 > globals->row_max)
	    mwrow2 = globals->row_max;

	for (col = globals->col_min; col < globals->col_max; col++) {
	    if ((FLAG_GET(globals->null_flag, row, col)))
		continue;

	    /* get current band values */
	    Segment_get(globals->bands_in, (void *)Rin.mean,
			row, col);

	    mwcol1 = col - radiusc;
	    mwcol2 = mwcol1 + mwncols;
	    if (mwcol1 < globals->col_min)
		mwcol1 = globals->col_min;
	    if (mwcol2 > globals->col_max)
		mwcol2 = globals->col_max;
	    
	    /* get minimum spectral distance for this cell */
	    count = 0;
	    mindiff = globals->max_diff;
	    mindiffzero = globals->max_diff;
	    avgdiff = 0;
	    for (mwrow = mwrow1; mwrow < mwrow2; mwrow++) {
		for (mwcol = mwcol1; mwcol < mwcol2; mwcol++) {
		    if ((FLAG_GET(globals->null_flag, mwrow, mwcol)))
			continue;
		    if (mwrow == row && mwcol == col)
			continue;

		    diff = mwrow - row;
		    diff2 = diff * diff;
		    diff = mwcol - col;
		    diff2 += diff * diff;

		    if (diff2 <= hspat2) {

			Segment_get(globals->bands_in, (void *)Rn.mean,
				    mwrow, mwcol);

			/* get spectral distance */
			diff2 = (globals->calculate_similarity)(&Rin, &Rn, globals);

			if (mindiff > diff2)
			    mindiff = diff2;
			if (mindiffzero > diff2 && diff2 > 0)
			    mindiffzero = diff2;
			avgdiff += sqrt(diff2);
			count++;
		    }
		}
	    }
	    if (count) {
		nvalid++;
		if (mindiff > 0)
		    mindiffavg += sqrt(mindiff);
		mindiffzeroavg += sqrt(mindiffzero);
		if (avgdiff > 0)
		    avgdiffavg += avgdiff / count;
	    }
	}
    }
    G_percent(1, 1, 1);
    if (!nvalid) {
	G_fatal_error(_("Empty moving windows"));
    }

    mindiffavg /= nvalid;
    mindiffzeroavg /= nvalid;
    avgdiffavg /= nvalid;
    G_debug(1, "Average minimum difference to neighbours: %g", mindiffavg);
    G_debug(1, "Average minimum difference excl zero to neighbours: %g", mindiffzeroavg);
    G_debug(1, "Average average difference to neighbours: %g", avgdiffavg);

    /* use avgdiffavg as hspec for adaptive bandwidth */

    hspec = globals->hr;
    if (hspec < 0 || hspec >= 1) {
	hspec = sqrt(avgdiffavg / 10.0);
	hspec = avgdiffavg;
	hspec = mindiffzeroavg;
	
	if (do_progressive)
	    G_message(_("Initial range bandwidth: %g"), hspec);
	else
	    G_message(_("Estimated range bandwidth: %g"), hspec);
	globals->hr = hspec;
    }
    else {
	G_message(_("Estimated range bandwidth: %g"), mindiffzeroavg);
    }
    if (do_adaptive) {
	/* bandwidth is now standard deviation for adaptive bandwidth 
	 * using a gaussian function with range bandwidth used as 
	 * bandwidth for the gaussian function
	 * the aim is to produce similar but improved results with 
	 * adaptive bandwidth
	 * thus increase bandwidth */
	hspec = sqrt(hspec);
    }

    hspec2 = hspec * hspec;
    sigmaspec2 = hspec2 / 9.;

    if (!do_progressive) {
	G_message(_("Spatial bandwidth: %g"), hspat);
	G_message(_("Range bandwidth: %g"), hspec);
    }

    G_debug(4, "Starting to process %"PRI_LONG" candidate cells",
	    globals->candidate_count);

    t = 0;
    n_changes = 1;
    maxdiff2 = 0;
    while (t < globals->end_t && n_changes > 0) {

	G_message(_("Processing pass %d..."), ++t);

	/* cells within an object should become more similar with each pass
	 * therefore the spectral bandwidth could be decreased
	 * and the spatial bandwidth could be increased */

	/* spatial bandwidth: double the area covered by the moving window
	 * area = M_PI * hspat * hspat
	 * new hspat = sqrt(M_PI * hspat * hspat * 2 / M_PI)
	 * no good, too large increases */

	if (do_progressive) {
	    if (t > 1)
		hspat *= 1.1;
	    hspat2 = hspat * hspat;
	    sigmaspat2 = hspat2 / 9.;
	    radiusc = hspat;	/* radius in cells truncated to integer */
	    mwnrows = mwncols = radiusc * 2 + 1;

	    /* spectral bandwidth: reduce by 0.7 */
	    if (t > 1)
		hspec *= 0.9;
	    hspec2 = hspec * hspec;
	    sigmaspec2 = hspec2 / 9.;

	    G_verbose_message(_("Spatial bandwidth: %g"), hspat);
	    G_verbose_message(_("Range bandwidth: %g"), hspec);
	}

	n_changes = 0;
	maxdiff2 = 0;

	/* swap input and output */
	seg_tmp = globals->bands_in;
	globals->bands_in = globals->bands_out;
	globals->bands_out = seg_tmp;

	/*process candidate cells */
	G_percent_reset();
	for (row = globals->row_min; row < globals->row_max; row++) {
	    G_percent(row - globals->row_min,
	              globals->row_max - globals->row_min, 4);

	    mwrow1 = row - radiusc;
	    mwrow2 = mwrow1 + mwnrows;
	    if (mwrow1 < globals->row_min)
		mwrow1 = globals->row_min;
	    if (mwrow2 > globals->row_max)
		mwrow2 = globals->row_max;

	    for (col = globals->col_min; col < globals->col_max; col++) {
		if ((FLAG_GET(globals->null_flag, row, col)))
		    continue;

		/* get current band values */
		Segment_get(globals->bands_in, (void *)Rin.mean,
			    row, col);
		
		/* init output */
		for (n = 0; n < globals->nbands; n++)
		    Rout.mean[n] = 0.;

		mwcol1 = col - radiusc;
		mwcol2 = mwcol1 + mwncols;
		if (mwcol1 < globals->col_min)
		    mwcol1 = globals->col_min;
		if (mwcol2 > globals->col_max)
		    mwcol2 = globals->col_max;

		hspecad2 = hspec2;
		
		if (do_adaptive) {
		    /* adapt initial range bandwidth */
		    
		    ka2 = hspec2; 	/* OTB: conductance parameter */
		    
		    avgdiff = 0;
		    count = 0;
		    for (mwrow = mwrow1; mwrow < mwrow2; mwrow++) {
			for (mwcol = mwcol1; mwcol < mwcol2; mwcol++) {
			    if ((FLAG_GET(globals->null_flag, mwrow, mwcol)))
				continue;
			    if (mwrow == row && mwcol == col)
				continue;

			    diff = mwrow - row;
			    diff2 = diff * diff;
			    diff = mwcol - col;
			    diff2 += diff * diff;

			    if (diff2 <= hspat2) {

				Segment_get(globals->bands_in, (void *)Rn.mean,
					    mwrow, mwcol);

				/* get spectral distance */
				diff2 = (globals->calculate_similarity)(&Rin, &Rn, globals);

				avgdiff += sqrt(diff2);
				count++;
			    }
			}
		    }
		    hspecad2 = 0;
		    if (avgdiff > 0) {
			avgdiff /= count;
			hspecad = hspec;
			/* OTB-like, contrast enhancing */
			hspecad = exp(-avgdiff * avgdiff / (2 * ka2)) * avgdiff;
			/* preference for large regions, from Perona Malik 1990 
			 * if the settings are right, it could be used to reduce noise */
			/* hspecad = 1 / (1 + (avgdiff * avgdiff / (2 * hspec2))); */
			hspecad2 = hspecad * hspecad;
			G_debug(1, "avg spectral diff: %g", avgdiff);
			G_debug(1, "initial hspec2: %g", hspec2);
			G_debug(1, "adapted hspec2: %g", hspecad2);
		    }
		}
		
		/* actual mean shift */
		wsum = 0;
		for (mwrow = mwrow1; mwrow < mwrow2; mwrow++) {
		    for (mwcol = mwcol1; mwcol < mwcol2; mwcol++) {
			if ((FLAG_GET(globals->null_flag, mwrow, mwcol)))
			    continue;
			diff = mwrow - row;
			diff2 = diff * diff;
			diff = mwcol - col;
			diff2 += diff * diff;

			if (diff2 <= hspat2) {
			    w = 1;
			    if (do_gauss)
				w = gauss_kernel(diff2, sigmaspat2);

			    Segment_get(globals->bands_in, (void *)Rn.mean,
					mwrow, mwcol);

			    /* check spectral distance */
			    diff2 = (globals->calculate_similarity)(&Rin, &Rn, globals);
			    if (diff2 <= hspecad2) {
				if (do_gauss)
				    w *= gauss_kernel(diff2, sigmaspec2);
				wsum += w;
				for (n = 0; n < globals->nbands; n++)
				    Rout.mean[n] += w * Rn.mean[n];
			    }
			}
		    }
		}
		
		if (wsum > 0) {
		    for (n = 0; n < globals->nbands; n++)
			Rout.mean[n] /= wsum;
		}
		else {
		    for (n = 0; n < globals->nbands; n++)
			Rout.mean[n] = Rin.mean[n];
		}

		/* put new band values */
		Segment_put(globals->bands_out, (void *)Rout.mean,
			    row, col);

		/* if the squared difference between old and new band values 
		 * is larger than alpha2, then increase n_changes */
		
		diff2 = (globals->calculate_similarity)(&Rin, &Rout, globals);
		if (diff2 > alpha2)
		    n_changes++;
		if (maxdiff2 < diff2)
		    maxdiff2 = diff2;
	    }
	}
	G_percent(1, 1, 1);
	G_message(_("Changes > threshold: %"PRI_LONG", largest change: %g"), n_changes, sqrt(maxdiff2));
    }
    if (n_changes > 1)
	G_message(_("Mean shift stopped at %d due to reaching max iteration limit, more changes may be possible"), t);
    else
	G_message(_("Mean shift converged after %d iterations"), t);

    /* identify connected components */
    cluster_bands(globals);

    /* remove small regions */
    remove_small_clumps(globals);
    
    return TRUE;
}

static int cmp_rc(const void *first, const void *second)
{
    struct rc *a = (struct rc *)first, *b = (struct rc *)second;

    if (a->row == b->row)
	return (a->col - b->col);

    return (a->row - b->row);
}

static void free_item(void *p)
{
    G_free(p);
}

static int find_best_neighbour(struct globals *globals, int row, int col,
                               int this_id, struct NB_TREE *nbtree,
			       int *reg_size, struct ngbr_stats **Rbest,
			       int *best_n_row, int *best_n_col)
{
    int rown, coln, n, count;
    int neighbors[8][2];
    struct rc next, ngbr_rc, *pngbr_rc;
    struct rclist rilist;
    int no_check;
    int ngbr_id;
    struct pavl_table *visited;
    struct ngbr_stats Ri, Rk, *Rfound;
    double sim, best_sim;
    int best_n_id;
    int have_Ri;

    Ri.mean = G_malloc(globals->datasize);
    Rk.mean = G_malloc(globals->datasize);

    nbtree_clear(nbtree);
    FLAG_UNSET(globals->candidate_flag, row, col);

    visited = pavl_create(cmp_rc, NULL);
    ngbr_rc.row = row;
    ngbr_rc.col = col;
    pngbr_rc = G_malloc(sizeof(struct rc));
    *pngbr_rc = ngbr_rc;
    pavl_insert(visited, pngbr_rc);
    pngbr_rc = NULL;

    /* breadth-first search */
    next.row = row;
    next.col = col;
    rclist_init(&rilist);
    count = 1;
    best_n_id = -1;
    best_sim = 2;
    
    do {
	have_Ri = 0;
	globals->find_neighbors(next.row, next.col, neighbors);
	n = globals->nn - 1;
	do {
	    rown = neighbors[n][0];
	    coln = neighbors[n][1];
	    no_check = 0;
	    if (rown < globals->row_min || rown >= globals->row_max ||
		coln < globals->col_min || coln >= globals->col_max)
		no_check = 1;
	    if (!no_check && (FLAG_GET(globals->null_flag, rown, coln)))
		no_check = 1;

	    ngbr_rc.row = rown;
	    ngbr_rc.col = coln;
	    if (pngbr_rc == NULL)
		pngbr_rc = G_malloc(sizeof(struct rc));
	    *pngbr_rc = ngbr_rc;

	    if (!no_check && pavl_insert(visited, pngbr_rc) == NULL) {
		pngbr_rc = NULL;

		/* get neighbor ID */
		Segment_get(&globals->rid_seg, (void *) &ngbr_id, rown, coln);
		/* same neighbour */
		if (ngbr_id == this_id) {
		    count++;
		    rclist_add(&rilist, rown, coln);
		    FLAG_UNSET(globals->candidate_flag, rown, coln);
		}
		else { /* different neighbour */
		    /* compare to this cell next.row, next.col */
		    if (!have_Ri) {
			Segment_get(globals->bands_out, (void *) Ri.mean, next.row, next.col);
			have_Ri = 1;
		    }
		    Segment_get(globals->bands_out, (void *) Rk.mean, rown, coln);
		    sim = globals->calculate_similarity(&Ri, &Rk, globals);
		    if (best_sim > sim) {
			best_sim = sim;
			best_n_id = ngbr_id;
			*best_n_row = rown;
			*best_n_col = coln;
		    }

		    /* find in neighbor tree */
		    Rk.id = ngbr_id;
		    if ((Rfound = nbtree_find(nbtree, &Rk))) {
			Rfound->count++;
			if (*Rbest && (*Rbest)->count < Rfound->count)
			    *Rbest = Rfound;
		    }
		    else {
			Rk.count = 1;
			Rk.row = rown;
			Rk.col = coln;
			nbtree_insert(nbtree, &Rk);
			if (!(*Rbest))
			    *Rbest = nbtree_find(nbtree, &Rk);
		    }
		}
	    }
	} while (n--);    /* end do loop - next neighbor */
    } while (rclist_drop(&rilist, &next));   /* while there are cells to check */

    rclist_destroy(&rilist);
    if (pngbr_rc)
	G_free(pngbr_rc);
    pavl_destroy(visited, free_item);
    G_free(Ri.mean);
    G_free(Rk.mean);
    
    *reg_size = count;

    return best_n_id;
}

static int check_reg_size(struct globals *globals, int minsize, int row, int col)
{
    int rown, coln, n;
    int neighbors[8][2];
    int this_id;
    int ngbr_id;
    LARGEINT reg_size;
    struct pavl_table *visited;
    struct rc next, ngbr_rc, *pngbr_rc;
    struct rclist rilist;
    int no_check;
    
    if (!(FLAG_GET(globals->candidate_flag, row, col)))
	return minsize;

    FLAG_UNSET(globals->candidate_flag, row, col);

    visited = pavl_create(cmp_rc, NULL);
    ngbr_rc.row = row;
    ngbr_rc.col = col;
    pngbr_rc = G_malloc(sizeof(struct rc));
    *pngbr_rc = ngbr_rc;
    pavl_insert(visited, pngbr_rc);
    pngbr_rc = NULL;

    /* get this ID */
    Segment_get(&globals->rid_seg, (void *) &this_id, row, col);

    /* breadth-first search */
    next.row = row;
    next.col = col;
    rclist_init(&rilist);
    reg_size = 1;

    do {
	globals->find_neighbors(next.row, next.col, neighbors);
	n = globals->nn - 1;
	do {
	    rown = neighbors[n][0];
	    coln = neighbors[n][1];
	    no_check = 0;
	    if (rown < globals->row_min || rown >= globals->row_max ||
		coln < globals->col_min || coln >= globals->col_max)
		no_check = 1;
	    if (!no_check && (FLAG_GET(globals->null_flag, rown, coln)))
		no_check = 1;

	    ngbr_rc.row = rown;
	    ngbr_rc.col = coln;
	    if (pngbr_rc == NULL)
		pngbr_rc = G_malloc(sizeof(struct rc));
	    *pngbr_rc = ngbr_rc;

	    if (!no_check && pavl_insert(visited, pngbr_rc) == NULL) {
		pngbr_rc = NULL;

		/* get neighbour ID */
		Segment_get(&globals->rid_seg, (void *) &ngbr_id, rown, coln);
		/* same neighbour */
		if (ngbr_id == this_id) {
		    reg_size++;
		    rclist_add(&rilist, rown, coln);
		    FLAG_UNSET(globals->candidate_flag, rown, coln);
		}
	    }
	} while (n--);    /* end do loop - next neighbor */
    } while (rclist_drop(&rilist, &next));   /* while there are cells to check */

    rclist_destroy(&rilist);
    if (pngbr_rc)
	G_free(pngbr_rc);
    pavl_destroy(visited, free_item);

    return reg_size;
}

static int update_rid(struct globals *globals, int row, int col, int new_id)
{
    int rown, coln, n;
    int neighbors[8][2];
    int this_id;
    int ngbr_id;
    struct rc next;
    struct rclist rilist;
    int no_check;

    /* get this ID */
    Segment_get(&globals->rid_seg, (void *) &this_id, row, col);
    Segment_put(&globals->rid_seg, (void *) &new_id, row, col);

    /* breadth-first search */
    next.row = row;
    next.col = col;
    rclist_init(&rilist);

    do {
	globals->find_neighbors(next.row, next.col, neighbors);
	n = globals->nn - 1;
	do {
	    rown = neighbors[n][0];
	    coln = neighbors[n][1];
	    no_check = 0;
	    if (rown < globals->row_min || rown >= globals->row_max ||
		coln < globals->col_min || coln >= globals->col_max)
		no_check = 1;
	    if (!no_check && (FLAG_GET(globals->null_flag, rown, coln)))
		no_check = 1;

	    if (!no_check) {
		/* get neighbour ID */
		Segment_get(&globals->rid_seg, (void *) &ngbr_id, rown, coln);
		/* same neighbour */
		if (ngbr_id == this_id) {
		    rclist_add(&rilist, rown, coln);
		    Segment_put(&globals->rid_seg, (void *) &new_id, rown, coln);
		}
	    }
	} while (n--);    /* end do loop - next neighbor */
    } while (rclist_drop(&rilist, &next));   /* while there are cells to check */

    rclist_destroy(&rilist);

    return 1;
}

int remove_small_clumps(struct globals *globals)
{
    int row, col, i;
    struct NB_TREE *nbtree;
    int this_id;
    struct ngbr_stats *Rbest;
    int best_n_id, best_n_row, best_n_col;
    int reg_size;
    CELL *renumber, n_regions, min_rid;

    /* two possible modes
     * best (most similar) neighbor
     * neighbor with longest shared boundary 
     */

    if (globals->min_segment_size < 2)
	return 0;

    G_message(_("Merging segments smaller than %d cells..."), globals->min_segment_size);

    /* init renumber */
    renumber = (CELL *) G_malloc(sizeof(CELL) * (globals->max_rid + 1));
    for (i = 0; i <= globals->max_rid; i++)
	renumber[i] = 0;

    /* clear candidate flag */
    flag_clear_all(globals->candidate_flag);

    min_rid = globals->max_rid;

    /* Set candidate flag to true/1 for all non-NULL cells */
    for (row = globals->row_min; row < globals->row_max; row++) {
	for (col = globals->col_min; col < globals->col_max; col++) {
	    if (!(FLAG_GET(globals->null_flag, row, col))) {
		FLAG_SET(globals->candidate_flag, row, col);
		Segment_get(&globals->rid_seg, (void *) &this_id, row, col);
		/* renumber is region size */
		if (renumber[this_id] <= globals->min_segment_size) {
		    renumber[this_id]++;
		    if (min_rid > this_id)
			min_rid = this_id;
		}
	    }
	}
    }
    min_rid--;

    nbtree = nbtree_create(globals->nbands, globals->datasize);

    /* go through all cells */
    G_percent_reset();
    for (row = globals->row_min; row < globals->row_max; row++) {
	G_percent(row - globals->row_min,
		  globals->row_max - globals->row_min, 2);
	for (col = globals->col_min; col < globals->col_max; col++) {
	    if ((FLAG_GET(globals->null_flag, row, col)))
		continue;
	    if (!(FLAG_GET(globals->candidate_flag, row, col)))
		continue;

	    /* get this ID */
	    Segment_get(&globals->rid_seg, (void *) &this_id, row, col);

	    reg_size = renumber[this_id];
	    best_n_id = 1;

	    while (reg_size < globals->min_segment_size && best_n_id > 0) {

		Rbest = NULL;
		reg_size = 1;
		best_n_row = best_n_col = -1;

		best_n_id = find_best_neighbour(globals, row, col, this_id,
		                                nbtree, &reg_size, &Rbest,
						&best_n_row, &best_n_col);

		/* Rbest: pointer to most common neighbour
		 * best_n_id, best_n_[row|col]: most similar neighbour */

		if (reg_size < globals->min_segment_size && best_n_id > 0) {
		    /* update rid */
		    update_rid(globals, row, col, best_n_id);
		    /* mark as merged */
		    renumber[best_n_id] += renumber[this_id];
		    reg_size = renumber[best_n_id];
		    renumber[this_id] = 0;
		    this_id = best_n_id;
		}
	    }
	}
    }
    G_percent(1, 1, 1);

    n_regions = 0;
    /* renumber becomes new region ID */
    for (i = 1; i <= globals->max_rid; i++) {
	if (renumber[i] > 0) {
	    renumber[i] = ++n_regions;
	}
    }

    G_message(_("Renumbering remaining %d segments..."), n_regions);

    for (row = globals->row_min; row < globals->row_max; row++) {
	G_percent(row - globals->row_min,
		  globals->row_max - globals->row_min, 4);
	for (col = globals->col_min; col < globals->col_max; col++) {
	    if ((FLAG_GET(globals->null_flag, row, col)))
		continue;

	    /* get this ID */
	    Segment_get(&globals->rid_seg, (void *) &this_id, row, col);
	    
	    if (Rast_is_c_null_value(&this_id) || this_id < 1)
		continue;

	    this_id = renumber[this_id] + min_rid;
	    Segment_put(&globals->rid_seg, (void *) &this_id, row, col);
	}
    }
    G_percent(1, 1, 1);
    
    globals->max_rid = n_regions + min_rid;
    G_free(renumber);
    nbtree_clear(nbtree);

    return 1;
}
