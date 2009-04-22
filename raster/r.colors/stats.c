
/****************************************************************************
 *
 * MODULE:       r.colors
 *
 * AUTHOR(S):    Michael Shapiro - CERL
 *               David Johnson
 *
 * PURPOSE:      Allows creation and/or modification of the color table 
 *               for a raster map layer.
 *
 * COPYRIGHT:    (C) 2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 ***************************************************************************/

#include <math.h>
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "local_proto.h"

int get_stats(const char *name, const char *mapset, struct Cell_stats *statf)
{
    CELL *cell;
    int row, nrows, ncols;
    int fd;

    if ((fd = G_open_cell_old(name, mapset)) < 0)
	G_fatal_error(_("Unable to open raster map <%s>"),
		      G_fully_qualified_name(name, mapset));

    cell = G_allocate_cell_buf();
    nrows = G_window_rows();
    ncols = G_window_cols();

    G_init_cell_stats(statf);
    G_verbose_message(_("Reading raster map <%s>..."),
		      G_fully_qualified_name(name, mapset));
    for (row = 0; row < nrows; row++) {
	G_percent(row, nrows, 2);
	if (G_get_c_raster_row(fd, cell, row) < 0)
	    G_fatal_error(_("Unable to read raster map <%s> row %d"),
			  G_fully_qualified_name(name, mapset), row);
	G_update_cell_stats(cell, ncols, statf);
    }
    G_percent(row, nrows, 2);
    G_close_cell(fd);
    G_free(cell);

    return 1;
}

void get_fp_stats(const char *name, const char *mapset,
		  struct FP_stats *statf,
		  DCELL min, DCELL max, int geometric, int geom_abs)
{
    DCELL *dcell;
    int row, col, nrows, ncols;
    int fd;

    if ((fd = G_open_cell_old(name, mapset)) < 0)
	G_fatal_error("Unable to open raster map <%s>",
		      G_fully_qualified_name(name, mapset));

    dcell = G_allocate_d_raster_buf();
    nrows = G_window_rows();
    ncols = G_window_cols();

    statf->geometric = geometric;
    statf->geom_abs = geom_abs;
    statf->flip = 0;

    if (statf->geometric) {
	if (min * max < 0)
	    G_fatal_error(_("Unable to use logarithmic scaling if range includes zero"));

	if (min < 0) {
	    statf->flip = 1;
	    min = -min;
	    max = -max;
	}

	min = log(min);
	max = log(max);
    }

    if (statf->geom_abs) {
	double a = log(fabs(min) + 1);
	double b = log(fabs(max) + 1);
	int has_zero = min * max < 0;
	min = a < b ? a : b;
	max = a > b ? a : b;
	if (has_zero)
	    min = 0;
    }

    statf->count = 1000;
    statf->min = min;
    statf->max = max;
    statf->stats = G_calloc(statf->count + 1, sizeof(unsigned long));
    statf->total = 0;

    G_verbose_message(_("Reading raster map <%s>..."),
		      G_fully_qualified_name(name, mapset));
    for (row = 0; row < nrows; row++) {
	G_percent(row, nrows, 2);

	if (G_get_d_raster_row(fd, dcell, row) < 0)
	    G_fatal_error(_("Unable to read raster map <%s> row %d"),
			  G_fully_qualified_name(name, mapset), row);
	
	for (col = 0; col < ncols; col++) {
	    DCELL x;
	    int i;

	    if (G_is_d_null_value(&dcell[col]))
		continue;

	    x = dcell[col];
	    if (statf->flip)
		x = -x;
	    if (statf->geometric)
		x = log(x);
	    if (statf->geom_abs)
		x = log(fabs(x) + 1);

	    i = (int) floor(statf->count * (x - statf->min) / (statf->max - statf->min));
	    statf->stats[i]++;
	    statf->total++;
	}
    }

    G_percent(row, nrows, 2);
    G_close_cell(fd);
    G_free(dcell);
}
