#include "Gwater.h"
#include <unistd.h>
#include <grass/gis.h>
#include <grass/glocale.h>


int do_cum(void)
{
    SHORT r, c, dr, dc;
    CELL is_swale;
    DCELL value, valued;
    POINT point;
    int killer, threshold, count;

    G_message(_("SECTION 3: Accumulating Surface Flow with SFD."));
    G_debug(1, "MFD convergence factor set to %d.", c_fac);

    count = 0;
    if (bas_thres <= 0)
	threshold = 60;
    else
	threshold = bas_thres;
    while (first_cum != -1) {
	G_percent(count++, do_points, 2);
	killer = first_cum;
	seg_get(&astar_pts, (char *)&point, 0, killer);
	first_cum = point.nxt;
	if ((dr = point.downr) > -1) {
	    r = point.r;
	    c = point.c;
	    dc = point.downc;
	    dseg_get(&wat, &value, r, c);
	    if (ABS(value) >= threshold)
		bseg_put(&swale, &one, r, c);
	    dseg_get(&wat, &valued, dr, dc);
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
	    dseg_put(&wat, &valued, dr, dc);
	    bseg_get(&swale, &is_swale, r, c);
	    if (is_swale || ABS(valued) >= threshold) {
		bseg_put(&swale, &one, dr, dc);
	    }
	    else {
		if (er_flag && !is_swale)
		    slope_length(r, c, dr, dc);
	    }
	}
    }
    seg_close(&astar_pts);

    G_percent(count, do_points, 1);	/* finish it */
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
    POINT point;
    int killer, threshold, count;

    /* MFD */
    int mfd_cells, stream_cells, swale_cells, astar_not_set;
    double *dist_to_nbr, *weight, sum_weight, max_weight;
    int r_nbr, c_nbr, r_max, c_max, r_swale, c_swale, ct_dir, np_side;
    double dx, dy;
    CELL ele, ele_nbr, aspect, asp_val, is_worked;
    double prop, max_acc, max_swale;
    int workedon;

    G_message(_("SECTION 3: Accumulating Surface Flow with MFD."));

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

    /* reset worked, takes time... */
    for (r = 0; r < nrows; r++) {
	for (c = 0; c < ncols; c++) {
	    bseg_put(&worked, &zero, r, c);
	}
    }

    workedon = 0;

    count = 0;
    if (bas_thres <= 0)
	threshold = 60;
    else
	threshold = bas_thres;

    while (first_cum != -1) {
	G_percent(count++, do_points, 2);
	killer = first_cum;
	seg_get(&astar_pts, (char *)&point, 0, killer);
	first_cum = point.nxt;
	if ((dr = point.downr) > -1) {
	    r = point.r;
	    c = point.c;
	    dc = point.downc;
	    dseg_get(&wat, &value, r, c);
	    dseg_get(&wat, &valued, dr, dc);

	    /* disabled for MFD */
	    /* if (ABS(value) > threshold)
	       bseg_put(&swale, &one, r, c); */

	    /* start MFD */

	    /* get weights */
	    max_weight = 0;
	    sum_weight = 0;
	    np_side = -1;
	    mfd_cells = 0;
	    astar_not_set = 1;
	    cseg_get(&alt, &ele, r, c);
	    /* this loop is needed to get the sum of weights */
	    for (ct_dir = 0; ct_dir < sides; ct_dir++) {
		/* get r, c (r_nbr, c_nbr) for neighbours */
		r_nbr = r + nextdr[ct_dir];
		c_nbr = c + nextdc[ct_dir];
		weight[ct_dir] = -1;
		/* check that neighbour is within region */
		if (r_nbr >= 0 && r_nbr < nrows && c_nbr >= 0 &&
		    c_nbr < ncols) {
		    bseg_get(&worked, &is_worked, r_nbr, c_nbr);
		    if (is_worked == 0) {
			cseg_get(&alt, &ele_nbr, r_nbr, c_nbr);
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
	    stream_cells = 0;
	    swale_cells = 0;
	    max_acc = -1;
	    max_swale = -1;
	    r_max = dr;
	    c_max = dc;
	    r_swale = dr;
	    c_swale = dc;

	    if (mfd_cells > 1) {
		prop = 0.0;
		for (ct_dir = 0; ct_dir < sides; ct_dir++) {
		    /* get r, c (r_nbr, c_nbr) for neighbours */
		    r_nbr = r + nextdr[ct_dir];
		    c_nbr = c + nextdc[ct_dir];

		    /* check that neighbour is within region */
		    if (r_nbr >= 0 && r_nbr < nrows && c_nbr >= 0 &&
			c_nbr < ncols && weight[ct_dir] > -0.5) {
			bseg_get(&worked, &is_worked, r_nbr, c_nbr);
			bseg_get(&swale, &is_swale, r_nbr, c_nbr);
			if (is_worked == 0) {

			    weight[ct_dir] = weight[ct_dir] / sum_weight;
			    prop += weight[ct_dir];	/* check everything sums up to 1.0 */

			    dseg_get(&wat, &valued, r_nbr, c_nbr);
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
			    dseg_put(&wat, &valued, r_nbr, c_nbr);

			    if ((ABS(valued) + 0.5) >= threshold)
				stream_cells++;
			    if (ABS(valued) > max_acc) {
				max_acc = ABS(valued);
				r_max = r_nbr;
				c_max = c_nbr;
			    }
			    /* assist in stream merging */
			    if (is_swale && ABS(valued) > max_swale) {
				max_swale = ABS(valued);
				r_swale = r_nbr;
				c_swale = c_nbr;
			    }
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
		dseg_get(&wat, &valued, dr, dc);
		if (max_swale > -0.5) {
		    r_max = r_swale;
		    c_max = c_swale;
		}
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
		dseg_put(&wat, &valued, dr, dc);
	    }

	    /* MFD finished */

	    valued = ABS(valued) + 0.5;
	    bseg_get(&swale, &is_swale, r, c);
	    /* start new stream: only works with A * path */
	    if (!is_swale && (int)valued >= threshold && stream_cells < 2 &&
		swale_cells < 1) {
		bseg_put(&swale, &one, dr, dc);
	    }
	    /* continue stream */
	    if (is_swale) {
		bseg_put(&swale, &one, r_max, c_max);
	    }
	    else {
		if (er_flag && !is_swale)
		    slope_length(r, c, r_max, c_max);
	    }
	    /* update asp */
	    if (dr != r_max || dc != c_max) {
		aspect = drain[r - r_max + 1][c - c_max + 1];
		cseg_get(&asp, &asp_val, r, c);
		if (asp_val < 0)
		    aspect = -aspect;
		cseg_put(&asp, &aspect, r, c);

	    }
	    bseg_put(&worked, &one, r, c);
	}
    }
    G_percent(count, do_points, 1);	/* finish it */
    if (workedon)
	G_warning(_("MFD: A * path already processed when distributing flow: %d of %d"),
		  workedon, do_points);

    seg_close(&astar_pts);

    bseg_close(&worked);

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
