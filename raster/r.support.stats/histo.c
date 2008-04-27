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


/* 
 * do_histogram() - Creates histogram for CELL
 *
 * RETURN: 0 on success / 1 on failure
 */
int do_histogram (char *name, char *mapset)
{
    CELL *cell;
    struct Cell_head cellhd;
    struct Cell_stats statf;
    int nrows, ncols;
    int row;
    int fd;

    if (G_get_cellhd(name, mapset, &cellhd) < 0)
        return 1;

    G_set_window(&cellhd);
    if ((fd = G_open_cell_old(name, mapset)) < 0)
        return 1;

    nrows = G_window_rows();
    ncols = G_window_cols();
    cell = G_allocate_cell_buf();

    G_init_cell_stats(&statf);

    /* Update statistics for each row */
    for (row = 0; row < nrows; row++) {
        G_percent(row, nrows, 10);

        if (G_get_map_row_nomask(fd, cell, row) < 0)
            break;

        G_update_cell_stats(cell, ncols, &statf);
    }

    /* Write histogram if it made it through the loop */
    if (row == nrows)
        G_write_histogram_cs(name, &statf);

    G_free_cell_stats(&statf);
    G_close_cell(fd);
    G_free(cell);

    if (row == nrows)
        return 0;

    return 1;
}
