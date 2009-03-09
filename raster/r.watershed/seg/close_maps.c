#include "Gwater.h"
#include <unistd.h>
#include <grass/glocale.h>

int close_maps(void)
{
    struct Colors colors;
    int r, c;
    CELL is_swale;
    DCELL *dbuf = NULL;
    int fd;
    struct FPRange accRange;
    DCELL min, max;
    DCELL clr_min, clr_max;
    DCELL sum, sum_sqr, stddev, lstddev, dvalue;

    dseg_close(&slp);
    cseg_close(&alt);
    if (wat_flag) {
	sum = sum_sqr = stddev = 0.0;
	dbuf = G_allocate_d_raster_buf();
	if (abs_acc) {
	    G_warning("Writing out only positive flow accumulation values.");
	    G_warning("Cells with a likely underestimate for flow accumulation can no longer be identified.");

	    fd = G_open_raster_new(wat_name, DCELL_TYPE);
	    if (fd < 0) {
		G_warning(_("unable to open new accum map layer."));
	    }
	    for (r = 0; r < nrows; r++) {
		G_set_d_null_value(dbuf, ncols);	/* reset row to all NULL */
		for (c = 0; c < ncols; c++) {
		    dseg_get(&wat, &dvalue, r, c);
		    if (G_is_d_null_value(&dvalue) == 0 && dvalue) {
			dvalue = ABS(dvalue);
			dbuf[c] = dvalue;
			sum += dvalue;
			sum_sqr += dvalue * dvalue;
		    }
		}
		G_put_raster_row(fd, dbuf, DCELL_TYPE);
	    }
	    if (G_close_cell(fd) < 0)
		G_warning(_("Close failed."));
	}
	else {
	    dseg_write_cellfile(&wat, wat_name);

	    /* get standard deviation */
	    fd = G_open_cell_old(wat_name, "");
	    if (fd < 0) {
		G_fatal_error(_("unable to open flow accumulation map layer"));
	    }

	    for (r = 0; r < nrows; r++) {
		G_get_d_raster_row(fd, dbuf, r);
		for (c = 0; c < ncols; c++) {
		    dvalue = dbuf[c];
		    if (G_is_d_null_value(&dvalue) == 0 && dvalue) {
			dvalue = ABS(dvalue);
			sum += dvalue;
			sum_sqr += dvalue * dvalue;
		    }
		}
	    }
	}

	stddev = sqrt((sum_sqr - (sum + sum / do_points)) / (do_points - 1));
	G_debug(1, "stddev: %f", stddev);

	/* set nice color rules: yellow, green, cyan, blue, black */
	/* start with white to get more detail? NULL cells are white by default, may be confusing */

	lstddev = log(stddev);

	G_read_fp_range(wat_name, this_mapset, &accRange);
	min = max = 0;
	G_get_fp_range_min_max(&accRange, &min, &max);

	G_init_colors(&colors);

	if (min < 0) {
	    if (min < (-stddev - 1)) {
		clr_min = min;
		clr_max = -stddev - 1;
		G_add_d_raster_color_rule(&clr_min, 0, 0, 0, &clr_max, 0,
					  0, 0, &colors);
	    }
	    clr_min = -stddev - 1.;
	    clr_max = -1. * exp(lstddev * 0.75);
	    G_add_d_raster_color_rule(&clr_min, 0, 0, 0, &clr_max, 0,
				      0, 255, &colors);
	    clr_min = clr_max;
	    clr_max = -1. * exp(lstddev * 0.5);
	    G_add_d_raster_color_rule(&clr_min, 0, 0, 255, &clr_max, 0,
				      255, 255, &colors);
	    clr_min = clr_max;
	    clr_max = -1. * exp(lstddev * 0.35);
	    G_add_d_raster_color_rule(&clr_min, 0, 255, 255, &clr_max, 0,
				      255, 0, &colors);
	    clr_min = clr_max;
	    clr_max = -1.;
	    G_add_d_raster_color_rule(&clr_min, 0, 255, 0, &clr_max, 255,
				      255, 0, &colors);
	}
	clr_min = -1.;
	clr_max = 1.;
	G_add_d_raster_color_rule(&clr_min, 255, 255, 0, &clr_max, 255,
				  255, 0, &colors);
	clr_min = 1;
	clr_max = exp(lstddev * 0.35);
	G_add_d_raster_color_rule(&clr_min, 255, 255, 0, &clr_max, 0,
				  255, 0, &colors);
	clr_min = clr_max;
	clr_max = exp(lstddev * 0.5);
	G_add_d_raster_color_rule(&clr_min, 0, 255, 0, &clr_max, 0,
				  255, 255, &colors);
	clr_min = clr_max;
	clr_max = exp(lstddev * 0.75);
	G_add_d_raster_color_rule(&clr_min, 0, 255, 255, &clr_max, 0,
				  0, 255, &colors);
	clr_min = clr_max;
	clr_max = stddev + 1.;
	G_add_d_raster_color_rule(&clr_min, 0, 0, 255, &clr_max, 0, 0,
				  0, &colors);

	if (max > 0 && max > stddev + 1) {
	    clr_min = stddev + 1;
	    clr_max = max;
	    G_add_d_raster_color_rule(&clr_min, 0, 0, 0, &clr_max, 0, 0, 0,
				      &colors);
	}
	G_write_colors(wat_name, this_mapset, &colors);
    }
    if (asp_flag) {
	cseg_write_cellfile(&asp, asp_name);
	G_init_colors(&colors);
	G_make_grey_scale_colors(&colors, 1, 8);
	G_write_colors(asp_name, this_mapset, &colors);
    }
    cseg_close(&asp);
    /* visual ouput no longer needed */
    if (dis_flag) {
	if (bas_thres <= 0)
	    bas_thres = 60;
	for (r = 0; r < nrows; r++) {
	    for (c = 0; c < ncols; c++) {
		dseg_get(&wat, &dvalue, r, c);
		if (dvalue < 0) {
		    dvalue = 0;
		    dseg_put(&wat, &dvalue, r, c);
		}
		else {
		    bseg_get(&swale, &is_swale, r, c);
		    if (is_swale) {
			dvalue = bas_thres;
			dseg_put(&wat, &dvalue, r, c);
		    }
		}
	    }
	}
	dseg_write_cellfile(&wat, dis_name);
	G_init_colors(&colors);
	G_make_rainbow_colors(&colors, 1, 120);
	G_write_colors(dis_name, this_mapset, &colors);
    }
    /* error in gislib.a
       G_free_colors(&colors);
     */
    dseg_close(&wat);
    if (ls_flag) {
	dseg_write_cellfile(&l_s, ls_name);
	dseg_close(&l_s);
    }
    bseg_close(&swale);
    if (sl_flag) {
	for (r = 0; r < nrows; r++) {
	    for (c = 0; c < ncols; c++) {
		dseg_get(&s_l, &dvalue, r, c);
		if (dvalue > max_length)
		    dseg_put(&s_l, &max_length, r, c);
	    }
	}
	dseg_write_cellfile(&s_l, sl_name);
    }
    if (sl_flag || ls_flag || sg_flag)
	dseg_close(&s_l);
    if (ril_flag)
	dseg_close(&ril);
    if (sg_flag)
	dseg_write_cellfile(&s_g, sg_name);
    if (sg_flag)
	dseg_close(&s_g);
    if (ls_flag || sg_flag)
	cseg_close(&r_h);

    return 0;
}
