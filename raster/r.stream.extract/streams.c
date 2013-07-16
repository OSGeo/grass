#include <stdlib.h>
#include <math.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include "local_proto.h"

double mfd_pow(double base)
{
    int i;
    double result = base;

    for (i = 2; i <= c_fac; i++) {
	result *= base;
    }
    return result;
}

static int continue_stream(CELL stream_id, int r_max, int c_max,
		    int *stream_no)
{
    CELL curr_stream, stream_nbr, old_stream;
    int r_nbr, c_nbr;
    int asp_r[9] = { 0, -1, -1, -1, 0, 1, 1, 1, 0 };
    int asp_c[9] = { 0, 1, 0, -1, -1, -1, 0, 1, 1 };
    int stream_node_step = 1000;
    ASP_FLAG af;

    G_debug(3, "continue stream");
    
    cseg_get(&stream, &curr_stream, r_max, c_max);

    if (curr_stream <= 0) {
	/* no confluence, just continue */
	G_debug(3, "no confluence, just continue stream");
	curr_stream = stream_id;
	cseg_put(&stream, &curr_stream, r_max, c_max);
	seg_get(&aspflag, (char *)&af, r_max, c_max);
	FLAG_SET(af.flag, STREAMFLAG);
	seg_put(&aspflag, (char *)&af, r_max, c_max);
	return 0;
    }

    G_debug(3, "confluence");
	    
    /* new confluence */
    if (stream_node[curr_stream].r != r_max ||
	stream_node[curr_stream].c != c_max) {
	size_t new_size;
	
	G_debug(3, "new confluence");
	/* set new stream id */
	(*stream_no)++;
	/* add stream node */
	if (*stream_no >= n_alloc_nodes - 1) {
	    n_alloc_nodes += stream_node_step;
	    stream_node =
		(struct snode *)G_realloc(stream_node,
					  n_alloc_nodes *
					  sizeof(struct snode));
	}
	stream_node[*stream_no].r = r_max;
	stream_node[*stream_no].c = c_max;
	stream_node[*stream_no].id = *stream_no;
	stream_node[*stream_no].n_trib = 0;
	stream_node[*stream_no].n_trib_total = 0;
	stream_node[*stream_no].n_alloc = 0;
	stream_node[*stream_no].trib = NULL;
	n_stream_nodes++;

	/* debug */
	if (n_stream_nodes != *stream_no)
	    G_warning(_("BUG: stream_no %d and n_stream_nodes %lld out of sync"),
		      *stream_no, n_stream_nodes);

	stream_node[*stream_no].n_alloc += 2;
	new_size = stream_node[*stream_no].n_alloc * sizeof(int);
	stream_node[*stream_no].trib =
	    (int *)G_realloc(stream_node[*stream_no].trib, new_size);

	/* add the two tributaries */
	G_debug(3, "add tributaries");
	stream_node[*stream_no].trib[stream_node[*stream_no].n_trib++] =
	    curr_stream;
	stream_node[*stream_no].trib[stream_node[*stream_no].n_trib++] =
	    stream_id;

	/* update stream IDs downstream */
	G_debug(3, "update stream IDs downstream");
	r_nbr = r_max;
	c_nbr = c_max;
	old_stream = curr_stream;
	curr_stream = *stream_no;
	cseg_put(&stream, &curr_stream, r_nbr, c_nbr);
	seg_get(&aspflag, (char *)&af, r_nbr, c_nbr);

	while (af.asp > 0) {
	    r_nbr = r_nbr + asp_r[(int)af.asp];
	    c_nbr = c_nbr + asp_c[(int)af.asp];
	    cseg_get(&stream, &stream_nbr, r_nbr, c_nbr);
	    if (stream_nbr != old_stream)
		af.asp = -1;
	    else {
		cseg_put(&stream, &curr_stream, r_nbr, c_nbr);
		seg_get(&aspflag, (char *)&af, r_nbr, c_nbr);
	    }
	}
    }
    else {
	/* stream node already existing here */
	G_debug(3, "existing confluence");
	/* add new tributary to stream node */
	if (stream_node[curr_stream].n_trib >=
	    stream_node[curr_stream].n_alloc) {
	    size_t new_size;

	    stream_node[curr_stream].n_alloc += 2;
	    new_size = stream_node[curr_stream].n_alloc * sizeof(int);
	    stream_node[curr_stream].trib =
		(int *)G_realloc(stream_node[curr_stream].trib, new_size);
	}

	stream_node[curr_stream].trib[stream_node[curr_stream].n_trib++] =
	    stream_id;
    }

    G_debug(3, "%d tribs", stream_node[curr_stream].n_trib);
    if (stream_node[curr_stream].n_trib == 1)
	G_warning(_("BUG: stream node %d has only 1 tributary: %d"), curr_stream,
		  stream_node[curr_stream].trib[0]);

    return 1;
}

