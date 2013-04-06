#include "Gwater.h"
#include <unistd.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>


int close_maps(void)
{
    struct Colors colors;
    int r, c, fd;
    CELL *buf = NULL;
    DCELL *dbuf = NULL;
    struct FPRange accRange;
    DCELL min, max;
    DCELL clr_min, clr_max;
    DCELL sum, sum_sqr, stddev, lstddev, dvalue;

    if (asp_flag || dis_flag)
	buf = Rast_allocate_c_buf();
    if (wat_flag || ls_flag || sl_flag || sg_flag || tci_flag)
	dbuf = Rast_allocate_d_buf();
    G_free(alt);
    if (ls_flag || sg_flag)
	G_free(r_h);

    sum = sum_sqr = stddev = 0.0;
    if (wat_flag) {
	fd = Rast_open_new(wat_name, DCELL_TYPE);
	if (abs_acc) {
	    G_warning("Writing out only positive flow accumulation values.");
	    G_warning("Cells with a likely underestimate for flow accumulation can no longer be identified.");
	    for (r = 0; r < nrows; r++) {
		Rast_set_d_null_value(dbuf, ncols);	/* reset row to all NULL */
		for (c = 0; c < ncols; c++) {
		    dvalue = wat[SEG_INDEX(wat_seg, r, c)];
		    if (Rast_is_d_null_value(&dvalue) == 0 && dvalue) {
			dvalue = ABS(dvalue);
			dbuf[c] = dvalue;
			sum += dvalue;
			sum_sqr += dvalue * dvalue;
		    }
		}
		Rast_put_row(fd, dbuf, DCELL_TYPE);
	    }
	}
	else {
	    for (r = 0; r < nrows; r++) {
		Rast_set_d_null_value(dbuf, ncols);	/* reset row to all NULL */
		for (c = 0; c < ncols; c++) {
		    dvalue = wat[SEG_INDEX(wat_seg, r, c)];
		    if (!Rast_is_d_null_value(&dvalue)) {
			dbuf[c] = dvalue;
			dvalue = ABS(dvalue);
			sum += dvalue;
			sum_sqr += dvalue * dvalue;
		    }
		}
		Rast_put_row(fd, dbuf, DCELL_TYPE);
	    }
	}
	Rast_close(fd);

	stddev =
	    sqrt((sum_sqr - (sum + sum / do_points)) / (do_points - 1));
	G_debug(1, "stddev: %f", stddev);

	/* set nice color rules: yellow, green, cyan, blue, black */

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
	    Rast_add_d_color_rule(&clr_min, 0, 0, 0, &clr_max, 0, 0,
				      0, &colors);
	}
	Rast_write_colors(wat_name, this_mapset, &colors);
    }

    /* TCI */
    if (tci_flag) {
	DCELL watvalue;
	double mean;

	sum = sum_sqr = stddev = 0.0;
	fd = Rast_open_new(tci_name, DCELL_TYPE);
	for (r = 0; r < nrows; r++) {
	    Rast_set_d_null_value(dbuf, ncols);	/* reset row to all NULL */
	    for (c = 0; c < ncols; c++) {
		dvalue = tci[SEG_INDEX(wat_seg, r, c)];
		watvalue = wat[SEG_INDEX(wat_seg, r, c)];
		if (!Rast_is_d_null_value(&watvalue)) {
		    dbuf[c] = dvalue;
		    sum += dvalue;
		    sum_sqr += dvalue * dvalue;
		}
	    }
	    Rast_put_row(fd, dbuf, DCELL_TYPE);
	}
	Rast_close(fd);

	mean = sum / do_points;
	stddev =
	    sqrt((sum_sqr - (sum + sum / do_points)) / (do_points - 1));
	G_debug(1, "stddev: %f", stddev);

	/* set nice color rules: yellow, green, cyan, blue, black */

	lstddev = log(stddev);

	Rast_read_fp_range(tci_name, this_mapset, &accRange);
	min = max = 0;
	Rast_get_fp_range_min_max(&accRange, &min, &max);

	Rast_init_colors(&colors);

	if (min - 1 < mean - 0.5 * stddev) {
	    clr_min = min - 1;
	    clr_max = mean - 0.5 * stddev;
	    Rast_add_d_color_rule(&clr_min, 255, 255, 0, &clr_max, 255,
					  255, 0, &colors);
	}

	clr_min = mean - 0.5 * stddev;
	clr_max = mean - 0.2 * stddev;
	Rast_add_d_color_rule(&clr_min, 255, 255, 0, &clr_max, 0,
				  255, 0, &colors);
	clr_min = clr_max;
	clr_max = mean + 0.2 * stddev;
	Rast_add_d_color_rule(&clr_min, 0, 255, 0, &clr_max, 0,
				  255, 255, &colors);
	clr_min = clr_max;
	clr_max = mean + 0.6 * stddev;
	Rast_add_d_color_rule(&clr_min, 0, 255, 255, &clr_max, 0,
				  0, 255, &colors);
	clr_min = clr_max;
	clr_max = mean + 1. * stddev;
	Rast_add_d_color_rule(&clr_min, 0, 0, 255, &clr_max, 0, 0,
				  0, &colors);

	if (max > 0 && max > clr_max) {
	    clr_min = clr_max;
	    clr_max = max + 1;
	    Rast_add_d_color_rule(&clr_min, 0, 0, 0, &clr_max, 0, 0,
				      0, &colors);
	}
	Rast_write_colors(tci_name, this_mapset, &colors);
    }

    /* TODO: elevation == NULL -> drainage direction == NULL (wat == 0 where ele == NULL) */
    /* keep drainage direction == 0 for real depressions */
    if (asp_flag) {
	fd = Rast_open_c_new(asp_name);
	for (r = 0; r < nrows; r++) {
	    Rast_set_c_null_value(buf, ncols);	/* reset row to all NULL */
	    for (c = 0; c < ncols; c++) {
		dvalue = wat[SEG_INDEX(wat_seg, r, c)];
		if (!Rast_is_d_null_value(&dvalue))
		    buf[c] = asp[SEG_INDEX(asp_seg, r, c)];
	    }
	    Rast_put_row(fd, buf, CELL_TYPE);
	}
	Rast_close(fd);
	Rast_init_colors(&colors);
	Rast_make_aspect_colors(&colors, -8, 8);
	Rast_write_colors(asp_name, this_mapset, &colors);
    }
    G_free(asp);

    flag_destroy(swale);
    /*
       Rast_free_colors(&colors);
     */
    G_free(wat);

    if (ls_flag) {
	fd = Rast_open_new(ls_name, DCELL_TYPE);
	for (r = 0; r < nrows; r++) {
	    for (c = 0; c < ncols; c++) {
		dbuf[c] = l_s[SEG_INDEX(l_s_seg, r, c)];
	    }
	    Rast_put_row(fd, dbuf, DCELL_TYPE);
	}
	Rast_close(fd);
	G_free(l_s);
    }

    if (sl_flag) {
	fd = Rast_open_new(sl_name, DCELL_TYPE);
	for (r = 0; r < nrows; r++) {
	    for (c = 0; c < ncols; c++) {
		dbuf[c] = s_l[SEG_INDEX(s_l_seg, r, c)];
		if (dbuf[c] > max_length)
		    dbuf[c] = max_length;
	    }
	    Rast_put_row(fd, dbuf, DCELL_TYPE);
	}
	Rast_close(fd);
    }

    if (sl_flag || ls_flag || sg_flag)
	G_free(s_l);

    if (sg_flag) {
	fd = Rast_open_new(sg_name, DCELL_TYPE);
	for (r = 0; r < nrows; r++) {
	    for (c = 0; c < ncols; c++) {
		dbuf[c] = s_g[SEG_INDEX(s_g_seg, r, c)];
	    }
	    Rast_put_row(fd, dbuf, DCELL_TYPE);
	}
	Rast_close(fd);
	G_free(s_g);
    }

    return 0;
}
