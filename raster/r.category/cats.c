/****************************************************************************
 *
 * MODULE:       r.cats
 *
 * AUTHOR(S):    Michael Shapiro - CERL
 *
 * PURPOSE:      Prints category values and labels associated with
 *               user-specified raster map layers.
 *
 * SPDX-FileCopyrightText: 2006 Other GRASS authors
 * SPDX-License-Identifier: GPL-2.0-or-later
 ***************************************************************************/

#include <stdlib.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include "local_proto.h"

static struct Cell_stats statf;

int get_cats(const char *name, const char *mapset)
{
    int fd;
    int row, nrows, ncols;
    CELL *cell;
    struct Cell_head cellhd;

    /* set the window to the cell header */
    Rast_get_cellhd(name, mapset, &cellhd);

    Rast_set_window(&cellhd);

    /* open the raster map */
    fd = Rast_open_old(name, mapset);
    nrows = Rast_window_rows();
    ncols = Rast_window_cols();
    cell = Rast_allocate_c_buf();
    Rast_init_cell_stats(&statf);

    /* read the raster map */
    G_verbose_message(_("Reading <%s> in <%s>"), name, mapset);
    for (row = 0; row < nrows; row++) {
        if (G_verbose() > G_verbose_std())
            G_percent(row, nrows, 2);
        Rast_get_c_row_nomask(fd, cell, row);
        Rast_update_cell_stats(cell, ncols, &statf);
    }
    /* done */
    if (G_verbose() > G_verbose_std())
        G_percent(row, nrows, 2);
    Rast_close(fd);
    G_free(cell);
    Rast_rewind_cell_stats(&statf);

    return 0;
}

int next_cat(long *x)
{
    long count;
    CELL cat;

    if (Rast_next_cell_stat(&cat, &count, &statf)) {
        *x = cat;
        return 1;
    }

    return 0;
}

