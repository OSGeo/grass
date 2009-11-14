#include "Gwater.h"
#include <unistd.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>


int do_cum(void)
{
    SHORT r, c, dr, dc;
    CELL asp_val, asp_val_down;
    char is_swale, this_flag_value, flag_value;
    DCELL value, valued;
    POINT point;
    int killer, threshold;
    SHORT asp_r[9] = { 0, -1, -1, -1, 0, 1, 1, 1, 0 };
    SHORT asp_c[9] = { 0, 1, 0, -1, -1, -1, 0, 1, 1 };
    WAT_ALT wa, wadown;

    G_message(_("SECTION 3: Accumulating Surface Flow with SFD."));

    if (bas_thres <= 0)
	threshold = 60;
    else
	threshold = bas_thres;
    for (killer = 0; killer < do_points; killer++) {
	G_percent(killer, do_points, 1);
	seg_get(&astar_pts, (char *)&point, 0, killer);
	r = point.r;
	c = point.c;
	asp_val = point.asp;
	/* skip user-defined depressions */
	if (asp_val) {
	    dr = r + asp_r[ABS(asp_val)];
	    dc = c + asp_c[ABS(asp_val)];
	}
	else
	    dr = dc = -1;

	bseg_get(&bitflags, &this_flag_value, r, c);
	FLAG_UNSET(this_flag_value, WORKEDFLAG);

	if (dr >= 0 && dr < nrows && dc >= 0 && dc < ncols) {
	    /* TODO: do not distribute flow along edges, this causes artifacts */
	    seg_get(&watalt, (char *)&wa, r, c);
	    value = wa.wat;
	    is_swale = FLAG_GET(this_flag_value, SWALEFLAG);
	    if (fabs(value) >= threshold && !is_swale) {
		is_swale = 1;
		FLAG_SET(this_flag_value, SWALEFLAG);
	    }
	    seg_get(&watalt, (char *)&wadown, dr, dc);
	    valued = wadown.wat;
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
	    wadown.wat = valued;
	    seg_put(&watalt, (char *)&wadown, dr, dc);
	    /* update asp for depression */
	    if (is_swale && pit_flag) {
		cseg_get(&asp, &asp_val_down, dr, dc);
		if (asp_val > 0 && asp_val_down == 0) {
		    asp_val = -asp_val;
		    cseg_put(&asp, &asp_val, r, c);
		}
	    }
	    if (is_swale || fabs(valued) >= threshold) {
		bseg_get(&bitflags, &flag_value, dr, dc);
		FLAG_SET(flag_value, SWALEFLAG);
		bseg_put(&bitflags, &flag_value, dr, dc);
		is_swale = 1;
	    }
	    else {
		if (er_flag && !is_swale)
		    slope_length(r, c, dr, dc);
	    }
	}
	bseg_put(&bitflags, &this_flag_value, r, c);
    }
    G_percent(do_points, do_points, 1);	/* finish it */

    seg_close(&astar_pts);

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
    DCELL value, valued, *wat_nbr;
    POINT point;
    WAT_ALT wa;
    int killer, threshold;

    /* MFD */
    int mfd_cells, stream_cells, swale_cells, astar_not_set, is_null;
    double *dist_to_nbr, *weight, sum_weight, max_weight;
    int r_nbr, c_nbr, r_max, c_max, ct_dir, np_side;
    double dx, dy;
    CELL ele, *ele_nbr, asp_val;
    double prop, max_acc;
    int edge;
    char workedon, *flag_nbr, this_flag_value;
    SHORT asp_r[9] = { 0, -1, -1, -1, 0, 1, 1, 1, 0 };
    SHORT asp_c[9] = { 0, 1, 0, -1, -1, -1, 0, 1, 1 };

    G_message(_("SECTION 3: Accumulating Surface Flow with MFD."));
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

    flag_nbr = (char *)G_malloc(sides * sizeof(char));
    wat_nbr = (DCELL *)G_malloc(sides * sizeof(DCELL));
    ele_nbr = (CELL *)G_malloc(sides * sizeof(CELL));

    workedon = 0;

    if (bas_thres <= 0)
	threshold = 60;
    else
	threshold = bas_thres;

    for (killer = 0; killer < do_points; killer++) {
	G_percent(killer, do_points, 1);
	seg_get(&astar_pts, (char *)&point, 0, killer);
	r = point.r;
	c = point.c;
	asp_val = point.asp;
	/* skip user-defined depressions */
	if (asp_val) {
	    dr = r + asp_r[ABS(asp_val)];
	    dc = c + asp_c[ABS(asp_val)];
	}
	else
	    dr = dc = -1;

	seg_get(&watalt, (char *)&wa, r, c);
	value = wa.wat;
	if (point.guessed && value > 0) {
	    value = -value;
	    wa.wat = value;
	    seg_put(&watalt, (char *)&wa, r, c);
	}

	bseg_get(&bitflags, &this_flag_value, r, c);
	FLAG_UNSET(this_flag_value, WORKEDFLAG);
	
	if (dr >= 0 && dr < nrows && dc >= 0 && dc < ncols) {
	    /* do not distribute flow along edges, this causes artifacts */
	    if (FLAG_GET(this_flag_value, EDGEFLAG)) {
		bseg_put(&bitflags, &this_flag_value, r, c);
		continue;
	    }

	    r_max = dr;
	    c_max = dc;

	    /* get weights */
	    max_weight = 0;
	    sum_weight = 0;
	    np_side = -1;
	    mfd_cells = 0;
	    stream_cells = 0;
	    swale_cells = 0;
	    astar_not_set = 1;
	    ele = wa.ele;
	    is_null = 0;
	    edge = 0;
	    /* this loop is needed to get the sum of weights */
	    for (ct_dir = 0; ct_dir < sides; ct_dir++) {
		/* get r, c (r_nbr, c_nbr) for neighbours */
		r_nbr = r + nextdr[ct_dir];
		c_nbr = c + nextdc[ct_dir];
		weight[ct_dir] = -1;

		wat_nbr[ct_dir] = 0;
		ele_nbr[ct_dir] = 0;

		/* WORKEDFLAG has been set during A* Search
		 * reversed meaning here: 0 = done, 1 = not yet done */
		FLAG_UNSET(flag_nbr[ct_dir], WORKEDFLAG);

		/* check if neighbour is within region */
		if (r_nbr >= 0 && r_nbr < nrows && c_nbr >= 0 &&
		    c_nbr < ncols) {

		    bseg_get(&bitflags, &flag_nbr[ct_dir], r_nbr, c_nbr);
		    if (FLAG_GET(flag_nbr[ct_dir], WORKEDFLAG)) {

			seg_get(&watalt, (char *)&wa, r_nbr, c_nbr);
			wat_nbr[ct_dir] = wa.wat;
			ele_nbr[ct_dir] = wa.ele;

			edge = is_null = FLAG_GET(flag_nbr[ct_dir], NULLFLAG);
			if (!is_null && ele_nbr[ct_dir] <= ele) {
			    if (ele_nbr[ct_dir] < ele) {
				weight[ct_dir] =
				    mfd_pow(((ele -
					      ele_nbr[ct_dir]) / dist_to_nbr[ct_dir]),
					    c_fac);
			    }
			    if (ele_nbr[ct_dir] == ele) {
				weight[ct_dir] =
				    mfd_pow((0.5 / dist_to_nbr[ct_dir]),
					    c_fac);
			    }
			    sum_weight += weight[ct_dir];
			    mfd_cells++;

			    if (weight[ct_dir] > max_weight) {
				max_weight = weight[ct_dir];
			    }

			    if (dr == r_nbr && dc == c_nbr) {
				astar_not_set = 0;
			    }
			}
		    }
		    if (dr == r_nbr && dc == c_nbr)
			np_side = ct_dir;
		}
		else
		    edge = 1;
		if (edge)
		    break;
	    }
	    /* do not distribute flow along edges, this causes artifacts */
	    if (edge) {
		G_debug(3, "edge: should not get to here");
		bseg_put(&bitflags, &this_flag_value, r, c);
		continue;
	    }

	    /* honour A * path 
	     * mfd_cells == 0: fine, SFD along A * path
	     * mfd_cells == 1 && astar_not_set == 0: fine, SFD along A * path
	     * mfd_cells > 0 && astar_not_set == 1: A * path not included, add to mfd_cells
	     */

	    /* MFD, A * path not included, add to mfd_cells */
	    if (mfd_cells > 0 && astar_not_set == 1) {
		mfd_cells++;
		sum_weight += max_weight;
		weight[np_side] = max_weight;
	    }

	    /* set flow accumulation for neighbours */
	    max_acc = -1;

	    if (mfd_cells > 1) {
		prop = 0.0;
		for (ct_dir = 0; ct_dir < sides; ct_dir++) {
		    /* get r, c (r_nbr, c_nbr) for neighbours */
		    r_nbr = r + nextdr[ct_dir];
		    c_nbr = c + nextdc[ct_dir];

		    /* check if neighbour is within region */
		    if (r_nbr >= 0 && r_nbr < nrows && c_nbr >= 0 &&
			c_nbr < ncols && weight[ct_dir] > -0.5) {

			if (FLAG_GET(flag_nbr[ct_dir], WORKEDFLAG)) {

			    weight[ct_dir] = weight[ct_dir] / sum_weight;
			    /* check everything sums up to 1.0 */
			    prop += weight[ct_dir];

			    if (value > 0) {
				if (wat_nbr[ct_dir] > 0)
				    wat_nbr[ct_dir] += value * weight[ct_dir];
				else
				    wat_nbr[ct_dir] -= value * weight[ct_dir];
			    }
			    else {
				if (wat_nbr[ct_dir] < 0)
				    wat_nbr[ct_dir] += value * weight[ct_dir];
				else
				    wat_nbr[ct_dir] = value * weight[ct_dir] - wat_nbr[ct_dir];
			    }
			    valued = wat_nbr[ct_dir];
			    wa.wat = valued;
			    wa.ele = ele_nbr[ct_dir];
			    seg_put(&watalt, (char *)&wa, r_nbr, c_nbr);

			    /* get main drainage direction */
			    if (ABS(wat_nbr[ct_dir]) >= max_acc) {
				max_acc = ABS(wat_nbr[ct_dir]);
				r_max = r_nbr;
				c_max = c_nbr;
			    }
			}
			else if (ct_dir == np_side) {
			    /* check for consistency with A * path */
			    workedon++;
			}
		    }
		}
		if (ABS(prop - 1.0) > 5E-6f) {
		    G_warning(_("MFD: cumulative proportion of flow distribution not 1.0 but %f"),
			      prop);
		}

		/* adjust main drainage direction to A* path if possible */
		if (fabs(wat_nbr[np_side]) >= max_acc) {
		    max_acc = fabs(wat_nbr[ct_dir]);
		    r_max = dr;
		    c_max = dc;
		}
	    }
	    /* SFD-like accumulation */
	    else {
		seg_get(&watalt, (char *)&wa, dr, dc);
		valued = wa.wat;
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
		wa.wat = valued;
		seg_put(&watalt, (char *)&wa, dr, dc);
	    }

	    if (!st_flag) {
		/* update asp */
		if (dr != r_max || dc != c_max) {
		    if (asp_val > 0) {
			asp_val = drain[r - r_max + 1][c - c_max + 1];
			cseg_put(&asp, &asp_val, r, c);
		    }
		}
	    }
	}
	bseg_put(&bitflags, &this_flag_value, r, c);
    }
    G_percent(do_points, do_points, 1);	/* finish it */
    
    if (workedon)
	G_warning(_("MFD: A * path already processed when distributing flow: %d of %d cells"),
		  workedon, do_points);

    if (!st_flag) {
	seg_close(&astar_pts);
    }
    
    G_free(dist_to_nbr);
    G_free(weight);
    G_free(wat_nbr);
    G_free(ele_nbr);
    G_free(flag_nbr);

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