/*
 * accumulate surface flow
 */
int do_accum(double d8cut)
{
    int r, c, dr, dc;
    CELL ele_val, *ele_nbr;
    DCELL value, *wat_nbr;
    struct Cell_head window;
    int mfd_cells, astar_not_set;
    double *dist_to_nbr, *weight, sum_weight, max_weight;
    double dx, dy;
    int r_nbr, c_nbr, ct_dir, np_side;
    int is_worked;
    double prop;
    int edge;
    int asp_r[9] = { 0, -1, -1, -1, 0, 1, 1, 1, 0 };
    int asp_c[9] = { 0, 1, 0, -1, -1, -1, 0, 1, 1 };
    int nextdr[8] = { 1, -1, 0, 0, -1, 1, 1, -1 };
    int nextdc[8] = { 0, 0, -1, 1, 1, -1, 1, -1 };
    GW_LARGE_INT workedon, killer;
    char *flag_nbr;
    POINT astarpoint;
    WAT_ALT wa;
    ASP_FLAG af, af_nbr;

    G_message(_("Calculating flow accumulation..."));

    /* distances to neighbours */
    dist_to_nbr = (double *)G_malloc(sides * sizeof(double));
    weight = (double *)G_malloc(sides * sizeof(double));
    flag_nbr = (char *)G_malloc(sides * sizeof(char));
    wat_nbr = (DCELL *)G_malloc(sides * sizeof(DCELL));
    ele_nbr = (CELL *)G_malloc(sides * sizeof(CELL));

    G_get_set_window(&window);

    for (ct_dir = 0; ct_dir < sides; ct_dir++) {
	/* get r, c (r_nbr, c_nbr) for neighbours */
	r_nbr = nextdr[ct_dir];
	c_nbr = nextdc[ct_dir];
	/* account for rare cases when ns_res != ew_res */
	dy = abs(r_nbr) * window.ns_res;
	dx = abs(c_nbr) * window.ew_res;
	if (ct_dir < 4)
	    dist_to_nbr[ct_dir] = dx + dy;
	else
	    dist_to_nbr[ct_dir] = sqrt(dx * dx + dy * dy);
    }

    /* distribute and accumulate */
    for (killer = 0; killer < n_points; killer++) {

	G_percent(killer, n_points, 1);
	
	seg_get(&astar_pts, (char *)&astarpoint, 0, killer);
	r = astarpoint.r;
	c = astarpoint.c;

	seg_get(&aspflag, (char *)&af, r, c);

	/* do not distribute flow along edges or out of real depressions */
	if (af.asp <= 0) {
	    FLAG_UNSET(af.flag, WORKEDFLAG);
	    seg_put(&aspflag, (char *)&af, r, c);
	    continue;
	}

	if (af.asp) {
	    dr = r + asp_r[abs((int)af.asp)];
	    dc = c + asp_c[abs((int)af.asp)];
	}

	seg_get(&watalt, (char *)&wa, r, c);
	value = wa.wat;

	/* WORKEDFLAG has been set during A* Search
	 * reversed meaning here: 0 = done, 1 = not yet done */
	FLAG_UNSET(af.flag, WORKEDFLAG);
	seg_put(&aspflag, (char *)&af, r, c);

	/***************************************/
	/*  get weights for flow distribution  */
	/***************************************/

	max_weight = 0;
	sum_weight = 0;
	np_side = -1;
	mfd_cells = 0;
	astar_not_set = 1;
	ele_val = wa.ele;
	edge = 0;
	/* this loop is needed to get the sum of weights */
	for (ct_dir = 0; ct_dir < sides; ct_dir++) {
	    /* get r_nbr, c_nbr for neighbours */
	    r_nbr = r + nextdr[ct_dir];
	    c_nbr = c + nextdc[ct_dir];
	    weight[ct_dir] = -1;
	    wat_nbr[ct_dir] = 0;
	    ele_nbr[ct_dir] = 0;

	    /* check that neighbour is within region */
	    if (r_nbr >= 0 && r_nbr < nrows && c_nbr >= 0 && c_nbr < ncols) {

		seg_get(&aspflag, (char *)&af_nbr, r_nbr, c_nbr);
		flag_nbr[ct_dir] = af_nbr.flag;
		if ((edge = FLAG_GET(flag_nbr[ct_dir], NULLFLAG)))
		    break;
		seg_get(&watalt, (char *)&wa, r_nbr, c_nbr);
		wat_nbr[ct_dir] = wa.wat;
		ele_nbr[ct_dir] = wa.ele;

		/* WORKEDFLAG has been set during A* Search
		 * reversed meaning here: 0 = done, 1 = not yet done */
		is_worked = FLAG_GET(flag_nbr[ct_dir], WORKEDFLAG) == 0;
		if (is_worked == 0) {
		    if (ele_nbr[ct_dir] <= ele_val) {
			if (ele_nbr[ct_dir] < ele_val) {
			    weight[ct_dir] =
				mfd_pow((ele_val -
					 ele_nbr[ct_dir]) / dist_to_nbr[ct_dir]);
			}
			if (ele_nbr[ct_dir] == ele_val) {
			    weight[ct_dir] =
				mfd_pow(0.5 / dist_to_nbr[ct_dir]);
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
	    G_debug(3, "edge");
	    continue;
	}

	/* honour A * path 
	 * mfd_cells == 0: fine, SFD along A * path
	 * mfd_cells == 1 && astar_not_set == 0: fine, SFD along A * path
	 * mfd_cells > 0 && astar_not_set == 1: A * path not included, add to mfd_cells
	 */

	/************************************/
	/*  distribute and accumulate flow  */
	/************************************/

	/* MFD, A * path not included, add to mfd_cells */
	if (mfd_cells > 0 && astar_not_set == 1) {
	    mfd_cells++;
	    sum_weight += max_weight;
	    weight[np_side] = max_weight;
	}

	/* use SFD (D8) if d8cut threshold exceeded */
	if (fabs(value) > d8cut)
	    mfd_cells = 0;

	if (mfd_cells > 1) {
	    prop = 0.0;
	    for (ct_dir = 0; ct_dir < sides; ct_dir++) {
		/* get r, c (r_nbr, c_nbr) for neighbours */
		r_nbr = r + nextdr[ct_dir];
		c_nbr = c + nextdc[ct_dir];

		/* check that neighbour is within region */
		if (r_nbr >= 0 && r_nbr < nrows && c_nbr >= 0 &&
		    c_nbr < ncols && weight[ct_dir] > -0.5) {
		    is_worked = FLAG_GET(flag_nbr[ct_dir], WORKEDFLAG) == 0;
		    if (is_worked == 0) {

			weight[ct_dir] = weight[ct_dir] / sum_weight;
			/* check everything sums up to 1.0 */
			prop += weight[ct_dir];

			wa.wat = wat_nbr[ct_dir] + value * weight[ct_dir];
			wa.ele = ele_nbr[ct_dir];
			seg_put(&watalt, (char *)&wa, r_nbr, c_nbr);
		    }
		    else if (ct_dir == np_side) {
			/* check for consistency with A * path */
			workedon++;
		    }
		}
	    }
	    if (fabs(prop - 1.0) > 5E-6f) {
		G_warning(_("MFD: cumulative proportion of flow distribution not 1.0 but %f"),
			  prop);
	    }
	}
	/* get out of depression in SFD mode */
	else {
	    wa.wat = wat_nbr[np_side] + value;
	    wa.ele = ele_nbr[np_side];
	    seg_put(&watalt, (char *)&wa, dr, dc);
	}
    }
    G_percent(1, 1, 2);

    G_free(dist_to_nbr);
    G_free(weight);
    G_free(wat_nbr);
    G_free(ele_nbr);
    G_free(flag_nbr);

    return 1;
}

/*
 * extracts streams for threshold, accumulation is provided
 */
int extract_streams(double threshold, double mont_exp, int internal_acc)
{
    int r, c, dr, dc;
    CELL is_swale, ele_val, *ele_nbr;
    DCELL value, valued, *wat_nbr;
    struct Cell_head window;
    int mfd_cells, stream_cells, swale_cells;
    double *dist_to_nbr;
    double dx, dy;
    int r_nbr, c_nbr, r_max, c_max, ct_dir, np_side, max_side;
    int is_worked;
    double max_acc;
    int edge, flat;
    int asp_r[9] = { 0, -1, -1, -1, 0, 1, 1, 1, 0 };
    int asp_c[9] = { 0, 1, 0, -1, -1, -1, 0, 1, 1 };
    int nextdr[8] = { 1, -1, 0, 0, -1, 1, 1, -1 };
    int nextdc[8] = { 0, 0, -1, 1, 1, -1, 1, -1 };
    /* sides */
    /*
     *  | 7 | 1 | 4 |
     *  | 2 |   | 3 |
     *  | 5 | 0 | 6 |
     */
    GW_LARGE_INT workedon, killer;
    int stream_no = 0, stream_node_step = 1000;
    double slope, diag;
    char *flag_nbr;
    POINT astarpoint;
    WAT_ALT wa;
    ASP_FLAG af, af_nbr;

    G_message(_("Extracting streams..."));

    /* init stream nodes */
    n_alloc_nodes = stream_node_step;
    stream_node =
	(struct snode *)G_malloc(n_alloc_nodes * sizeof(struct snode));
    n_stream_nodes = 0;

    /* init outlet nodes */
    n_alloc_outlets = stream_node_step;
    outlets =
	(POINT *)G_malloc(n_alloc_outlets * sizeof(POINT));
    n_outlets = 0;

    /* distances to neighbours */
    dist_to_nbr = (double *)G_malloc(sides * sizeof(double));
    flag_nbr = (char *)G_malloc(sides * sizeof(char));
    wat_nbr = (DCELL *)G_malloc(sides * sizeof(DCELL));
    ele_nbr = (CELL *)G_malloc(sides * sizeof(CELL));

    G_get_set_window(&window);

    for (ct_dir = 0; ct_dir < sides; ct_dir++) {
	/* get r, c (r_nbr, c_nbr) for neighbours */
	r_nbr = nextdr[ct_dir];
	c_nbr = nextdc[ct_dir];
	/* account for rare cases when ns_res != ew_res */
	dy = abs(r_nbr) * window.ns_res;
	dx = abs(c_nbr) * window.ew_res;
	if (ct_dir < 4)
	    dist_to_nbr[ct_dir] = dx + dy;
	else
	    dist_to_nbr[ct_dir] = sqrt(dx * dx + dy * dy);
    }

    diag = sqrt(2);

    workedon = 0;

    /* extract streams */
    for (killer =  0; killer < n_points; killer++) {
	G_percent(killer, n_points, 1);
	
	seg_get(&astar_pts, (char *)&astarpoint, 0, killer);
	r = astarpoint.r;
	c = astarpoint.c;

	seg_get(&aspflag, (char *)&af, r, c);
	/* internal acc: SET, external acc: UNSET */
	if (internal_acc)
	    FLAG_SET(af.flag, WORKEDFLAG);
	else
	    FLAG_UNSET(af.flag, WORKEDFLAG);
	seg_put(&aspflag, (char *)&af, r, c);

	/* do not distribute flow along edges */
	if (af.asp <= 0) {
	    G_debug(3, "edge");
	    is_swale = FLAG_GET(af.flag, STREAMFLAG);
	    if (is_swale) {
		G_debug(2, "edge outlet");
		/* add outlet point */
		if (n_outlets >= n_alloc_outlets) {
		    n_alloc_outlets += stream_node_step;
		    outlets =
			(POINT *)G_realloc(outlets,
						  n_alloc_outlets *
						  sizeof(POINT));
		}
		outlets[n_outlets].r = r;
		outlets[n_outlets].c = c;
		n_outlets++;
	    }

	    if (af.asp == 0) {
		/* can only happen with real depressions */
		if (!have_depressions)
		    G_fatal_error(_("Bug in stream extraction"));
		G_debug(2, "bottom of real depression");
	    } 
	    continue;
	}

	if (af.asp) {
	    dr = r + asp_r[abs((int)af.asp)];
	    dc = c + asp_c[abs((int)af.asp)];
	}
	else {
	    /* can only happen with real depressions,
	     * but should not get to here */
	    dr = r;
	    dc = c;
	}

	r_nbr = r_max = dr;
	c_nbr = c_max = dc;

	seg_get(&watalt, (char *)&wa, r, c);
	value = wa.wat;

	/**********************************/
	/*  find main drainage direction  */
	/**********************************/

	max_acc = -1;
	max_side = np_side = -1;
	mfd_cells = 0;
	stream_cells = 0;
	swale_cells = 0;
	ele_val = wa.ele;
	edge = 0;
	flat = 1;
	/* find main drainage direction */
	for (ct_dir = 0; ct_dir < sides; ct_dir++) {
	    /* get r_nbr, c_nbr for neighbours */
	    r_nbr = r + nextdr[ct_dir];
	    c_nbr = c + nextdc[ct_dir];
	    wat_nbr[ct_dir] = 0;
	    ele_nbr[ct_dir] = 0;
	    flag_nbr[ct_dir] = 0;

	    /* check that neighbour is within region */
	    if (r_nbr >= 0 && r_nbr < nrows && c_nbr >= 0 && c_nbr < ncols) {

		if (dr == r_nbr && dc == c_nbr)
		    np_side = ct_dir;

		seg_get(&aspflag, (char *)&af_nbr, r_nbr, c_nbr);
		flag_nbr[ct_dir] = af_nbr.flag;
		if ((edge = FLAG_GET(flag_nbr[ct_dir], NULLFLAG)))
		    break;
		seg_get(&watalt, (char *)&wa, r_nbr, c_nbr);
		wat_nbr[ct_dir] = wa.wat;
		ele_nbr[ct_dir] = wa.ele;

		/* check for swale cells */
		is_swale = FLAG_GET(flag_nbr[ct_dir], STREAMFLAG);
		if (is_swale)
		    swale_cells++;

		/* check for stream cells */
		valued = fabs(wat_nbr[ct_dir]);
		/* check all upstream neighbours */
		if (valued >= threshold && ct_dir != np_side &&
		    ele_nbr[ct_dir] > ele_val)
		    stream_cells++;

		is_worked = FLAG_GET(flag_nbr[ct_dir], WORKEDFLAG);
		if (!internal_acc)
		    is_worked = is_worked == 0;

		if (is_worked == 0) {
		    if (ele_nbr[ct_dir] != ele_val)
			flat = 0;
		    if (ele_nbr[ct_dir] <= ele_val) {

			mfd_cells++;

			/* set main drainage direction */
			if (valued >= max_acc) {
			    max_acc = valued;
			    r_max = r_nbr;
			    c_max = c_nbr;
			    max_side = ct_dir;
			}
		    }
		}
		else if (ct_dir == np_side && !edge) {
		    /* check for consistency with A * path */
		    workedon++;
		}
	    }
	    else
		edge = 1;
	    if (edge)
		break;
	}

	is_swale = FLAG_GET(af.flag, STREAMFLAG);

	/* do not continue streams along edges, these are artifacts */
	if (edge) {
	    G_debug(3, "edge");
	    if (is_swale) {
		G_debug(2, "edge outlet");
		/* add outlet point */
		if (n_outlets >= n_alloc_outlets) {
		    n_alloc_outlets += stream_node_step;
		    outlets =
			(POINT *)G_realloc(outlets,
						  n_alloc_outlets *
						  sizeof(POINT));
		}
		outlets[n_outlets].r = r;
		outlets[n_outlets].c = c;
		n_outlets++;
		if (af.asp > 0) {
		    af.asp = -1 * drain[r - r_nbr + 1][c - c_nbr + 1];
		    seg_put(&aspflag, (char *)&af, r, c);
		}
	    }
	    continue;
	}

	if (np_side < 0)
	    G_fatal_error("np_side < 0");
	    
	/* set main drainage direction to A* path if possible */
	if (mfd_cells > 0 && max_side != np_side) {
	    if (fabs(wat_nbr[np_side] >= max_acc)) {
		max_acc = fabs(wat_nbr[np_side]);
		r_max = dr;
		c_max = dc;
		max_side = np_side;
	    }
	}
	if (mfd_cells == 0) {
	    flat = 0;
	    r_max = dr;
	    c_max = dc;
	    max_side = np_side;
	}

	/* update aspect */
	/* r_max == r && c_max == c should not happen */
	if ((r_max != dr || c_max != dc) && (r_max != r || c_max != c)) {
	    af.asp = drain[r - r_max + 1][c - c_max + 1];
	    seg_put(&aspflag, (char *)&af, r, c);
	}

	/**********************/
	/*  start new stream  */
	/**********************/

	/* Montgomery's stream initiation acc * (tan(slope))^mont_exp */
	/* uses whatever unit is accumulation */
	if (mont_exp > 0) {
	    if (r_max == r && c_max == c)
		G_warning
		    (_("Can't use Montgomery's method, no stream direction found"));
	    else {
		slope = (double)(ele_val - ele_nbr[max_side]) / ele_scale;

		if (max_side > 3)
		    slope /= diag;

		value *= pow(fabs(slope), mont_exp);
	    }
	}

	if (!is_swale && fabs(value) >= threshold && stream_cells < 1 &&
	    swale_cells < 1 && !flat) {
	    G_debug(2, "start new stream");
	    is_swale = ++stream_no;
	    cseg_put(&stream, &is_swale, r, c);
	    FLAG_SET(af.flag, STREAMFLAG);
	    seg_put(&aspflag, (char *)&af, r, c);
	    /* add stream node */
	    if (stream_no >= n_alloc_nodes - 1) {
		n_alloc_nodes += stream_node_step;
		stream_node =
		    (struct snode *)G_realloc(stream_node,
					      n_alloc_nodes *
					      sizeof(struct snode));
	    }
	    stream_node[stream_no].r = r;
	    stream_node[stream_no].c = c;
	    stream_node[stream_no].id = stream_no;
	    stream_node[stream_no].n_trib = 0;
	    stream_node[stream_no].n_trib_total = 0;
	    stream_node[stream_no].n_alloc = 0;
	    stream_node[stream_no].trib = NULL;
	    n_stream_nodes++;

	    /* debug */
	    if (n_stream_nodes != stream_no)
		G_warning(_("BUG: stream_no %d and n_stream_nodes %lld out of sync"),
			  stream_no, n_stream_nodes);
	}

	/*********************/
	/*  continue stream  */
	/*********************/

	if (is_swale > 0) {
	    cseg_get(&stream, &is_swale, r, c);
	    if (r_max == r && c_max == c) {
		/* can't continue stream, add outlet point
		 * r_max == r && c_max == c should not happen */
		G_debug(1, "can't continue stream at r %d c %d", r, c);

		if (n_outlets >= n_alloc_outlets) {
		    n_alloc_outlets += stream_node_step;
		    outlets =
			(POINT *)G_malloc(n_alloc_outlets *
						 sizeof(POINT));
		}
		outlets[n_outlets].r = r;
		outlets[n_outlets].c = c;
		n_outlets++;
	    }
	    else {
		continue_stream(is_swale, r_max, c_max, &stream_no);
	    }
	}
    }
    G_percent(1, 1, 2);
    if (workedon)
	G_warning(_("MFD: A * path already processed when setting drainage direction: %lld of %lld cells"),
		  workedon, n_points);

    G_free(dist_to_nbr);
    G_free(wat_nbr);
    G_free(ele_nbr);
    G_free(flag_nbr);

    G_debug(1, "%lld outlets", n_outlets);
    G_debug(1, "%lld nodes", n_stream_nodes);
    G_debug(1, "%d streams", stream_no);

    return 1;
}
