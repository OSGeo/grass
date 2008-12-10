#include "Gwater.h"
#include <grass/gis.h>
#include <grass/glocale.h>


int do_cum(void)
{
    SHORT r, c, dr, dc;
    CELL is_swale, value, valued;
    int killer, threshold, count;

    G_message(_("SECTION 3: Accumulating Surface Flow with SFD."));

    count = 0;
    if (bas_thres <= 0)
	threshold = 60;
    else
	threshold = bas_thres;
    while (first_cum != -1) {
	G_percent(count++, do_points, 2);
	killer = first_cum;
	first_cum = astar_pts[killer].nxt;
	if ((dr = astar_pts[killer].downr) > -1) {
	    r = astar_pts[killer].r;
	    c = astar_pts[killer].c;
	    dc = astar_pts[killer].downc;
	    value = wat[SEG_INDEX(wat_seg, r, c)];
	    if (ABS(value) >= threshold)
		FLAG_SET(swale, r, c);
	    valued = wat[SEG_INDEX(wat_seg, dr, dc)];
	    if (value > 0) {
		if (valued > 0)
		    valued += value;
		else
		    valued -= value;
	    }
	    else {
		if (valued < 0)
		    valued += value;
		else
		    valued = value - valued;
	    }
	    wat[SEG_INDEX(wat_seg, dr, dc)] = valued;
	    valued = ABS(valued) + 0.5;
	    is_swale = FLAG_GET(swale, r, c);
	    if (is_swale || ((int)valued) >= threshold) {
		FLAG_SET(swale, dr, dc);
	    }
	    else {
		if (er_flag && !is_swale)
		    slope_length(r, c, dr, dc);
	    }
	}
    }
    G_percent(count, do_points, 1);	/* finish it */
    G_free(astar_pts);

    return 0;
}

/***************************************
 * 
 * MFD references
 * 
 * original:
 * Quinn, P., Beven, K., Chevallier, P., and Planchon, 0. 1991. 
 * The prediction of hillslope flow paths for distributed hydrological 
 * modelling using digital terrain models, Hydrol. Process., 5, 59-79.
 * 
 * modified by Holmgren (1994):
 * Holmgren, P. 1994. Multiple flow direction algorithms for runoff 
 * modelling in grid based elevation models: an empirical evaluation
 * Hydrol. Process., 8, 327-334.
 * 
 * implemented here:
 * Holmgren (1994) with modifications to honour A * path in order to get
 * out of depressions and across obstacles with gracefull flow convergence
 * before depressions/obstacles and gracefull flow divergence after 
 * depressions/obstacles
 * 
 * ************************************/

