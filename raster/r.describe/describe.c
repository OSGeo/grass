
/****************************************************************************
 *
 * MODULE:       r.describe
 *
 * AUTHOR(S):    Michael Shapiro - CERL
 *
 * PURPOSE:      Prints terse list of category values found in a raster
 *               map layer.
 *
 * COPYRIGHT:    (C) 2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 ***************************************************************************/

#include <grass/gis.h>
#include "local_proto.h"


int describe(char *name, char *mapset, int compact, char *no_data_str,
	     int range, int windowed, int nsteps, int as_int, int skip_nulls)
{
    int fd;
    struct Cell_stats statf;
    CELL *buf, *b;
    int nrows, ncols;
    int row, col;
    struct Cell_head window;
    CELL negmin = 0, negmax = 0, zero = 0, posmin = 0, posmax = 0;
    CELL null = 0;
    RASTER_MAP_TYPE map_type;
    struct Quant q;
    struct FPRange r;
    DCELL dmin, dmax;
    int (*get_row) ();

    if (windowed) {
	get_row = G_get_c_raster_row;
    }
    else {
	char msg[100];

	if (G_get_cellhd(name, mapset, &window) < 0) {
	    sprintf(msg, "can't get cell header for [%s] in [%s]", name,
		    mapset);
	    G_fatal_error(msg);
	}
	G_set_window(&window);
	get_row = G_get_c_raster_row_nomask;
    }
    fd = G_open_cell_old(name, mapset);
    if (fd < 0)
	return 0;

    map_type = G_get_raster_map_type(fd);
    if (as_int)
	map_type = CELL_TYPE;	/* read as int */

    /* allocate the cell buffer */
    buf = G_allocate_cell_buf();

    if (map_type != CELL_TYPE && range)
	/* this will make it report fp range */
    {
	range = 0;
	nsteps = 1;
    }

    /* start the cell stats */
    if (!range) {
	G_init_cell_stats(&statf);
    }
    else {
	zero = 0;
	negmin = 0;
	negmax = 0;
	posmin = 0;
	posmax = 0;
	null = 0;
	dmin = 0.0;
	dmax = 0.0;
    }

    /* set up quantization rules */
    if (map_type != CELL_TYPE) {
	G_quant_init(&q);
	G_read_fp_range(name, mapset, &r);
	G_get_fp_range_min_max(&r, &dmin, &dmax);
	G_quant_add_rule(&q, dmin, dmax, 1, nsteps);
	G_set_quant_rules(fd, &q);
    }

    nrows = G_window_rows();
    ncols = G_window_cols();

    G_verbose_message("Reading [%s in %s] ...", name, mapset);
    for (row = 0; row < nrows; row++) {
	G_percent(row, nrows, 2);
	if ((*get_row) (fd, b = buf, row) < 0)
	    break;
	if (range) {
	    for (col = ncols; col-- > 0; b++) {
		if (G_is_c_null_value(b))
		    null = 1;
		else if (*b == 0)
		    zero = 1;
		else if (*b < 0) {
		    if (!negmin)
			negmin = negmax = *b;
		    else if (*b > negmax)
			negmax = *b;
		    else if (*b < negmin)
			negmin = *b;
		}
		else {
		    if (!posmin)
			posmin = posmax = *b;
		    else if (*b > posmax)
			posmax = *b;
		    else if (*b < posmin)
			posmin = *b;
		}
	    }
	}
	else
	    G_update_cell_stats(buf, ncols, &statf);
    }
    G_percent(nrows, nrows, 2);
    G_close_cell(fd);
    G_free(buf);

    if (range) {
	if (compact)
	    compact_range_list(negmin, negmax, zero, posmin, posmax, null,
			       no_data_str, skip_nulls);
	else
	    range_list(negmin, negmax, zero, posmin, posmax, null,
		       no_data_str, skip_nulls);
    }
    else {
	G_rewind_cell_stats(&statf);

	if (compact)
	    compact_list(&statf, dmin, dmax, no_data_str, skip_nulls,
			 map_type, nsteps);
	else
	    long_list(&statf, dmin, dmax, no_data_str, skip_nulls, map_type,
		      nsteps);

	G_free_cell_stats(&statf);
    }
    return 1;
}
