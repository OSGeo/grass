/*
 **********************************************************************
 *
 * MODULE:        r.support.stats
 *
 * AUTHOR(S):     Brad Douglas <rez touchofmadness com>
 *
 * PURPOSE:       Update raster statistics
 *
 * COPYRIGHT:     (C) 2006 by the GRASS Development Team
 *
 *                This program is free software under the GNU General
 *                Purpose License (>=v2). Read the file COPYING that
 *                comes with GRASS for details.
 *
 ***********************************************************************/

#include <stdlib.h>
#include <grass/gis.h>
#include <grass/raster.h>


/* 
 * do_histogram() - Creates histogram for CELL
 *
 * RETURN: 0 on success / 1 on failure
 */
int do_histogram(const char *name)
{
    CELL *cell;
    struct Cell_head cellhd;
    struct Cell_stats statf;
    int nrows, ncols;
    int row;
    int fd;

    Rast_get_cellhd(name, "", &cellhd);

    Rast_set_window(&cellhd);
    fd = Rast_open_old(name, "");

    nrows = Rast_window_rows();
    ncols = Rast_window_cols();
    cell = Rast_allocate_c_buf();

    Rast_init_cell_stats(&statf);

    /* Update statistics for each row */
    for (row = 0; row < nrows; row++) {
	G_percent(row, nrows, 2);

	Rast_get_c_row_nomask(fd, cell, row);

	Rast_update_cell_stats(cell, ncols, &statf);
    }

    /* Write histogram if it made it through the loop */
    if (row == nrows)
	Rast_write_histogram_cs(name, &statf);

    Rast_free_cell_stats(&statf);
    Rast_close(fd);
    G_free(cell);

    if (row == nrows)
	return 0;

    return 1;
}