int do_cum_mfd(void)
{
    int r, c, dr, dc;
    CELL is_swale;
    DCELL value, valued;
    int killer, threshold, count;

    int mfd_cells, astar_not_set;
    double *dist_to_nbr, *weight, sum_weight, max_weight;
    int r_nbr, c_nbr, ct_dir, np_side, in_val;
    double dx, dy;
    CELL ele, ele_nbr;
    double prop;
    int workedon;

    G_message(_("SECTION 3: Accumulating Surface Flow with MFD"));
    G_debug(1, "MFD convergence factor set to %d.", c_fac);

    /* distances to neighbours */
    dist_to_nbr = (double *)G_malloc(sides * sizeof(double));
    weight = (double *)G_malloc(sides * sizeof(double));

    for (ct_dir = 0; ct_dir < sides; ct_dir++) {
	/* get r, c (r_nbr, c_nbr) for neighbours */
	r_nbr = nextdr[ct_dir];
	c_nbr = nextdc[ct_dir];
	/* account for rare cases when ns_res != ew_res */
	dy = ABS(r_nbr) * window.ns_res;
	dx = ABS(c_nbr) * window.ew_res;
	if (ct_dir < 4)
	    dist_to_nbr[ct_dir] = dx + dy;
	else
	    dist_to_nbr[ct_dir] = sqrt(dx * dx + dy * dy);
    }

    flag_clear_all(worked);
    workedon = 0;

    count = 0;
    if (bas_thres <= 0)
	threshold = 60;
    else
	threshold = bas_thres;

    while (first_cum != -1) {
	G_percent(count++, do_points, 2);
	killer = first_cum;
	first_cum = astar_pts[killer].nxt;
	if ((dr = astar_pts[killer].downr) > -1) {
	    r = astar_pts[killer].r;
	    c = astar_pts[killer].c;
	    dc = astar_pts[killer].downc;
	    value = wat[SEG_INDEX(wat_seg, r, c)];
	    valued = wat[SEG_INDEX(wat_seg, dr, dc)];

	    /* disabled for MFD */
	    /* if (ABS(value) > threshold)
	       FLAG_SET(swale, r, c); */

	    /* start MFD */

	    /* get weights */
	    max_weight = 0;
	    sum_weight = 0;
	    np_side = -1;
	    mfd_cells = 0;
	    astar_not_set = 1;
	    ele = alt[SEG_INDEX(alt_seg, r, c)];
	    /* this loop is needed to get the sum of weights */
	    for (ct_dir = 0; ct_dir < sides; ct_dir++) {
		/* get r, c (r_nbr, c_nbr) for neighbours */
		r_nbr = r + nextdr[ct_dir];
		c_nbr = c + nextdc[ct_dir];
		weight[ct_dir] = -1;
		/* check that neighbour is within region */
		if (r_nbr >= 0 && r_nbr < nrows && c_nbr >= 0 &&
		    c_nbr < ncols) {
		    in_val = FLAG_GET(worked, r_nbr, c_nbr);
		    if (in_val == 0) {
			ele_nbr = alt[SEG_INDEX(alt_seg, r_nbr, c_nbr)];
			if (ele_nbr < ele) {
			    weight[ct_dir] =
				mfd_pow(((ele -
					  ele_nbr) / dist_to_nbr[ct_dir]),
					c_fac);
			    sum_weight += weight[ct_dir];
			    mfd_cells++;

			    if (weight[ct_dir] > max_weight)
				max_weight = weight[ct_dir];

			    if (dr == r_nbr && dc == c_nbr) {
				astar_not_set = 0;
			    }
			}
		    }
		    if (dr == r_nbr && dc == c_nbr)
			np_side = ct_dir;
		}
	    }

	    /* honour A * path 
	     * mfd_cells == 0: fine, SFD along A * path
	     * mfd_cells == 1 && astar_not_set == 0: fine, SFD along A * path
	     * mfd_cells > 1 && astar_not_set == 0: MFD, set A * path to max_weight
	     * mfd_cells > 0 && astar_not_set == 1: A * path not included, add to mfd_cells
	     */

	    /* MFD, A * path not included, add to mfd_cells */
	    if (mfd_cells > 0 && astar_not_set == 1) {
		mfd_cells++;
		sum_weight += max_weight;
		weight[np_side] = max_weight;
	    }

	    /* MFD, A * path included, set A * path to max_weight */
	    if (mfd_cells > 1 && astar_not_set == 0) {
		sum_weight = sum_weight - weight[np_side] + max_weight;
		weight[np_side] = max_weight;
	    }

	    /* set flow accumulation for neighbours */
	    if (mfd_cells > 1) {
		prop = 0.0;
		for (ct_dir = 0; ct_dir < sides; ct_dir++) {
		    /* get r, c (r_nbr, c_nbr) for neighbours */
		    r_nbr = r + nextdr[ct_dir];
		    c_nbr = c + nextdc[ct_dir];

		    /* check that neighbour is within region */
		    if (r_nbr >= 0 && r_nbr < nrows && c_nbr >= 0 &&
			c_nbr < ncols && weight[ct_dir] > -0.5) {
			in_val = FLAG_GET(worked, r_nbr, c_nbr);
			if (in_val == 0) {

			    weight[ct_dir] = weight[ct_dir] / sum_weight;
			    prop += weight[ct_dir];	/* check everything sums up to 1.0 */

			    valued = wat[SEG_INDEX(wat_seg, r_nbr, c_nbr)];
			    if (value > 0) {
				if (valued > 0)
				    valued += value * weight[ct_dir];
				else
				    valued -= value * weight[ct_dir];
			    }
			    else {
				if (valued < 0)
				    valued += value * weight[ct_dir];
				else
				    valued = value * weight[ct_dir] - valued;
			    }
			    wat[SEG_INDEX(wat_seg, r_nbr, c_nbr)] = valued;
			}
			else if (ct_dir == np_side) {
			    workedon++;	/* check for consistency with A * path */
			}
		    }
		}
		if (ABS(prop - 1.0) > 5E-6f) {
		    G_warning(_("MFD: cumulative proportion of flow distribution not 1.0 but %f"),
			      prop);
		}
		valued = wat[SEG_INDEX(wat_seg, dr, dc)];
	    }

	    if (mfd_cells < 2) {
		if (value > 0) {
		    if (valued > 0)
			valued += value;
		    else
			valued -= value;
		}
		else {
		    if (valued < 0)
			valued += value;
		    else
			valued = value - valued;
		}
		wat[SEG_INDEX(wat_seg, dr, dc)] = valued;
	    }

	    /* MFD finished */

	    valued = ABS(valued) + 0.5;

	    is_swale = FLAG_GET(swale, r, c);
	    if (is_swale || ((int)valued) >= threshold) {
		FLAG_SET(swale, dr, dc);
	    }
	    else {
		if (er_flag && !is_swale)
		    slope_length(r, c, dr, dc);
	    }
	    FLAG_SET(worked, r, c);
	}
    }
    G_percent(count, do_points, 1);	/* finish it */
    if (workedon)
	G_warning(_("MFD: A * path already processed when distributing flow: %d of %d"),
		  workedon, do_points);

    G_free(astar_pts);

    flag_destroy(worked);

    return 0;
}

double mfd_pow(double base, int exp)
{
    int x;
    double result;

    result = base;
    if (exp == 1)
	return result;

    for (x = 2; x <= exp; x++) {
	result *= base;
    }
    return result;
}
