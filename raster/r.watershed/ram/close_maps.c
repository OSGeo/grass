#include "Gwater.h"
#include <unistd.h>
#include <grass/gis.h>
#include <grass/glocale.h>


int close_maps(void)
{
    struct Colors colors;
    int value, r, c, fd;
    CELL *buf = NULL;

    if (wat_flag || asp_flag || dis_flag || ls_flag || sl_flag || sg_flag)
	buf = G_allocate_cell_buf();
    G_free(alt);
    if (ls_flag || sg_flag)
	G_free(r_h);

    if (wat_flag) {
	fd = G_open_cell_new(wat_name);
	if (fd < 0) {
	    G_warning(_("unable to open new accum map layer."));
	}
	else {
	    for (r = 0; r < nrows; r++) {
		for (c = 0; c < ncols; c++) {
		    buf[c] = wat[SEG_INDEX(wat_seg, r, c)];
		}
		G_put_raster_row(fd, buf, CELL_TYPE);
	    }
	    if (G_close_cell(fd) < 0)
		G_warning(_("Close failed."));
	}
    }

    if (asp_flag) {
	fd = G_open_cell_new(asp_name);
	if (fd < 0) {
	    G_warning(_("unable to open new aspect map layer."));
	}
	else {
	    for (r = 0; r < nrows; r++) {
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
	fd = G_open_cell_new(ls_name);
	if (fd < 0) {
	    G_warning(_("unable to open new L map layer."));
	}
	else {
	    for (r = 0; r < nrows; r++) {
		for (c = 0; c < ncols; c++) {
		    buf[c] = l_s[SEG_INDEX(l_s_seg, r, c)] + .5;
		}
		G_put_raster_row(fd, buf, CELL_TYPE);
	    }
	    if (G_close_cell(fd) < 0)
		G_warning(_("Close failed."));
	}
	G_free(l_s);
    }

    if (sl_flag) {
	fd = G_open_cell_new(sl_name);
	if (fd < 0) {
	    G_warning(_("unable to open new slope length map layer."));
	}
	else {
	    for (r = 0; r < nrows; r++) {
		for (c = 0; c < ncols; c++) {
		    buf[c] = s_l[SEG_INDEX(s_l_seg, r, c)] + .5;
		    if (buf[c] > max_length)
			buf[c] = max_length + .5;
		}
		G_put_raster_row(fd, buf, CELL_TYPE);
	    }
	    if (G_close_cell(fd) < 0)
		G_warning(_("Close failed."));
	}
    }

    if (sl_flag || ls_flag || sg_flag)
	G_free(s_l);

    if (sg_flag) {
	fd = G_open_cell_new(sg_name);
	if (fd < 0) {
	    G_warning(_("unable to open new S map layer."));
	}
	else {
	    for (r = 0; r < nrows; r++) {
		for (c = 0; c < ncols; c++) {
		    buf[c] = s_g[SEG_INDEX(s_g_seg, r, c)] * 100 + .5;
		}
		G_put_raster_row(fd, buf, CELL_TYPE);
	    }
	    if (G_close_cell(fd) < 0)
		G_warning(_("Close failed."));
	}
	G_free(s_g);
    }

    return 0;
}
