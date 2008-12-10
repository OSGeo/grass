#include "Gwater.h"
#include <unistd.h>

int close_maps(void)
{
    struct Colors colors, logcolors;
    int r, c;
    CELL is_swale;
    double dvalue;
    struct FPRange accRange;
    DCELL min, max;
    DCELL clr_min, clr_max;

    dseg_close(&slp);
    cseg_close(&alt);
    if (wat_flag) {
	dseg_write_cellfile(&wat, wat_name);

	/* set nice color rules: yellow, green, cyan, blue, black */
	G_read_fp_range(wat_name, this_mapset, &accRange);
	min = max = 0;
	G_get_fp_range_min_max(&accRange, &min, &max);

	G_init_colors(&colors);
	if (ABS(min) > max) {
	    clr_min = 1;
	    clr_max = 0.25 * max;
	    G_add_d_raster_color_rule(&clr_min, 255, 255, 0, &clr_max, 0, 255,
				      0, &colors);
	    clr_min = 0.25 * max;
	    clr_max = 0.5 * max;
	    G_add_d_raster_color_rule(&clr_min, 0, 255, 0, &clr_max, 0, 255,
				      255, &colors);
	    clr_min = 0.5 * max;
	    clr_max = 0.75 * max;
	    G_add_d_raster_color_rule(&clr_min, 0, 255, 255, &clr_max, 0, 0,
				      255, &colors);
	    clr_min = 0.75 * max;
	    clr_max = max;
	    G_add_d_raster_color_rule(&clr_min, 0, 0, 255, &clr_max, 0, 0, 0,
				      &colors);
	    max = -max;
	}
	else {
	    min = ABS(min);
	    clr_min = 1;
	    clr_max = 0.25 * min;
	    G_add_d_raster_color_rule(&clr_min, 255, 255, 0, &clr_max, 0, 255,
				      0, &colors);
	    clr_min = 0.25 * min;
	    clr_max = 0.5 * min;
	    G_add_d_raster_color_rule(&clr_min, 0, 255, 0, &clr_max, 0, 255,
				      255, &colors);
	    clr_min = 0.5 * min;
	    clr_max = 0.75 * min;
	    G_add_d_raster_color_rule(&clr_min, 0, 255, 255, &clr_max, 0, 0,
				      255, &colors);
	    clr_min = 0.75 * min;
	    clr_max = min;
	    G_add_d_raster_color_rule(&clr_min, 0, 0, 255, &clr_max, 0, 0, 0,
				      &colors);
	}
	G_abs_log_colors(&logcolors, &colors, 5);
	G_add_d_raster_color_rule(&min, 0, 0, 0, &max, 0, 0, 0, &logcolors);
	G_write_colors(wat_name, this_mapset, &logcolors);

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
