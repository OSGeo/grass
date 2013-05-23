#include "Gwater.h"
#include <unistd.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

double get_dist(double *dist_to_nbr, double *contour)
{
    int ct_dir, r_nbr, c_nbr;
    double dx, dy, ns_res, ew_res;

    if (G_projection() == PROJECTION_LL) {
	double ew_dist1, ew_dist2, ew_dist3;
	double ns_dist1, ns_dist2, ns_dist3;

	G_begin_distance_calculations();

	/* EW Dist at North edge */
	ew_dist1 = G_distance(window.east, window.north,
	                      window.west, window.north);
	/* EW Dist at Center */
	ew_dist2 = G_distance(window.east, (window.north + window.south) / 2.,
	                      window.west, (window.north + window.south) / 2.);
	/* EW Dist at South Edge */
	ew_dist3 = G_distance(window.east, window.south,
	                      window.west, window.south);
	/* NS Dist at East edge */
	ns_dist1 = G_distance(window.east, window.north,
	                      window.east, window.south);
	/* NS Dist at Center */
	ns_dist2 = G_distance((window.west + window.east) / 2., window.north,
	                      (window.west + window.east) / 2., window.south);
	/* NS Dist at West edge */
	ns_dist3 = G_distance(window.west, window.north,
	                      window.west, window.south);

	ew_res = (ew_dist1 + ew_dist2 + ew_dist3) / (3 * window.cols);
	ns_res = (ns_dist1 + ns_dist2 + ns_dist3) / (3 * window.rows);
    }
    else {
	ns_res = window.ns_res;
	ew_res = window.ew_res;
    }
    
    for (ct_dir = 0; ct_dir < sides; ct_dir++) {
	/* get r, c (r_nbr, c_nbr) for neighbours */
	r_nbr = nextdr[ct_dir];
	c_nbr = nextdc[ct_dir];
	/* account for rare cases when ns_res != ew_res */
	dy = ABS(r_nbr) * ns_res;
	dx = ABS(c_nbr) * ew_res;
	if (ct_dir < 4)
	    dist_to_nbr[ct_dir] = (dx + dy) * ele_scale;
	else
	    dist_to_nbr[ct_dir] = sqrt(dx * dx + dy * dy) * ele_scale;
    }
    /* Quinn et al. 1991:
     * ns contour: ew_res / 2
     * ew contour: ns_res / 2
     * diag contour: sqrt(ns_res * nsres / 4 + ew_res * ew_res / 4) / 2
     *             = sqrt(ns_res * nsres + ew_res * ew_res) / 4
     * if ns_res == ew_res:
     *             sqrt(2 * (res * res) / 4 = res * 0.354
     *
     * contour lengths "have been subjectively chosen", 
     * no justification why the diagonal contour is shorter
     * BUT: if the diag contour is a bit shorter than the cardinal contour,
     * this is further enhancing the correction for diagonal flow bias
     * diagonal slope is already corrected for longer distance
     * smaller slope and shorter contour length have the same effect:
     * higher TCI
     */
    if (sides == 8) {
	/* contours are sides of an octagon, irregular if ns_res != ew_res
	 * ideally: arc lengths of an ellipse */
	contour[0] = contour[1] = tan(atan(ew_res / ns_res) / 2.) * ns_res;
	contour[2] = contour[3] = tan(atan(ns_res / ew_res) / 2.) * ew_res;
	G_debug(1, "ns contour: %.4f", contour[0]);
	G_debug(1, "ew contour: %.4f", contour[2]);
	contour[4] = (ew_res - contour[0]);
	contour[5] = (ns_res - contour[2]);
	contour[7] = sqrt(contour[4] * contour[4] + contour[5] * contour[5]) / 2.;
	G_debug(1, "diag contour: %.4f", contour[7]);
	contour[4] = contour[5] = contour[6] = contour[7];
    }
    else {
	/* contours are sides of a rectangle */
	contour[0] = contour[1] = ew_res;
	contour[2] = contour[3] = ns_res;
    }
    return ew_res * ns_res;
}

double get_slope_tci(CELL ele, CELL down_ele, double dist)
{
    if (down_ele >= ele)
	return 0.5 / dist;
    else
	return (double)(ele - down_ele) / dist;
}

