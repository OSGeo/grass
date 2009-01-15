#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "Gwater.h"
#include <grass/gis.h>
#include <grass/glocale.h>


int init_vars(int argc, char *argv[])
{
    SHORT r, c;
    int fd, num_cseg_total, num_open_segs;
    int seg_rows, seg_cols;
    double segs_mb;

    /* int page_block, num_cseg; */
    int max_bytes;
    CELL *buf, alt_value, asp_value, worked_value, block_value;
    DCELL wat_value;
    char MASK_flag;

    G_gisinit(argv[0]);
    ele_flag = wat_flag = asp_flag = pit_flag = run_flag = ril_flag = 0;
    ob_flag = bas_flag = seg_flag = haf_flag = arm_flag = dis_flag = 0;
    zero = sl_flag = sg_flag = ls_flag = er_flag = bas_thres = 0;
    nxt_avail_pt = 0;
    /* dep_flag = 0; */
    max_length = d_zero = 0.0;
    d_one = 1.0;
    ril_value = -1.0;
    /* dep_slope = 0.0; */
    max_bytes = 0;
    sides = 8;
    mfd = 1;
    c_fac = 5;
    segs_mb = 300;
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
	else if (sscanf(argv[r], "mb=%lf", &segs_mb) == 1) ;
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

    G_message(_("SECTION 1 beginning: Initiating Variables. %d sections total."),
	      tot_parts);

    this_mapset = G_mapset();
    if (sl_flag || sg_flag || ls_flag)
	er_flag = 1;
    /* for sd factor
       if (dep_flag)        {
       if (sscanf (dep_name, "%lf", &dep_slope) != 1)       {
       dep_flag = -1;
       }
       }
     */
    G_get_set_window(&window);
    nrows = G_window_rows();
    ncols = G_window_cols();
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

    /* segment parameters: one size fits all. Fine tune? */
    /* Segment rows and cols: 200 */
    /* 1 segment open for all rasters: 1.34 MB */

    seg_rows = SROW;
    seg_cols = SCOL;

    if (segs_mb < 3.0) {
	segs_mb = 300;
	G_warning(_("Maximum memory to be used was smaller than 3 MB, set to default = 300 MB."));
    }

    num_open_segs = segs_mb / 1.34;

    G_debug(1, "segs MB: %.0f", segs_mb);
    G_debug(1, "region rows: %d", nrows);
    G_debug(1, "seg rows: %d", seg_rows);
    G_debug(1, "region cols: %d", ncols);
    G_debug(1, "seg cols: %d", seg_cols);

    num_cseg_total = nrows / SROW + 1;
    G_debug(1, "   row segments:\t%d", num_cseg_total);

    num_cseg_total = ncols / SCOL + 1;
    G_debug(1, "column segments:\t%d", num_cseg_total);

    num_cseg_total = (ncols / seg_cols + 1) * (nrows / seg_rows + 1);
    G_debug(1, " total segments:\t%d", num_cseg_total);
    G_debug(1, "  open segments:\t%d", num_open_segs);

    /* nonsense to have more segments open than exist */
    if (num_open_segs > num_cseg_total)
	num_open_segs = num_cseg_total;
    G_debug(1, "  open segments after adjusting:\t%d", num_open_segs);

    cseg_open(&alt, seg_rows, seg_cols, num_open_segs);
    cseg_read_cell(&alt, ele_name, "");
    if (er_flag) {
	cseg_open(&r_h, seg_rows, seg_cols, num_open_segs);
	cseg_read_cell(&r_h, ele_name, "");
    }
    
    /* NULL cells in input elevation map */
    bseg_open(&in_list, seg_rows, seg_cols, num_open_segs);
    bseg_open(&worked, seg_rows, seg_cols, num_open_segs);
    G_debug(1, "Checking for masked and NULL cells in input elevation <%s>.", ele_name);
    MASK_flag = 0;
    do_points = nrows * ncols;
    fd = G_open_cell_old(ele_name, "");
    if (fd < 0) {
	G_fatal_error(_("unable to open elevation map layer"));
    }
    buf = G_allocate_cell_buf();
    for (r = 0; r < nrows; r++) {
	G_get_c_raster_row(fd, buf, r);
	for (c = 0; c < ncols; c++) {
	    /* check for masked and NULL cells */
	    alt_value = buf[c];
	    if (G_is_c_null_value(&alt_value)) {
		bseg_put(&worked, &one, r, c);
		bseg_put(&in_list, &one, r, c);
		do_points--;
	    }
	}
    }
    G_close_cell(fd);
    G_free(buf);
    if (do_points < nrows * ncols)
	MASK_flag = 1;
    
    /* initial flow accumulation */
    dseg_open(&wat, seg_rows, seg_cols, num_open_segs);
    if (run_flag) {
	dseg_read_cell(&wat, run_name, "");
	if (MASK_flag) {
	    for (r = 0; r < nrows; r++) {
		for (c = 0; c < ncols; c++) {
		    bseg_get(&worked, &worked_value, r, c);
		    if (worked_value)
			dseg_put(&wat, &d_zero, r, c);
		}
	    }
	}
    }
    else {
	for (r = 0; r < nrows; r++) {
	    for (c = 0; c < ncols; c++)
		if (MASK_flag) {
		    bseg_get(&worked, &worked_value, r, c);
		    if (worked_value)
			dseg_put(&wat, &d_zero, r, c);
		    else
			dseg_put(&wat, &d_one, r, c);
		}
		else {
		    if (-1 == dseg_put(&wat, &d_one, r, c))
			exit(EXIT_FAILURE);
		}
	}
    }
    cseg_open(&asp, seg_rows, seg_cols, num_open_segs);
    /* depression: drainage direction will be set to zero later */
    if (pit_flag) {
	fd = G_open_cell_old(pit_name, "");
	if (fd < 0) {
	    G_fatal_error(_("unable to open depression map layer"));
	}
	buf = G_allocate_cell_buf();
	for (r = 0; r < nrows; r++) {
	    G_get_c_raster_row(fd, buf, r);
	    for (c = 0; c < ncols; c++) {
		asp_value = buf[c];
		if (!G_is_c_null_value(&asp_value)) {
		    cseg_put(&asp, &one, r, c);
		}
		else {
		    cseg_put(&asp, &zero, r, c);
		}
	    }
	}
	G_close_cell(fd);
	G_free(buf);
    }
    else {
	for (r = 0; r < nrows; r++) {
	    for (c = 0; c < ncols; c++)
		if (-1 == cseg_put(&asp, &zero, r, c))
		    exit(EXIT_FAILURE);
	}
    }
    bseg_open(&swale, seg_rows, seg_cols, num_open_segs);
    if (ob_flag) {
	fd = G_open_cell_old(ob_name, "");
	if (fd < 0) {
	    G_fatal_error(_("unable to open blocking map layer"));
	}
	buf = G_allocate_cell_buf();
	for (r = 0; r < nrows; r++) {
	    G_get_c_raster_row(fd, buf, r);
	    for (c = 0; c < ncols; c++) {
		block_value = buf[c];
		if (!G_is_c_null_value(&block_value)) {
		    bseg_put(&swale, &one, r, c);
		}
		else {
		    bseg_put(&swale, &zero, r, c);
		}
	    }
	}
	G_close_cell(fd);
	G_free(buf);
    }
    else {
	for (r = 0; r < nrows; r++) {
	    for (c = 0; c < ncols; c++)
		bseg_put(&swale, &zero, r, c);
	}
    }
    if (ril_flag) {
	dseg_open(&ril, 1, seg_rows * seg_cols, num_open_segs);
	dseg_read_cell(&ril, ril_name, "");
    }
    
    /* dseg_open(&slp, SROW, SCOL, num_open_segs); */

    /* RUSLE: LS and/or S factor */

    if (er_flag) {
	dseg_open(&s_l, seg_rows, seg_cols, num_open_segs);
    }
    if (sg_flag)
	dseg_open(&s_g, 1, seg_rows * seg_cols, num_open_segs);
    if (ls_flag)
	dseg_open(&l_s, 1, seg_rows * seg_cols, num_open_segs);

    seg_open(&astar_pts, 1, do_points, 1, seg_rows * seg_cols * 2,
	     num_open_segs / 2, sizeof(POINT));

    /* heap_index will track astar_pts in ternary min-heap */
    /* heap_index is one-based */
    seg_open(&heap_index, 1, do_points + 1, 1, seg_cols * num_open_segs * seg_rows / 10,
	     10, sizeof(HEAP));

    G_message(_("SECTION 1b (of %1d): Determining Offmap Flow."), tot_parts);

    /* heap is empty */
    heap_size = 0;

    first_astar = first_cum = -1;

    if (MASK_flag) {
	for (r = 0; r < nrows; r++) {
	    G_percent(r, nrows, 2);
	    for (c = 0; c < ncols; c++) {
		bseg_get(&worked, &worked_value, r, c);
		if (worked_value) {
		    dseg_put(&wat, &d_zero, r, c);
		}
		else {
		    if (er_flag)
			dseg_put(&s_l, &half_res, r, c);
		    cseg_get(&asp, &asp_value, r, c);
		    if (r == 0 || c == 0 || r == nrows - 1 ||
			c == ncols - 1 || asp_value != 0) {
			dseg_get(&wat, &wat_value, r, c);
			if (wat_value > 0) {
			    wat_value = -wat_value;
			    dseg_put(&wat, &wat_value, r, c);
			}
			/* set depression */
			if (asp_value) {
			    asp_value = 0;
			    if (wat_value < 0) {
				wat_value = -wat_value;
				dseg_put(&wat, &wat_value, r, c);
			    }
			}
			else if (r == 0)
			    asp_value = -2;
			else if (c == 0)
			    asp_value = -4;
			else if (r == nrows - 1)
			    asp_value = -6;
			else if (c == ncols - 1)
			    asp_value = -8;
			if (-1 == cseg_put(&asp, &asp_value, r, c))
			    exit(EXIT_FAILURE);
			cseg_get(&alt, &alt_value, r, c);
			add_pt(r, c, alt_value, alt_value);
		    }
		    else if (!bseg_get(&worked, &worked_value, r - 1, c)
			     && worked_value != 0) {
			cseg_get(&alt, &alt_value, r, c);
			add_pt(r, c, alt_value, alt_value);
			asp_value = -2;
			cseg_put(&asp, &asp_value, r, c);
			dseg_get(&wat, &wat_value, r, c);
			if (wat_value > 0) {
			    wat_value = -wat_value;
			    dseg_put(&wat, &wat_value, r, c);
			}
		    }
		    else if (!bseg_get(&worked, &worked_value, r + 1, c)
			     && worked_value != 0) {
			cseg_get(&alt, &alt_value, r, c);
			add_pt(r, c, alt_value, alt_value);
			asp_value = -6;
			cseg_put(&asp, &asp_value, r, c);
			dseg_get(&wat, &wat_value, r, c);
			if (wat_value > 0) {
			    wat_value = -wat_value;
			    dseg_put(&wat, &wat_value, r, c);
			}
		    }
		    else if (!bseg_get(&worked, &worked_value, r, c - 1)
			     && worked_value != 0) {
			cseg_get(&alt, &alt_value, r, c);
			add_pt(r, c, alt_value, alt_value);
			asp_value = -4;
			cseg_put(&asp, &asp_value, r, c);
			dseg_get(&wat, &wat_value, r, c);
			if (wat_value > 0) {
			    wat_value = -wat_value;
			    dseg_put(&wat, &wat_value, r, c);
			}
		    }
		    else if (!bseg_get(&worked, &worked_value, r, c + 1)
			     && worked_value != 0) {
			cseg_get(&alt, &alt_value, r, c);
			add_pt(r, c, alt_value, alt_value);
			asp_value = -8;
			cseg_put(&asp, &asp_value, r, c);
			dseg_get(&wat, &wat_value, r, c);
			if (wat_value > 0) {
			    wat_value = -wat_value;
			    dseg_put(&wat, &wat_value, r, c);
			}
		    }
		    else if (sides == 8 &&
			     !bseg_get(&worked, &worked_value, r - 1, c - 1)
			     && worked_value != 0) {
			cseg_get(&alt, &alt_value, r, c);
			add_pt(r, c, alt_value, alt_value);
			asp_value = -3;
			cseg_put(&asp, &asp_value, r, c);
			dseg_get(&wat, &wat_value, r, c);
			if (wat_value > 0) {
			    wat_value = -wat_value;
			    dseg_put(&wat, &wat_value, r, c);
			}
		    }
		    else if (sides == 8 &&
			     !bseg_get(&worked, &worked_value, r - 1, c + 1)
			     && worked_value != 0) {
			cseg_get(&alt, &alt_value, r, c);
			add_pt(r, c, alt_value, alt_value);
			asp_value = -1;
			cseg_put(&asp, &asp_value, r, c);
			dseg_get(&wat, &wat_value, r, c);
			if (wat_value > 0) {
			    wat_value = -wat_value;
			    dseg_put(&wat, &wat_value, r, c);
			}
		    }
		    else if (sides == 8 &&
			     !bseg_get(&worked, &worked_value, r + 1, c - 1)
			     && worked_value != 0) {
			cseg_get(&alt, &alt_value, r, c);
			add_pt(r, c, alt_value, alt_value);
			asp_value = -5;
			cseg_put(&asp, &asp_value, r, c);
			dseg_get(&wat, &wat_value, r, c);
			if (wat_value > 0) {
			    wat_value = -wat_value;
			    dseg_put(&wat, &wat_value, r, c);
			}
		    }
		    else if (sides == 8 &&
			     !bseg_get(&worked, &worked_value, r + 1, c + 1)
			     && worked_value != 0) {
			cseg_get(&alt, &alt_value, r, c);
			add_pt(r, c, alt_value, alt_value);
			asp_value = -7;
			cseg_put(&asp, &asp_value, r, c);
			dseg_get(&wat, &wat_value, r, c);
			if (wat_value > 0) {
			    wat_value = -wat_value;
			    dseg_put(&wat, &wat_value, r, c);
			}
		    }
		    else {
			bseg_put(&in_list, &zero, r, c);
			/* dseg_put(&slp, &dzero, r, c); */
		    }
		}
	    }
	}
    }
    else {
	for (r = 0; r < nrows; r++) {
	    G_percent(r, nrows, 2);
	    for (c = 0; c < ncols; c++) {
		bseg_put(&worked, &zero, r, c);
		if (er_flag)
		    dseg_put(&s_l, &half_res, r, c);
		cseg_get(&asp, &asp_value, r, c);
		if (r == 0 || c == 0 || r == nrows - 1 ||
		    c == ncols - 1 || asp_value != 0) {
		    dseg_get(&wat, &wat_value, r, c);
		    if (wat_value > 0) {
			wat_value = -wat_value;
			if (-1 == dseg_put(&wat, &wat_value, r, c))
			    exit(EXIT_FAILURE);
		    }
		    /* set depression */
		    if (asp_value) {
			asp_value = 0;
			if (wat_value < 0) {
			    wat_value = -wat_value;
			    dseg_put(&wat, &wat_value, r, c);
			}
		    }
		    else if (r == 0)
			asp_value = -2;
		    else if (c == 0)
			asp_value = -4;
		    else if (r == nrows - 1)
			asp_value = -6;
		    else if (c == ncols - 1)
			asp_value = -8;
		    if (-1 == cseg_put(&asp, &asp_value, r, c))
			exit(EXIT_FAILURE);
		    cseg_get(&alt, &alt_value, r, c);
		    add_pt(r, c, alt_value, alt_value);
		}
		else {
		    bseg_put(&in_list, &zero, r, c);
		    /* dseg_put(&slp, &dzero, r, c); */
		}
	    }
	}
    }
    G_percent(r, nrows, 1);	/* finish it */

    return 0;
}
