#include "Gwater.h"
#include <unistd.h>
#include <grass/glocale.h>

int close_maps(void)
{
    struct Colors colors;
    int r, c;
    DCELL *dbuf = NULL;
    CELL *cbuf = NULL;
    int fd;
    struct FPRange accRange;
    DCELL min, max;
    DCELL clr_min, clr_max;
    DCELL sum, sum_sqr, stddev, lstddev, dvalue;
    WAT_ALT *wabuf;
    ASP_FLAG *afbuf;

    if (rtn_flag)
        bseg_close(&rtn);

    if (wat_flag) {
	G_message(_("Closing accumulation map"));
	sum = sum_sqr = stddev = 0.0;
	dbuf = Rast_allocate_d_buf();
	wabuf = G_malloc(ncols * sizeof(WAT_ALT));
	seg_flush(&watalt);
	if (abs_acc) {
	    G_message("Writing out only positive flow accumulation values.");
	    G_message
		("Cells with a likely underestimate for flow accumulation can no longer be identified.");
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
	G_percent(r, nrows, 1);	/* finish it */

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

    /* TCI, SPI */
    if (atanb_flag) {
	DCELL *dbuf2;
	A_TANB sca_tanb;
	double mean;
	double sum2, sum_sqr2;
	int fd2;

	if (tci_flag && spi_flag)
	    G_message(_("Closing TCI and SPI maps"));
	else if (tci_flag && !spi_flag)
	    G_message(_("Closing TCI map"));
	else if (!tci_flag && spi_flag)
	    G_message(_("Closing SPI map"));

	sum = sum_sqr = stddev = 0.0;
	sum2 = sum_sqr2 = 0.0;
	wabuf = G_malloc(ncols * sizeof(WAT_ALT));
	seg_flush(&atanb);
	if (!wat_flag)
	    seg_flush(&watalt);

	fd = fd2 = -1;
	dbuf = dbuf2 = NULL;
	if (tci_flag) {
	    fd = Rast_open_new(tci_name, DCELL_TYPE);
	    dbuf = Rast_allocate_d_buf();
	}
	if (spi_flag) {
	    fd2 = Rast_open_new(spi_name, DCELL_TYPE);
	    dbuf2 = Rast_allocate_d_buf();
	}

	for (r = 0; r < nrows; r++) {
	    G_percent(r, nrows, 1);
	    if (tci_flag)
		Rast_set_d_null_value(dbuf, ncols);	/* reset row to all NULL */
	    if (spi_flag)
		Rast_set_d_null_value(dbuf2, ncols);	/* reset row to all NULL */
	    for (c = 0; c < ncols; c++) {
		seg_get(&atanb, (char *)&sca_tanb, r, c);
		if (!Rast_is_d_null_value(&sca_tanb.tanb)) {

		    if (tci_flag) {
			dvalue = log(sca_tanb.sca / sca_tanb.tanb);
			dbuf[c] = dvalue;
			sum += dvalue;
			sum_sqr += dvalue * dvalue;
		    }
		    if (spi_flag) {
			dvalue = sca_tanb.sca * sca_tanb.tanb;
			dbuf2[c] = dvalue;
			sum2 += dvalue;
			sum_sqr2 += dvalue * dvalue;
		    }
		}
	    }
	    if (tci_flag)
		Rast_put_row(fd, dbuf, DCELL_TYPE);
	    if (spi_flag)
		Rast_put_row(fd2, dbuf2, DCELL_TYPE);
	}
	G_percent(r, nrows, 1);	/* finish it */

	G_free(wabuf);
	seg_close(&atanb);

	if (tci_flag) {
	    Rast_close(fd);
	    G_free(dbuf);

	    mean = sum / do_points;
	    stddev =
		sqrt((sum_sqr - (sum + sum / do_points)) / (do_points - 1));
	    G_debug(1, "stddev: %f", stddev);

	    /* set nice color rules: yellow, green, cyan, blue, black */
	    /* start with white to get more detail? NULL cells are white by default, may be confusing */

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
	if (spi_flag) {
	    Rast_close(fd2);
	    G_free(dbuf2);

	    mean = sum2 / do_points;
	    stddev =
		sqrt((sum_sqr2 - (sum2 + sum2 / do_points)) / (do_points -
							       1));
	    G_debug(1, "stddev: %f", stddev);

	    /* set nice color rules: yellow, green, cyan, blue, black */
	    /* start with white to get more detail? NULL cells are white by default, may be confusing */

	    lstddev = log(stddev);

	    Rast_read_fp_range(spi_name, this_mapset, &accRange);
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
	    Rast_write_colors(spi_name, this_mapset, &colors);
	}
    }

    seg_close(&watalt);
    if (asp_flag) {
	G_message(_("Closing flow direction map"));
	cbuf = Rast_allocate_c_buf();
	afbuf = G_malloc(ncols * sizeof(ASP_FLAG));
	seg_flush(&aspflag);

	fd = Rast_open_new(asp_name, CELL_TYPE);

	for (r = 0; r < nrows; r++) {
	    G_percent(r, nrows, 1);
	    Rast_set_c_null_value(cbuf, ncols);	/* reset row to all NULL */
	    seg_get_row(&aspflag, (char *)afbuf, r);
	    for (c = 0; c < ncols; c++) {
		if (!FLAG_GET(afbuf[c].flag, NULLFLAG)) {
		    cbuf[c] = afbuf[c].asp;
		}
	    }
	    Rast_put_row(fd, cbuf, CELL_TYPE);
	}

	Rast_close(fd);

	Rast_init_colors(&colors);
	Rast_make_aspect_colors(&colors, -8, 8);
	Rast_write_colors(asp_name, this_mapset, &colors);
    }
    seg_close(&aspflag);
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
	G_percent(r, nrows, 1);	/* finish it */
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
