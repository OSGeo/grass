#include "Gwater.h"
#include <unistd.h>
#include <grass/glocale.h>

int close_maps(void)
{
    struct Colors colors;
    int r, c;
    DCELL *dbuf = NULL;
    int fd;
    struct FPRange accRange;
    DCELL min, max;
    DCELL clr_min, clr_max;
    DCELL sum, sum_sqr, stddev, lstddev, dvalue;
    WAT_ALT *wabuf;

    /* bseg_close(&swale); */
    bseg_close(&bitflags);
    if (wat_flag) {
	G_message(_("Closing accumulation map"));
	sum = sum_sqr = stddev = 0.0;
	dbuf = Rast_allocate_d_buf();
	wabuf = G_malloc(ncols * sizeof(WAT_ALT));
	seg_flush(&watalt);
	if (abs_acc) {
	    G_warning("Writing out only positive flow accumulation values.");
	    G_warning("Cells with a likely underestimate for flow accumulation can no longer be identified.");
	}

	fd = Rast_open_new(wat_name, DCELL_TYPE);

	for (r = 0; r < nrows; r++) {
	    G_percent(r, nrows, 1);
	    Rast_set_d_null_value(dbuf, ncols);	/* reset row to all NULL */
	    seg_get_row(&watalt, (char *)wabuf, r);
	    for (c = 0; c < ncols; c++) {
		/* dseg_get(&wat, &dvalue, r, c); */
		dvalue = wabuf[c].wat;
		if (Rast_is_d_null_value(&dvalue) == 0 && dvalue) {
		    if (abs_acc) {
			dvalue = fabs(dvalue);
			sum += dvalue;
		    }
		    else
			sum += fabs(dvalue);

		    dbuf[c] = dvalue;
		    sum_sqr += dvalue * dvalue;
		}
	    }
	    Rast_put_row(fd, dbuf, DCELL_TYPE);
	}
	G_percent(r, nrows, 1);    /* finish it */

	Rast_close(fd);
	G_free(wabuf);
	G_free(dbuf);

	stddev = sqrt((sum_sqr - (sum + sum / do_points)) / (do_points - 1));
	G_debug(1, "stddev: %f", stddev);

	/* set nice color rules: yellow, green, cyan, blue, black */
	/* start with white to get more detail? NULL cells are white by default, may be confusing */

	lstddev = log(stddev);

	Rast_read_fp_range(wat_name, this_mapset, &accRange);
	min = max = 0;
	Rast_get_fp_range_min_max(&accRange, &min, &max);

	Rast_init_colors(&colors);

	if (min < 0) {
	    if (min < (-stddev - 1)) {
		clr_min = min - 1;
		clr_max = -stddev - 1;
		Rast_add_d_color_rule(&clr_min, 0, 0, 0, &clr_max, 0,
					  0, 0, &colors);
	    }
	    clr_min = -stddev - 1.;
	    clr_max = -1. * exp(lstddev * 0.75);
	    Rast_add_d_color_rule(&clr_min, 0, 0, 0, &clr_max, 0,
				      0, 255, &colors);
	    clr_min = clr_max;
	    clr_max = -1. * exp(lstddev * 0.5);
	    Rast_add_d_color_rule(&clr_min, 0, 0, 255, &clr_max, 0,
				      255, 255, &colors);
	    clr_min = clr_max;
	    clr_max = -1. * exp(lstddev * 0.35);
	    Rast_add_d_color_rule(&clr_min, 0, 255, 255, &clr_max, 0,
				      255, 0, &colors);
	    clr_min = clr_max;
	    clr_max = -1.;
	    Rast_add_d_color_rule(&clr_min, 0, 255, 0, &clr_max, 255,
				      255, 0, &colors);
	}
	clr_min = -1.;
	clr_max = 1.;
	Rast_add_d_color_rule(&clr_min, 255, 255, 0, &clr_max, 255,
				  255, 0, &colors);
	clr_min = 1;
	clr_max = exp(lstddev * 0.35);
	Rast_add_d_color_rule(&clr_min, 255, 255, 0, &clr_max, 0,
				  255, 0, &colors);
	clr_min = clr_max;
	clr_max = exp(lstddev * 0.5);
	Rast_add_d_color_rule(&clr_min, 0, 255, 0, &clr_max, 0,
				  255, 255, &colors);
	clr_min = clr_max;
	clr_max = exp(lstddev * 0.75);
	Rast_add_d_color_rule(&clr_min, 0, 255, 255, &clr_max, 0,
				  0, 255, &colors);
	clr_min = clr_max;
	clr_max = stddev + 1.;
	Rast_add_d_color_rule(&clr_min, 0, 0, 255, &clr_max, 0, 0,
				  0, &colors);

	if (max > 0 && max > stddev + 1) {
	    clr_min = stddev + 1;
	    clr_max = max + 1;
	    Rast_add_d_color_rule(&clr_min, 0, 0, 0, &clr_max, 0, 0, 0,
				      &colors);
	}
	Rast_write_colors(wat_name, this_mapset, &colors);
    }
    seg_close(&watalt);
    if (asp_flag) {
	G_message(_("Closing flow direction map"));
	bseg_write_cellfile(&asp, asp_name);
	Rast_init_colors(&colors);
	Rast_make_grey_scale_colors(&colors, 1, 8);
	Rast_write_colors(asp_name, this_mapset, &colors);
    }
    bseg_close(&asp);
    if (ls_flag) {
	G_message(_("Closing LS map"));
	dseg_write_cellfile(&l_s, ls_name);
	dseg_close(&l_s);
    }
    if (sl_flag) {
	G_message(_("Closing SL map"));
	for (r = 0; r < nrows; r++) {
	    G_percent(r, nrows, 1);
	    for (c = 0; c < ncols; c++) {
		dseg_get(&s_l, &dvalue, r, c);
		if (dvalue > max_length)
		    dseg_put(&s_l, &max_length, r, c);
	    }
	}
	G_percent(r, nrows, 1);    /* finish it */
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
