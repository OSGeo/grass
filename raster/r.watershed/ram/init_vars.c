#include <stdlib.h>
#include <string.h>
#include "Gwater.h"
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

int ele_round(double);

int init_vars(int argc, char *argv[])
{
    int r, c;
    CELL *buf, alt_value, asp_value, block_value;
    DCELL dvalue, wat_value, *dbuf;
    void *elebuf, *ptr;
    int fd, ele_map_type;
    size_t ele_size;
    char MASK_flag;
    int seg_idx;

    G_gisinit(argv[0]);
    /* input */
    ele_flag = pit_flag = run_flag = ril_flag = rtn_flag = 0;
    /* output */
    wat_flag = asp_flag = tci_flag = spi_flag = atanb_flag = 0;
    bas_flag = seg_flag = haf_flag = 0;
    bas_thres = 0;
    /* shed, unused */
    arm_flag = dis_flag = 0;
    /* RUSLE */
    ob_flag = st_flag = sl_flag = sg_flag = ls_flag = er_flag = 0;
    zero = 0;
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
    flat_flag = 0;
    ele_scale = 1;

    for (r = 1; r < argc; r++) {
	if (sscanf(argv[r], "elevation=%s", ele_name) == 1)
	    ele_flag++;
	else if (sscanf(argv[r], "accumulation=%s", wat_name) == 1)
	    wat_flag++;
	else if (sscanf(argv[r], "tci=%s", tci_name) == 1)
	    tci_flag++;
	else if (sscanf(argv[r], "spi=%s", spi_name) == 1)
	    spi_flag++;
	else if (sscanf(argv[r], "drainage=%s", asp_name) == 1)
	    asp_flag++;
	else if (sscanf(argv[r], "depression=%s", pit_name) == 1)
	    pit_flag++;
	else if (sscanf(argv[r], "threshold=%d", &bas_thres) == 1) ;
	else if (sscanf(argv[r], "max_slope_length=%lf", &max_length) == 1) ;
	else if (sscanf(argv[r], "basin=%s", bas_name) == 1)
	    bas_flag++;
	else if (sscanf(argv[r], "stream=%s", seg_name) == 1)
	    seg_flag++;
	else if (sscanf(argv[r], "half_basin=%s", haf_name) == 1)
	    haf_flag++;
	else if (sscanf(argv[r], "flow=%s", run_name) == 1)
	    run_flag++;
	else if (sscanf(argv[r], "retention=%s", rtn_name) == 1)
	    rtn_flag++;
	else if (sscanf(argv[r], "ar=%s", arm_name) == 1)
	    arm_flag++;
	/* slope length
	   else if (sscanf(argv[r], "sl=%[^\n]", sl_name) == 1)
	   sl_flag++; */
	else if (sscanf(argv[r], "length_slope=%s", ls_name) == 1)
	    ls_flag++;
	else if (sscanf(argv[r], "slope_steepness=%s", sg_name) == 1)
	    sg_flag++;
	else if (sscanf(argv[r], "blocking=%s", ob_name) == 1)
	    ob_flag++;
	else if (sscanf(argv[r], "disturbed_land=%s", ril_name) == 1) {
	    if (sscanf(ril_name, "%lf", &ril_value) == 0) {
		ril_value = -1.0;
		ril_flag++;
	    }
	}
	/* slope deposition
	   else if (sscanf (argv[r], "sd=%[^\n]", dep_name) == 1) dep_flag++; */
	else if (sscanf(argv[r], "-%d", &sides) == 1) {
	    if (sides != 4)
		usage(argv[0]);
	}
	else if (sscanf(argv[r], "convergence=%d", &c_fac) == 1) ;
	else if (strcmp(argv[r], "-s") == 0)
	    mfd = 0;
	else if (strcmp(argv[r], "-a") == 0)
	    abs_acc = 1;
	else if (strcmp(argv[r], "-b") == 0)
	    flat_flag = 1;
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
    nrows = Rast_window_rows();
    ncols = Rast_window_cols();
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
    wat =
	(DCELL *) G_malloc(sizeof(DCELL) *
			   size_array(&wat_seg, nrows, ncols));

    sca = tanb = NULL;
    atanb_flag = 0;
    if (tci_flag || spi_flag) {
	sca = (DCELL *) G_malloc(sizeof(DCELL) *
				 size_array(&wat_seg, nrows, ncols));
	tanb = (DCELL *) G_malloc(sizeof(DCELL) *
				  size_array(&wat_seg, nrows, ncols));
	atanb_flag = 1;
    }

    asp =
	(CELL *) G_malloc(size_array(&asp_seg, nrows, ncols) * sizeof(CELL));

    if (er_flag) {
	r_h =
	    (CELL *) G_malloc(sizeof(CELL) *
			      size_array(&r_h_seg, nrows, ncols));
    }

    swale = flag_create(nrows, ncols);
    in_list = flag_create(nrows, ncols);
    worked = flag_create(nrows, ncols);

    /* open elevation input */
    fd = Rast_open_old(ele_name, "");

    ele_map_type = Rast_get_map_type(fd);
    ele_size = Rast_cell_size(ele_map_type);
    elebuf = Rast_allocate_buf(ele_map_type);

    if (ele_map_type == FCELL_TYPE || ele_map_type == DCELL_TYPE)
	ele_scale = 1000;	/* should be enough to do the trick */
    if (flat_flag)
	ele_scale = 10000;

    /* read elevation input and mark NULL/masked cells */
    /* initialize accumulation and drainage direction */
    MASK_flag = 0;
    do_points = nrows * ncols;
    for (r = 0; r < nrows; r++) {
	Rast_get_row(fd, elebuf, r, ele_map_type);
	ptr = elebuf;
	for (c = 0; c < ncols; c++) {
	    seg_idx = SEG_INDEX(alt_seg, r, c);

	    /* all flags need to be manually set to zero */
	    flag_unset(swale, r, c);
	    flag_unset(in_list, r, c);
	    flag_unset(worked, r, c);

	    /* check for masked and NULL cells */
	    if (Rast_is_null_value(ptr, ele_map_type)) {
		FLAG_SET(worked, r, c);
		FLAG_SET(in_list, r, c);
		Rast_set_c_null_value(&alt_value, 1);
		Rast_set_d_null_value(&wat_value, 1);
		do_points--;
	    }
	    else {
		if (ele_map_type == CELL_TYPE) {
		    alt_value = *((CELL *) ptr);
		    alt_value *= ele_scale;
		}
		else if (ele_map_type == FCELL_TYPE) {
		    dvalue = *((FCELL *) ptr);
		    dvalue *= ele_scale;
		    alt_value = ele_round(dvalue);
		}
		else if (ele_map_type == DCELL_TYPE) {
		    dvalue = *((DCELL *) ptr);
		    dvalue *= ele_scale;
		    alt_value = ele_round(dvalue);
		}
		wat_value = 1.0;
	    }
	    alt[seg_idx] = alt_value;
	    wat[seg_idx] = wat_value;
	    asp[seg_idx] = 0;
	    if (er_flag) {
		r_h[seg_idx] = alt_value;
	    }
	    if (atanb_flag) {
		Rast_set_d_null_value(&sca[seg_idx], 1);
		Rast_set_d_null_value(&tanb[seg_idx], 1);
	    }
	    ptr = G_incr_void_ptr(ptr, ele_size);
	}
    }
    Rast_close(fd);
    G_free(elebuf);
    if (do_points < nrows * ncols)
	MASK_flag = 1;

    /* read flow accumulation from input map flow: amount of overland flow per cell */
    if (run_flag) {
	dbuf = Rast_allocate_d_buf();
	fd = Rast_open_old(run_name, "");
	for (r = 0; r < nrows; r++) {
	    Rast_get_d_row(fd, dbuf, r);
	    for (c = 0; c < ncols; c++) {
		if (MASK_flag) {
		    block_value = FLAG_GET(worked, r, c);
		    if (!block_value)
			wat[SEG_INDEX(wat_seg, r, c)] = dbuf[c];
		    else
			wat[SEG_INDEX(wat_seg, r, c)] = 0.0;
		}
		else
		    wat[SEG_INDEX(wat_seg, r, c)] = dbuf[c];
	    }
	}
	Rast_close(fd);
	G_free(dbuf);
    }

    /* read retention map to adjust flow distribution (AG) */
    rtn = NULL;
    if (rtn_flag) {
	rtn = (char *) G_malloc(sizeof(char) *
                           size_array(&rtn_seg, nrows, ncols));
        buf = Rast_allocate_c_buf();
        fd = Rast_open_old(rtn_name, "");
        for (r = 0; r < nrows; r++) {
            Rast_get_c_row(fd, buf, r);
            for (c = 0; c < ncols; c++) {
                if (MASK_flag) {
                    block_value = FLAG_GET(worked, r, c);
                    if (!block_value) {
                        block_value = buf[c];
                    }
		    else
			block_value = 100;
                }
                else
                    block_value = buf[c];

                if (!Rast_is_c_null_value(&block_value)) {
		    if (block_value < 0)
			block_value = 0;
		    if (block_value > 100)
			block_value = 100;
                    rtn[SEG_INDEX(rtn_seg, r, c)] = block_value;
		}
                else
                    rtn[SEG_INDEX(rtn_seg, r, c)] = 100;
            }
        }
        Rast_close(fd);
        G_free(buf);
    }

    /* overland blocking map; this is also creating streams... */
    if (ob_flag) {
	buf = Rast_allocate_c_buf();
	fd = Rast_open_old(ob_name, "");
	for (r = 0; r < nrows; r++) {
	    Rast_get_c_row(fd, buf, r);
	    for (c = 0; c < ncols; c++) {
		block_value = buf[c];
		if (!Rast_is_c_null_value(&block_value) && block_value)
		    FLAG_SET(swale, r, c);
	    }
	}
	Rast_close(fd);
	G_free(buf);
    }

    if (ril_flag)
	ril_fd = Rast_open_old(ril_name, "");

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

    astar_pts = (int *)G_malloc((do_points + 1) * sizeof(int));

    /* heap_index will track astar_pts in ternary min-heap */
    /* heap_index is one-based */
    heap_index = (int *)G_malloc((do_points + 1) * sizeof(int));

    G_message(_("SECTION 1b (of %1d): Determining Offmap Flow."), tot_parts);

    /* heap is empty */
    heap_size = 0;

    if (pit_flag) {
	buf = Rast_allocate_c_buf();
	fd = Rast_open_old(pit_name, "");
    }
    else
	buf = NULL;
    first_astar = first_cum = -1;
    for (r = 0; r < nrows; r++) {
	G_percent(r, nrows, 3);
	if (pit_flag)
	    Rast_get_c_row(fd, buf, r);
	for (c = 0; c < ncols; c++) {
	    seg_idx = SEG_INDEX(wat_seg, r, c);
	    if (!(FLAG_GET(worked, r, c))) {
		asp_value = asp[seg_idx];
		if (er_flag)
		    s_l[seg_idx] = half_res;
		if (r == 0 || c == 0 || r == nrows - 1 || c == ncols - 1) {
		    wat_value = wat[seg_idx];
		    if (wat_value > 0)
			wat[seg_idx] = -wat_value;
		    if (r == 0)
			asp_value = -2;
		    else if (c == 0)
			asp_value = -4;
		    else if (r == nrows - 1)
			asp_value = -6;
		    else if (c == ncols - 1)
			asp_value = -8;
		    asp[seg_idx] = asp_value;
		    alt_value = alt[seg_idx];
		    add_pt(r, c, alt_value);
		}
		else if (FLAG_GET(worked, r - 1, c)) {
		    alt_value = alt[seg_idx];
		    add_pt(r, c, alt_value);
		    asp[seg_idx] = -2;
		    wat_value = wat[seg_idx];
		    if (wat_value > 0)
			wat[seg_idx] = -wat_value;
		}
		else if (FLAG_GET(worked, r + 1, c)) {
		    alt_value = alt[seg_idx];
		    add_pt(r, c, alt_value);
		    asp[seg_idx] = -6;
		    wat_value = wat[seg_idx];
		    if (wat_value > 0)
			wat[seg_idx] = -wat_value;
		}
		else if (FLAG_GET(worked, r, c - 1)) {
		    alt_value = alt[seg_idx];
		    add_pt(r, c, alt_value);
		    asp[seg_idx] = -4;
		    wat_value = wat[seg_idx];
		    if (wat_value > 0)
			wat[seg_idx] = -wat_value;
		}
		else if (FLAG_GET(worked, r, c + 1)) {
		    alt_value = alt[seg_idx];
		    add_pt(r, c, alt_value);
		    asp[seg_idx] = -8;
		    wat_value = wat[seg_idx];
		    if (wat_value > 0)
			wat[seg_idx] = -wat_value;
		}
		else if (sides == 8 && FLAG_GET(worked, r - 1, c - 1)) {
		    alt_value = alt[seg_idx];
		    add_pt(r, c, alt_value);
		    asp[seg_idx] = -3;
		    wat_value = wat[seg_idx];
		    if (wat_value > 0)
			wat[seg_idx] = -wat_value;
		}
		else if (sides == 8 && FLAG_GET(worked, r - 1, c + 1)) {
		    alt_value = alt[seg_idx];
		    add_pt(r, c, alt_value);
		    asp[seg_idx] = -1;
		    wat_value = wat[seg_idx];
		    if (wat_value > 0)
			wat[seg_idx] = -wat_value;
		}
		else if (sides == 8 && FLAG_GET(worked, r + 1, c - 1)) {
		    alt_value = alt[seg_idx];
		    add_pt(r, c, alt_value);
		    asp[seg_idx] = -5;
		    wat_value = wat[seg_idx];
		    if (wat_value > 0)
			wat[seg_idx] = -wat_value;
		}
		else if (sides == 8 && FLAG_GET(worked, r + 1, c + 1)) {
		    alt_value = alt[seg_idx];
		    add_pt(r, c, alt_value);
		    asp[seg_idx] = -7;
		    wat_value = wat[seg_idx];
		    if (wat_value > 0)
			wat[seg_idx] = -wat_value;
		}

		/* real depression ? */
		if (pit_flag && asp[seg_idx] == 0) {
		    if (!Rast_is_c_null_value(&buf[c]) && buf[c] != 0) {
			alt_value = alt[seg_idx];
			add_pt(r, c, alt_value);
		    }
		}
	    }
	}
    }

    G_percent(r, nrows, 1);	/* finish it */

    if (pit_flag) {
	Rast_close(fd);
	G_free(buf);
    }

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
