#include "Gwater.h"
#include <unistd.h>
#include <grass/gis.h>
#include <grass/glocale.h>


int close_maps(void)
{
    struct Colors colors;
    int value, r, c, fd;
    CELL *buf = NULL;
    DCELL *dbuf = NULL;
    struct FPRange accRange;
    DCELL min, max;
    DCELL clr_min, clr_max;
    DCELL sum, sum_sqr, stddev, lstddev, dvalue;

    if (asp_flag || dis_flag)
	buf = G_allocate_cell_buf();
    if (wat_flag || ls_flag || sl_flag || sg_flag)
	dbuf = G_allocate_d_raster_buf();
    G_free(alt);
    if (ls_flag || sg_flag)
	G_free(r_h);

    sum = sum_sqr = stddev = 0.0;
    if (wat_flag) {
	fd = G_open_raster_new(wat_name, DCELL_TYPE);
	if (fd < 0) {
	    G_warning(_("unable to open new accum map layer."));
	}
	else {
	    if (abs_acc) {
		G_warning("Writing out only positive flow accumulation values.");
		G_warning("Cells with a likely underestimate for flow accumulation can no longer be identified.");
		for (r = 0; r < nrows; r++) {
		    G_set_d_null_value(dbuf, ncols);	/* reset row to all NULL */
		    for (c = 0; c < ncols; c++) {
			dvalue = wat[SEG_INDEX(wat_seg, r, c)];
			if (G_is_d_null_value(&dvalue) == 0 && dvalue) {
			    dvalue = ABS(dvalue);
			    dbuf[c] = dvalue;
			    sum += dvalue;
			    sum_sqr += dvalue * dvalue;
			}
		    }
		    G_put_raster_row(fd, dbuf, DCELL_TYPE);
		}
	    }
	    else {
		for (r = 0; r < nrows; r++) {
		    G_set_d_null_value(dbuf, ncols);	/* reset row to all NULL */
		    for (c = 0; c < ncols; c++) {
			dvalue = wat[SEG_INDEX(wat_seg, r, c)];
			if (G_is_d_null_value(&dvalue) == 0 && dvalue) {
			    dbuf[c] = dvalue;
			    dvalue = ABS(dvalue);
			    sum += dvalue;
			    sum_sqr += dvalue * dvalue;
			}
		    }
		    G_put_raster_row(fd, dbuf, DCELL_TYPE);
		}
	    }
	    if (G_close_cell(fd) < 0)
		G_warning(_("Close failed."));

	    stddev =
		sqrt((sum_sqr - (sum + sum / do_points)) / (do_points - 1));
	    G_debug(1, "stddev: %f", stddev);

	    /* set nice color rules: yellow, green, cyan, blue, black */

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
		G_add_d_raster_color_rule(&clr_min, 0, 0, 0, &clr_max, 0, 0,
					  0, &colors);
	    }
	    G_write_colors(wat_name, this_mapset, &colors);
	}
    }

    /* TODO: elevation == NULL -> drainage direction == NULL (wat == 0 where ele == NULL) */
    /* keep drainage direction == 0 for real depressions */
    if (asp_flag) {
	fd = G_open_cell_new(asp_name);
	if (fd < 0) {
	    G_warning(_("unable to open new aspect map layer."));
	}
	else {
	    for (r = 0; r < nrows; r++) {
		G_set_c_null_value(buf, ncols);	/* reset row to all NULL */
		for (c = 0; c < ncols; c++) {
		    buf[c] = asp[SEG_INDEX(asp_seg, r, c)];
		}
		G_put_raster_row(fd, buf, CELL_TYPE);
	    }
	    if (G_close_cell(fd) < 0)
		G_warning(_("Close failed."));
	}
	G_init_colors(&colors);
	G_make_aspect_colors(&colors, 0, 8);
	G_write_colors(asp_name, this_mapset, &colors);
    }
    G_free(asp);

    /* visual output no longer needed */
    if (dis_flag) {
	fd = G_open_cell_new(dis_name);
	if (fd < 0) {
	    G_warning(_("unable to open new accum map layer."));
	}
	else {
	    if (bas_thres <= 0)
		bas_thres = 60;
	    for (r = 0; r < nrows; r++) {
		for (c = 0; c < ncols; c++) {
		    buf[c] = wat[SEG_INDEX(wat_seg, r, c)];
		    if (buf[c] < 0) {
			buf[c] = 0;
		    }
		    else {
			value = FLAG_GET(swale, r, c);
			if (value) {
			    buf[c] = bas_thres;
			}
		    }
		}
		G_put_raster_row(fd, buf, CELL_TYPE);
	    }
	    if (G_close_cell(fd) < 0)
		G_warning(_("Close failed."));
	}
	G_init_colors(&colors);
	G_make_rainbow_colors(&colors, 1, 120);
	G_write_colors(dis_name, this_mapset, &colors);
    }
    flag_destroy(swale);
    /*
       G_free_colors(&colors);
     */
    G_free(wat);

    if (ls_flag) {
	fd = G_open_raster_new(ls_name, DCELL_TYPE);
	if (fd < 0) {
	    G_warning(_("unable to open new LS factor map layer."));
	}
	else {
	    for (r = 0; r < nrows; r++) {
		for (c = 0; c < ncols; c++) {
		    dbuf[c] = l_s[SEG_INDEX(l_s_seg, r, c)];
		}
		G_put_raster_row(fd, dbuf, DCELL_TYPE);
	    }
	    if (G_close_cell(fd) < 0)
		G_warning(_("Close failed."));
	}
	G_free(l_s);
    }

    if (sl_flag) {
	fd = G_open_raster_new(sl_name, DCELL_TYPE);
	if (fd < 0) {
	    G_warning(_("unable to open new slope length map layer."));
	}
	else {
	    for (r = 0; r < nrows; r++) {
		for (c = 0; c < ncols; c++) {
		    dbuf[c] = s_l[SEG_INDEX(s_l_seg, r, c)];
		    if (dbuf[c] > max_length)
			dbuf[c] = max_length;
		}
		G_put_raster_row(fd, dbuf, DCELL_TYPE);
	    }
	    if (G_close_cell(fd) < 0)
		G_warning(_("Close failed."));
	}
    }

    if (sl_flag || ls_flag || sg_flag)
	G_free(s_l);

    if (sg_flag) {
	fd = G_open_raster_new(sg_name, DCELL_TYPE);
	if (fd < 0) {
	    G_warning(_("unable to open new S factor map layer."));
	}
	else {
	    for (r = 0; r < nrows; r++) {
		for (c = 0; c < ncols; c++) {
		    dbuf[c] = s_g[SEG_INDEX(s_g_seg, r, c)];
		}
		G_put_raster_row(fd, dbuf, DCELL_TYPE);
	    }
	    if (G_close_cell(fd) < 0)
		G_warning(_("Close failed."));
	}
	G_free(s_g);
    }

    return 0;
}
