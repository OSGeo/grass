#include <stdlib.h>
#include <string.h>
#include "Gwater.h"
#include <grass/gis.h>
#include <grass/glocale.h>

int ele_round(double);

int init_vars(int argc, char *argv[])
{
    SHORT r, c;
    CELL *buf, alt_value, wat_value, asp_value, block_value;
    DCELL dvalue;
    void *elebuf, *ptr;
    int fd, index, ele_map_type;
    size_t ele_size;
    char MASK_flag;

    G_gisinit(argv[0]);
    ele_flag = wat_flag = asp_flag = pit_flag = run_flag = ril_flag = 0;
    ob_flag = bas_flag = seg_flag = haf_flag = arm_flag = dis_flag = 0;
    zero = sl_flag = sg_flag = ls_flag = er_flag = bas_thres = 0;
    one = 1;
    nxt_avail_pt = 0;
    /* dep_flag = 0; */
    max_length = d_zero = 0.0;
    d_one = 1.0;
    ril_value = -1.0;
    /* dep_slope = 0.0; */
    sides = 8;
    mfd = 1;
    c_fac = 5;
    abs_acc = 0;
    ele_scale = 1;
    for (r = 1; r < argc; r++) {
	if (sscanf(argv[r], "el=%[^\n]", ele_name) == 1)
	    ele_flag++;
	else if (sscanf(argv[r], "ac=%[^\n]", wat_name) == 1)
	    wat_flag++;
	else if (sscanf(argv[r], "dr=%[^\n]", asp_name) == 1)
	    asp_flag++;
	else if (sscanf(argv[r], "de=%[^\n]", pit_name) == 1)
	    pit_flag++;
	else if (sscanf(argv[r], "t=%d", &bas_thres) == 1) ;
	else if (sscanf(argv[r], "ms=%lf", &max_length) == 1) ;
	else if (sscanf(argv[r], "ba=%[^\n]", bas_name) == 1)
	    bas_flag++;
	else if (sscanf(argv[r], "se=%[^\n]", seg_name) == 1)
	    seg_flag++;
	else if (sscanf(argv[r], "ha=%[^\n]", haf_name) == 1)
	    haf_flag++;
	else if (sscanf(argv[r], "ov=%[^\n]", run_name) == 1)
	    run_flag++;
	else if (sscanf(argv[r], "ar=%[^\n]", arm_name) == 1)
	    arm_flag++;
	else if (sscanf(argv[r], "di=%[^\n]", dis_name) == 1)
	    dis_flag++;
	else if (sscanf(argv[r], "sl=%[^\n]", sl_name) == 1)
	    sl_flag++;
	else if (sscanf(argv[r], "S=%[^\n]", sg_name) == 1)
	    sg_flag++;
	else if (sscanf(argv[r], "LS=%[^\n]", ls_name) == 1)
	    ls_flag++;
	else if (sscanf(argv[r], "ob=%[^\n]", ob_name) == 1)
	    ob_flag++;
	else if (sscanf(argv[r], "r=%[^\n]", ril_name) == 1) {
	    if (sscanf(ril_name, "%lf", &ril_value) == 0) {
		ril_value = -1.0;
		ril_flag++;
	    }
	}
	/* else if (sscanf (argv[r], "sd=%[^\n]", dep_name) == 1) dep_flag++; */
	else if (sscanf(argv[r], "-%d", &sides) == 1) {
	    if (sides != 4)
		usage(argv[0]);
	}
	else if (sscanf(argv[r], "conv=%d", &c_fac) == 1) ;
	else if (strcmp(argv[r], "-s") == 0)
	    mfd = 0;
	else if (strcmp(argv[r], "-a") == 0)
	    abs_acc = 1;
	else
	    usage(argv[0]);
    }
    if (mfd == 1 && (c_fac < 1 || c_fac > 10)) {
	G_fatal_error("Convergence factor must be between 1 and 10.");
    }
    if ((ele_flag != 1)
	||
	((arm_flag == 1) &&
	 ((bas_thres <= 0) || ((haf_flag != 1) && (bas_flag != 1))))
	||
	((bas_thres <= 0) &&
	 ((bas_flag == 1) || (seg_flag == 1) || (haf_flag == 1) ||
	  (sl_flag == 1) || (sg_flag == 1) || (ls_flag == 1)))
	)
	usage(argv[0]);
    tot_parts = 4;
    if (ls_flag || sg_flag)
	tot_parts++;
    if (bas_thres > 0)
	tot_parts++;
    G_message(_("SECTION 1a (of %1d): Initiating Memory."), tot_parts);
    this_mapset = G_mapset();
    if (sl_flag || sg_flag || ls_flag)
	er_flag = 1;
    /* for sd factor
       if (dep_flag)        {
       if (sscanf (dep_name, "%lf", &dep_slope) != 1)       {
       dep_mapset = do_exist (dep_name);
       dep_flag = -1;
       }
       }
     */
    G_get_set_window(&window);
    nrows = G_window_rows();
    ncols = G_window_cols();
    total_cells = nrows * ncols;
    if (max_length <= d_zero)
	max_length = 10 * nrows * window.ns_res + 10 * ncols * window.ew_res;
    if (window.ew_res < window.ns_res)
	half_res = .5 * window.ew_res;
    else
	half_res = .5 * window.ns_res;
    diag = sqrt(window.ew_res * window.ew_res +
		window.ns_res * window.ns_res);
    if (sides == 4)
	diag *= 0.5;

    alt =
	(CELL *) G_malloc(sizeof(CELL) * size_array(&alt_seg, nrows, ncols));
    if (er_flag) {
	r_h =
	    (CELL *) G_malloc(sizeof(CELL) * size_array(&r_h_seg, nrows, ncols));
    }

    swale = flag_create(nrows, ncols);
    in_list = flag_create(nrows, ncols);
    worked = flag_create(nrows, ncols);

    /* open elevation input */
    fd = G_open_cell_old(ele_name, "");
    if (fd < 0) {
	G_fatal_error(_("unable to open elevation map layer"));
    }

    ele_map_type = G_get_raster_map_type(fd);
    ele_size = G_raster_size(ele_map_type);
    elebuf = G_allocate_raster_buf(ele_map_type);

    if (ele_map_type == FCELL_TYPE || ele_map_type == DCELL_TYPE)
	ele_scale = 1000; 	/* should be enough to do the trick */

    /* read elevation input and mark NULL/masked cells */
    MASK_flag = 0;
    do_points = nrows * ncols;
    for (r = 0; r < nrows; r++) {
	G_get_raster_row(fd, elebuf, r, ele_map_type);
	ptr = elebuf;
	for (c = 0; c < ncols; c++) {
	    index = SEG_INDEX(alt_seg, r, c);

	    /* all flags need to be manually set to zero */
	    flag_unset(swale, r, c);
	    flag_unset(in_list, r, c);
	    flag_unset(worked, r, c);

	    /* check for masked and NULL cells */
	    if (G_is_null_value(ptr, ele_map_type)) {
		FLAG_SET(worked, r, c);
		FLAG_SET(in_list, r, c);
		G_set_c_null_value(&alt_value, 1);
		do_points--;
	    }
	    else {
		if (ele_map_type == CELL_TYPE) {
		    alt_value = *((CELL *)ptr);
		}
		else if (ele_map_type == FCELL_TYPE) {
		    dvalue = *((FCELL *)ptr);
		    dvalue *= ele_scale;
		    alt_value = ele_round(dvalue);
		}
		else if (ele_map_type == DCELL_TYPE) {
		    dvalue = *((DCELL *)ptr);
		    dvalue *= ele_scale;
		    alt_value = ele_round(dvalue);
		}
	    }
	    alt[index] = alt_value;
	    if (er_flag) {
		r_h[index] = alt_value;
	    }
	    ptr = G_incr_void_ptr(ptr, ele_size);
	}
    }
    G_close_cell(fd);
    G_free(elebuf);
    if (do_points < nrows * ncols)
	MASK_flag = 1;

    /* initialize flow accumulation ... */
    wat =
	(DCELL *) G_malloc(sizeof(DCELL) *
			   size_array(&wat_seg, nrows, ncols));

    buf = G_allocate_cell_buf();
    if (run_flag) {
	/* ... with input map flow: amount of overland flow per cell */
	fd = G_open_cell_old(run_name, "");
	if (fd < 0) {
	    G_fatal_error(_("unable to open runoff map layer"));
	}
	for (r = 0; r < nrows; r++) {
	    G_get_c_raster_row(fd, buf, r);
	    for (c = 0; c < ncols; c++) {
		if (MASK_flag) {
		    index = FLAG_GET(worked, r, c);
		    if (!index)
			wat[SEG_INDEX(wat_seg, r, c)] = buf[c];
		    else
			wat[SEG_INDEX(wat_seg, r, c)] = 0.0;
		}
		else
		    wat[SEG_INDEX(wat_seg, r, c)] = buf[c];
	    }
	}
	G_close_cell(fd);
    }
    else {
	/* ... with 1.0 */
	for (r = 0; r < nrows; r++) {
	    for (c = 0; c < ncols; c++) {
		if (MASK_flag) {
		    index = FLAG_GET(worked, r, c);
		    if (!index)
			wat[SEG_INDEX(wat_seg, r, c)] = 1.0;
		}
		else
		    wat[SEG_INDEX(wat_seg, r, c)] = 1.0;
	    }
	}
    }
    asp =
	(CELL *) G_malloc(size_array(&asp_seg, nrows, ncols) * sizeof(CELL));

    /* depression: drainage direction will be set to zero later */
    if (pit_flag) {
	fd = G_open_cell_old(pit_name, "");
	if (fd < 0) {
	    G_fatal_error(_("unable to open depression map layer"));
	}
	for (r = 0; r < nrows; r++) {
	    G_get_c_raster_row(fd, buf, r);
	    for (c = 0; c < ncols; c++) {
		asp_value = buf[c];
		if (!G_is_c_null_value(&asp_value) && asp_value)
		    asp[SEG_INDEX(asp_seg, r, c)] = 1;
	    }
	}
	G_close_cell(fd);
    }

    /* this is also creating streams... */
    if (ob_flag) {
	fd = G_open_cell_old(ob_name, "");
	if (fd < 0) {
	    G_fatal_error(_("unable to open blocking map layer"));
	}
	for (r = 0; r < nrows; r++) {
	    G_get_c_raster_row(fd, buf, r);
	    for (c = 0; c < ncols; c++) {
		block_value = buf[c];
		if (!G_is_c_null_value(&block_value) && block_value)
		    FLAG_SET(swale, r, c);
	    }
	}
	G_close_cell(fd);
    }
    G_free(buf);

    if (ril_flag) {
	ril_fd = G_open_cell_old(ril_name, "");
	if (ril_fd < 0) {
	    G_fatal_error(_("unable to open rill map layer"));
	}
    }

    /* RUSLE: LS and/or S factor */
    if (er_flag) {
	s_l =
	    (double *)G_malloc(size_array(&s_l_seg, nrows, ncols) *
			       sizeof(double));
    }

    if (sg_flag) {
	s_g =
	    (double *)G_malloc(size_array(&s_g_seg, nrows, ncols) *
			       sizeof(double));
    }
    if (ls_flag) {
	l_s =
	    (double *)G_malloc(size_array(&l_s_seg, nrows, ncols) *
			       sizeof(double));
    }

    astar_pts = (POINT *) G_malloc(do_points * sizeof(POINT));

    /* heap_index will track astar_pts in ternary min-heap */
    /* heap_index is one-based */
    heap_index = (int *)G_malloc((do_points + 1) * sizeof(int));

    G_message(_("SECTION 1b (of %1d): Determining Offmap Flow."), tot_parts);

    /* heap is empty */
    heap_size = 0;

    first_astar = first_cum = -1;
    if (MASK_flag) {
	for (r = 0; r < nrows; r++) {
	    G_percent(r, nrows, 3);
	    for (c = 0; c < ncols; c++) {
		if (FLAG_GET(worked, r, c)) {
		    wat[SEG_INDEX(wat_seg, r, c)] = 0;
		}
		else {
		    if (er_flag)
			s_l[SEG_INDEX(s_l_seg, r, c)] = half_res;
		    asp_value = asp[SEG_INDEX(asp_seg, r, c)];
		    if (r == 0 || c == 0 || r == nrows - 1 ||
			c == ncols - 1 || asp_value != 0) {
			wat_value = wat[SEG_INDEX(wat_seg, r, c)];
			if (wat_value > 0)
			    wat[SEG_INDEX(wat_seg, r, c)] = -wat_value;
			/* set depression */
			if (asp_value) {
			    asp_value = 0;
			    wat[SEG_INDEX(wat_seg, r, c)] = ABS(wat_value);
			}
			else if (r == 0)
			    asp_value = -2;
			else if (c == 0)
			    asp_value = -4;
			else if (r == nrows - 1)
			    asp_value = -6;
			else if (c == ncols - 1)
			    asp_value = -8;
			asp[SEG_INDEX(asp_seg, r, c)] = asp_value;
			alt_value = alt[SEG_INDEX(alt_seg, r, c)];
			add_pt(r, c, alt_value, alt_value);
		    }
		    else if (FLAG_GET(worked, r - 1, c)) {
			alt_value = alt[SEG_INDEX(alt_seg, r, c)];
			add_pt(r, c, alt_value, alt_value);
			asp[SEG_INDEX(asp_seg, r, c)] = -2;
			wat_value = wat[SEG_INDEX(wat_seg, r, c)];
			if (wat_value > 0)
			    wat[SEG_INDEX(wat_seg, r, c)] = -wat_value;
		    }
		    else if (FLAG_GET(worked, r + 1, c)) {
			alt_value = alt[SEG_INDEX(alt_seg, r, c)];
			add_pt(r, c, alt_value, alt_value);
			asp[SEG_INDEX(asp_seg, r, c)] = -6;
			wat_value = wat[SEG_INDEX(wat_seg, r, c)];
			if (wat_value > 0)
			    wat[SEG_INDEX(wat_seg, r, c)] = -wat_value;
		    }
		    else if (FLAG_GET(worked, r, c - 1)) {
			alt_value = alt[SEG_INDEX(alt_seg, r, c)];
			add_pt(r, c, alt_value, alt_value);
			asp[SEG_INDEX(asp_seg, r, c)] = -4;
			wat_value = wat[SEG_INDEX(wat_seg, r, c)];
			if (wat_value > 0)
			    wat[SEG_INDEX(wat_seg, r, c)] = -wat_value;
		    }
		    else if (FLAG_GET(worked, r, c + 1)) {
			alt_value = alt[SEG_INDEX(alt_seg, r, c)];
			add_pt(r, c, alt_value, alt_value);
			asp[SEG_INDEX(asp_seg, r, c)] = -8;
			wat_value = wat[SEG_INDEX(wat_seg, r, c)];
			if (wat_value > 0)
			    wat[SEG_INDEX(wat_seg, r, c)] = -wat_value;
		    }
		    else if (sides == 8 && FLAG_GET(worked, r - 1, c - 1)) {
			alt_value = alt[SEG_INDEX(alt_seg, r, c)];
			add_pt(r, c, alt_value, alt_value);
			asp[SEG_INDEX(asp_seg, r, c)] = -3;
			wat_value = wat[SEG_INDEX(wat_seg, r, c)];
			if (wat_value > 0)
			    wat[SEG_INDEX(wat_seg, r, c)] = -wat_value;
		    }
		    else if (sides == 8 && FLAG_GET(worked, r - 1, c + 1)) {
			alt_value = alt[SEG_INDEX(alt_seg, r, c)];
			add_pt(r, c, alt_value, alt_value);
			asp[SEG_INDEX(asp_seg, r, c)] = -1;
			wat_value = wat[SEG_INDEX(wat_seg, r, c)];
			if (wat_value > 0)
			    wat[SEG_INDEX(wat_seg, r, c)] = -wat_value;
		    }
		    else if (sides == 8 && FLAG_GET(worked, r + 1, c - 1)) {
			alt_value = alt[SEG_INDEX(alt_seg, r, c)];
			add_pt(r, c, alt_value, alt_value);
			asp[SEG_INDEX(asp_seg, r, c)] = -5;
			wat_value = wat[SEG_INDEX(wat_seg, r, c)];
			if (wat_value > 0)
			    wat[SEG_INDEX(wat_seg, r, c)] = -wat_value;
		    }
		    else if (sides == 8 && FLAG_GET(worked, r + 1, c + 1)) {
			alt_value = alt[SEG_INDEX(alt_seg, r, c)];
			add_pt(r, c, alt_value, alt_value);
			asp[SEG_INDEX(asp_seg, r, c)] = -7;
			wat_value = wat[SEG_INDEX(wat_seg, r, c)];
			if (wat_value > 0)
			    wat[SEG_INDEX(wat_seg, r, c)] = -wat_value;
		    }
		}
	    }
	}
    }
    else {
	for (r = 0; r < nrows; r++) {
	    G_percent(r, nrows, 3);
	    for (c = 0; c < ncols; c++) {
		if (er_flag)
		    s_l[SEG_INDEX(s_l_seg, r, c)] = half_res;
		asp_value = asp[SEG_INDEX(asp_seg, r, c)];
		if (r == 0 || c == 0 || r == nrows - 1 ||
		    c == ncols - 1 || asp_value != 0) {
		    wat_value = wat[SEG_INDEX(wat_seg, r, c)];
		    if (wat_value > 0) {
			wat[SEG_INDEX(wat_seg, r, c)] = -wat_value;
		    }
		    /* set depression */
		    if (asp_value) {
			asp_value = 0;
			wat[SEG_INDEX(wat_seg, r, c)] = ABS(wat_value);
		    }
		    else if (r == 0)
			asp_value = -2;
		    else if (c == 0)
			asp_value = -4;
		    else if (r == nrows - 1)
			asp_value = -6;
		    else if (c == ncols - 1)
			asp_value = -8;
		    asp[SEG_INDEX(asp_seg, r, c)] = asp_value;
		    alt_value = alt[SEG_INDEX(alt_seg, r, c)];
		    add_pt(r, c, alt_value, alt_value);
		}
	    }
	}
    }

    G_percent(r, nrows, 1);	/* finish it */

    return 0;
}

int ele_round(double x)
{
    int n;

    if (x >= 0.0)
	n = x + .5;
    else {
	n = -x + .5;
	n = -n;
    }

    return n;
}