int do_cum(void)
{
    int r, c, dr, dc;
    int r_nbr, c_nbr, ct_dir, np_side;
    char is_swale;
    DCELL value, valued;
    POINT point;
    GW_LARGE_INT killer;
    int threshold;
    int asp_r[9] = { 0, -1, -1, -1, 0, 1, 1, 1, 0 };
    int asp_c[9] = { 0, 1, 0, -1, -1, -1, 0, 1, 1 };
    WAT_ALT wa, wadown;
    ASP_FLAG af, afdown;
    double *dist_to_nbr, *contour;
    double tci_val, tci_div, cell_size;

    G_message(_("SECTION 3: Accumulating Surface Flow with SFD."));

    /* distances to neighbours, contour lengths */
    dist_to_nbr = (double *)G_malloc(sides * sizeof(double));
    contour = (double *)G_malloc(sides * sizeof(double));

    cell_size = get_dist(dist_to_nbr, contour);

    if (bas_thres <= 0)
	threshold = 60;
    else
	threshold = bas_thres;
    for (killer = 0; killer < do_points; killer++) {
	G_percent(killer, do_points, 1);
	seg_get(&astar_pts, (char *)&point, 0, killer);
	r = point.r;
	c = point.c;
	seg_get(&aspflag, (char *)&af, r, c);
	if (af.asp) {
	    dr = r + asp_r[ABS(af.asp)];
	    dc = c + asp_c[ABS(af.asp)];
	}
	/* skip user-defined depressions */
	else
	    dr = dc = -1;

	FLAG_UNSET(af.flag, WORKEDFLAG);

	if (dr >= 0 && dr < nrows && dc >= 0 && dc < ncols) {
	    np_side = -1;
	    r_nbr = dr;
	    c_nbr = dc;

	    for (ct_dir = 0; ct_dir < sides; ct_dir++) {
		/* get r, c (r_nbr, c_nbr) for neighbours */
		r_nbr = r + nextdr[ct_dir];
		c_nbr = c + nextdc[ct_dir];

		if (dr == r_nbr && dc == c_nbr)
		    np_side = ct_dir;
	    }
	    /* do not distribute flow along edges, this causes artifacts */
	    if (FLAG_GET(af.flag, EDGEFLAG)) {
		if (FLAG_GET(af.flag, SWALEFLAG) && af.asp > 0) {
		    af.asp = -1 * drain[r - dr + 1][c - dc + 1];
		}
		seg_put(&aspflag, (char *)&af, r, c);
		continue;
	    }

	    seg_get(&watalt, (char *)&wa, r, c);
	    value = wa.wat;
	    is_swale = FLAG_GET(af.flag, SWALEFLAG);
	    if (fabs(value) >= threshold && !is_swale) {
		is_swale = 1;
		FLAG_SET(af.flag, SWALEFLAG);
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

	    /* topographic wetness index ln(a / tan(beta)) */
	    if (tci_flag) {
		tci_div = contour[np_side] * 
		       get_slope_tci(wa.ele, wadown.ele,
				     dist_to_nbr[np_side]);
		tci_val = log((fabs(wa.wat) * cell_size) / tci_div);
		dseg_put(&tci, &tci_val, r, c);
	    }

	    /* update asp for depression */
	    if (is_swale || fabs(valued) >= threshold) {
		seg_get(&aspflag, (char *)&afdown, dr, dc);
		FLAG_SET(afdown.flag, SWALEFLAG);
		seg_put(&aspflag, (char *)&afdown, dr, dc);
		is_swale = 1;
	    }
	    else {
		seg_get(&aspflag, (char *)&afdown, dr, dc);
		if (er_flag && !is_swale && !FLAG_GET(afdown.flag, RUSLEBLOCKFLAG))
		    slope_length(r, c, dr, dc);
	    }
	}
	seg_put(&aspflag, (char *)&af, r, c);
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
 * Topographic Convergence Index (TCI)
 * tendency of water to accumulate at a given point considering 
 * the gravitational forces to move the water accumulated at that 
 * point further downstream
 * 
 * after Quinn et al. (1991), modified and adapted for the modified 
 * Holmgren MFD algorithm
 * TCI: specific catchment area divided by tangens of slope
 * specific catchment area: total catchment area divided by contour line
 * TCI for D8:     A / (L * tanb)
 * TCI for MFD:    A / (SUM(L_i) * (SUM(tanb_i * weight_i) / SUM(weight_i))
 * 
 * A: total catchment area
 * L_i: contour length towards i_th cell
 * tanb_i: slope = tan(b) towards i_th cell
 * weight_i: weight for flow distribution towards i_th cell
 * ************************************/

int do_cum_mfd(void)
{
    int r, c, dr, dc;
    DCELL value, valued, *wat_nbr;
    double tci_val, tci_div, sum_contour, cell_size;
    POINT point;
    WAT_ALT wa;
    ASP_FLAG af, afdown;
    GW_LARGE_INT killer;
    int threshold;

    /* MFD */
    int mfd_cells, stream_cells, swale_cells, astar_not_set, is_null;
    double *dist_to_nbr, *contour, *weight, sum_weight, max_weight;
    int r_nbr, c_nbr, r_max, c_max, ct_dir, np_side;
    CELL ele, *ele_nbr;
    double prop, max_val;
    int workedon, edge, is_swale, flat;
    char *flag_nbr;
    int asp_r[9] = { 0, -1, -1, -1, 0, 1, 1, 1, 0 };
    int asp_c[9] = { 0, 1, 0, -1, -1, -1, 0, 1, 1 };

    G_message(_("SECTION 3a: Accumulating Surface Flow with MFD."));
    G_debug(1, "MFD convergence factor set to %d.", c_fac);

    /* distances to neighbours */
    dist_to_nbr = (double *)G_malloc(sides * sizeof(double));
    weight = (double *)G_malloc(sides * sizeof(double));
    contour = (double *)G_malloc(sides * sizeof(double));
    
    cell_size = get_dist(dist_to_nbr, contour);

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
	seg_get(&aspflag, (char *)&af, r, c);
	if (af.asp) {
	    dr = r + asp_r[ABS(af.asp)];
	    dc = c + asp_c[ABS(af.asp)];
	}
	/* skip user-defined depressions */
	else
	    dr = dc = -1;

	/* WORKEDFLAG has been set during A* Search
	 * reversed meaning here: 0 = done, 1 = not yet done */
	FLAG_UNSET(af.flag, WORKEDFLAG);
	
	if (dr >= 0 && dr < nrows && dc >= 0 && dc < ncols) {
	    r_max = dr;
	    c_max = dc;

	    seg_get(&watalt, (char *)&wa, r, c);
	    value = wa.wat;

	    /* get weights */
	    max_weight = 0;
	    sum_weight = 0;
	    np_side = -1;
	    mfd_cells = 0;
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
		flag_nbr[ct_dir] = 0;

		/* check if neighbour is within region */
		if (r_nbr >= 0 && r_nbr < nrows && c_nbr >= 0 &&
		    c_nbr < ncols) {

		    if (dr == r_nbr && dc == c_nbr)
			np_side = ct_dir;

		    seg_get(&aspflag, (char *)&afdown, r_nbr, c_nbr);
		    flag_nbr[ct_dir] = afdown.flag;
		    seg_get(&watalt, (char *)&wa, r_nbr, c_nbr);
		    wat_nbr[ct_dir] = wa.wat;
		    ele_nbr[ct_dir] = wa.ele;

		    if (FLAG_GET(flag_nbr[ct_dir], WORKEDFLAG)) {

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
		}
		else
		    edge = 1;
		if (edge)
		    break;
	    }
	    /* do not continue streams along edges, this causes artifacts */
	    if (edge) {
		seg_put(&aspflag, (char *)&af, r, c);
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
	    max_val = -1;
	    tci_div = sum_contour = 0.;

	    if (mfd_cells > 1) {
		prop = 0.0;
		for (ct_dir = 0; ct_dir < sides; ct_dir++) {
		    r_nbr = r + nextdr[ct_dir];
		    c_nbr = c + nextdc[ct_dir];

		    /* check if neighbour is within region */
		    if (r_nbr >= 0 && r_nbr < nrows && c_nbr >= 0 &&
			c_nbr < ncols && weight[ct_dir] > -0.5) {

			if (FLAG_GET(flag_nbr[ct_dir], WORKEDFLAG)) {

			    if (tci_flag) {
				sum_contour += contour[ct_dir];
				tci_div += get_slope_tci(ele, ele_nbr[ct_dir],
				                         dist_to_nbr[ct_dir]) *
					   weight[ct_dir];
			    }

			    weight[ct_dir] = weight[ct_dir] / sum_weight;
			    /* check everything adds up to 1.0 */
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
			}
			else if (ct_dir == np_side) {
			    /* check for consistency with A * path */
			    workedon++;
			}
		    }
		}

		/* adjust main drainage direction to A* path if possible */
		/*if (fabs(wat_nbr[np_side]) >= max_acc) {
		    max_acc = fabs(wat_nbr[np_side]);
		    r_max = dr;
		    c_max = dc;
		} */

		if (fabs(prop - 1.0) > 5E-6f) {
		    G_warning(_("MFD: cumulative proportion of flow distribution not 1.0 but %f"),
			      prop);
		}
		if (tci_flag)
		    tci_div /= sum_weight;
	    }
	    /* SFD-like accumulation */
	    else {
		valued = wat_nbr[np_side];
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
		wa.ele = ele_nbr[np_side];
		seg_put(&watalt, (char *)&wa, dr, dc);

		if (tci_flag) {
		    sum_contour = contour[np_side];
		    tci_div = get_slope_tci(ele, ele_nbr[np_side],
				            dist_to_nbr[np_side]);
		}
	    }

	    /* topographic wetness index ln(a / tan(beta)) */
	    if (tci_flag) {
		tci_val = log((fabs(value) * cell_size) /
		              (sum_contour * tci_div));
		dseg_put(&tci, &tci_val, r, c);
	    }
	}
	seg_put(&aspflag, (char *)&af, r, c);
    }
    G_percent(do_points, do_points, 1);	/* finish it */
    
    if (workedon)
	G_warning(_("MFD: A * path already processed when distributing flow: %d of %"PRI_OFF_T" cells"),
		  workedon, do_points);


    G_message(_("SECTION 3b: Adjusting drainage directions."));

    for (killer = 0; killer < do_points; killer++) {
	G_percent(killer, do_points, 1);
	seg_get(&astar_pts, (char *)&point, 0, killer);
	r = point.r;
	c = point.c;
	seg_get(&aspflag, (char *)&af, r, c);
	if (af.asp) {
	    dr = r + asp_r[ABS(af.asp)];
	    dc = c + asp_c[ABS(af.asp)];
	}
	/* skip user-defined depressions */
	else
	    dr = dc = -1;

	FLAG_SET(af.flag, WORKEDFLAG);
	
	if (dr >= 0 && dr < nrows && dc >= 0 && dc < ncols) {
	    r_max = dr;
	    c_max = dc;

	    seg_get(&watalt, (char *)&wa, r, c);
	    value = wa.wat;

	    /* get max flow accumulation */
	    max_val = -1;
	    stream_cells = 0;
	    swale_cells = 0;
	    ele = wa.ele;
	    is_null = 0;
	    edge = 0;
	    flat = 1;

	    for (ct_dir = 0; ct_dir < sides; ct_dir++) {
		/* get r, c (r_nbr, c_nbr) for neighbours */
		r_nbr = r + nextdr[ct_dir];
		c_nbr = c + nextdc[ct_dir];
		weight[ct_dir] = -1;

		wat_nbr[ct_dir] = 0;
		ele_nbr[ct_dir] = 0;
		flag_nbr[ct_dir] = 0;

		/* check if neighbour is within region */
		if (r_nbr >= 0 && r_nbr < nrows && c_nbr >= 0 &&
		    c_nbr < ncols) {

		    seg_get(&aspflag, (char *)&afdown, r_nbr, c_nbr);
		    flag_nbr[ct_dir] = afdown.flag;
		    seg_get(&watalt, (char *)&wa, r_nbr, c_nbr);
		    wat_nbr[ct_dir] = wa.wat;
		    ele_nbr[ct_dir] = wa.ele;

		    /* check for swale or stream cells */
		    is_swale = FLAG_GET(flag_nbr[ct_dir], SWALEFLAG);
		    if (is_swale)
			swale_cells++;
		    if ((ABS(wat_nbr[ct_dir]) + 0.5) >= threshold &&
		        ele_nbr[ct_dir] > ele)
			stream_cells++;

		    if (!(FLAG_GET(flag_nbr[ct_dir], WORKEDFLAG))) {

			if (ele_nbr[ct_dir] != ele)
			    flat = 0;

			edge = is_null = FLAG_GET(flag_nbr[ct_dir], NULLFLAG);
			if (!is_null && ABS(wa.wat) > max_val) {
			    max_val = ABS(wa.wat);
			    r_max = r_nbr;
			    c_max = c_nbr;
			}
		    }
		}
		else
		    edge = 1;
		if (edge)
		    break;
	    }
	    /* do not continue streams along edges, this causes artifacts */
	    if (edge) {
		is_swale = FLAG_GET(af.flag, SWALEFLAG);
		if (is_swale && af.asp > 0) {
		    af.asp = -1 * drain[r - r_nbr + 1][c - c_nbr + 1];
		}
		seg_put(&aspflag, (char *)&af, r, c);
		continue;
	    }
	    /* update asp */
	    if (dr != r_max || dc != c_max) {
		if (af.asp < 0) {
		    af.asp = -1 * drain[r - r_max + 1][c - c_max + 1];
		}
		else
		    af.asp = drain[r - r_max + 1][c - c_max + 1];
	    }
	    is_swale = FLAG_GET(af.flag, SWALEFLAG);
	    /* start new stream */
	    if (!is_swale && fabs(value) >= threshold && stream_cells < 1 &&
		swale_cells < 1 && !flat) {
		FLAG_SET(af.flag, SWALEFLAG);
		is_swale = 1;
	    }
	    /* continue stream */
	    if (is_swale) {
		seg_get(&aspflag, (char *)&afdown, r_max, c_max);
		FLAG_SET(afdown.flag, SWALEFLAG);
		seg_put(&aspflag, (char *)&afdown, r_max, c_max);
	    }
	    else {
		if (er_flag && !is_swale && !FLAG_GET(af.flag, RUSLEBLOCKFLAG))
		    slope_length(r, c, r_max, c_max);
	    }
	}
	seg_put(&aspflag, (char *)&af, r, c);
    }

    seg_close(&astar_pts);
    
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
