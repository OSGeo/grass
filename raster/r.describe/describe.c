
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
#include <grass/raster.h>
#include <grass/glocale.h>
#include "local_proto.h"


int describe(const char *name, int compact, char *no_data_str,
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
    void (*get_row)(int, CELL *, int);

    if (windowed) {
	get_row = Rast_get_c_row;
    }
    else {
	Rast_get_cellhd(name, "", &window);

	Rast_set_window(&window);
	get_row = Rast_get_c_row_nomask;
    }
    fd = Rast_open_old(name, "");

    map_type = Rast_get_map_type(fd);
    if (as_int)
	map_type = CELL_TYPE;	/* read as int */

    /* allocate the cell buffer */
    buf = Rast_allocate_c_buf();

    if (map_type != CELL_TYPE && range)
	/* this will make it report fp range */
    {
	range = 0;
	nsteps = 1;
    }

    /* start the cell stats */
    if (!range) {
	Rast_init_cell_stats(&statf);
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
	Rast_quant_init(&q);
	Rast_read_fp_range(name, "", &r);
	Rast_get_fp_range_min_max(&r, &dmin, &dmax);
	Rast_quant_add_rule(&q, dmin, dmax, 1, nsteps);
	Rast_set_quant_rules(fd, &q);
    }

    nrows = Rast_window_rows();
    ncols = Rast_window_cols();

    G_verbose_message(_("Reading <%s> ..."), name);
    for (row = 0; row < nrows; row++) {
	G_percent(row, nrows, 2);
	(*get_row) (fd, b = buf, row);
	if (range) {
	    for (col = ncols; col-- > 0; b++) {
		if (Rast_is_c_null_value(b))
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
	    Rast_update_cell_stats(buf, ncols, &statf);
    }
    G_percent(nrows, nrows, 2);
    Rast_close(fd);
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
	Rast_rewind_cell_stats(&statf);

	if (compact)
	    compact_list(&statf, dmin, dmax, no_data_str, skip_nulls,
			 map_type, nsteps);
	else
	    long_list(&statf, dmin, dmax, no_data_str, skip_nulls, map_type,
		      nsteps);

	Rast_free_cell_stats(&statf);
    }
    return 1;
}
