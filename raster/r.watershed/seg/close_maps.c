#include "Gwater.h"
#include <unistd.h>

int 
close_maps (void)
{
    struct Colors colors;
    int r, c;
    CELL is_swale, value;
    double	dvalue;

    dseg_close (&slp);
    cseg_close (&alt);
    if (wat_flag)
	cseg_write_cellfile (&wat, wat_name);
    if (asp_flag) {
	cseg_write_cellfile (&asp, asp_name);
	G_init_colors (&colors);
	G_make_grey_scale_colors (&colors, 1, 8);
	G_write_colors (asp_name, this_mapset, &colors);
    }
    cseg_close (&asp);
    if (dis_flag) {
	if (bas_thres <= 0) 	bas_thres = 60;
	for (r=0; r<nrows; r++) {
	    for (c=0; c<ncols; c++) {
		cseg_get (&wat, &value, r, c);
		if (value < 0) {
		    value = 0;
		    cseg_put (&wat, &value, r, c);
		} else {
			bseg_get (&swale, &is_swale, r, c);
			if (is_swale) {
	 			value = bas_thres;
				cseg_put (&wat, &value, r, c);
			}
		}
	    }
	}
	cseg_write_cellfile (&wat, dis_name);
	G_init_colors (&colors);
	G_make_rainbow_colors (&colors, 1, 120);
	G_write_colors (dis_name, this_mapset, &colors);
    }
    /* error in gislib.a
    G_free_colors(&colors);
    */
    cseg_close (&wat);
    if (ls_flag) {
	dseg_write_cellfile (&l_s, ls_name);
        dseg_close (&l_s);
    }
    bseg_close (&swale);
    if (sl_flag) {
	for (r = 0; r < nrows; r++) {
		for (c = 0; c < ncols; c++) {
			dseg_get (&s_l, &dvalue, r, c);
			if (dvalue > max_length)
				dseg_put (&s_l, &max_length, r, c);
		}
	}
	dseg_write_cellfile (&s_l, sl_name);
    }
    if (sl_flag || ls_flag || sg_flag)
        dseg_close (&s_l);
    if (ril_flag)
        dseg_close (&ril);
    if (sg_flag)
	dseg_write_cellfile (&s_g, sg_name);
    if (sg_flag)
        dseg_close (&s_g);
    if (ls_flag || sg_flag)
        cseg_close (&r_h);

    return 0;
}
