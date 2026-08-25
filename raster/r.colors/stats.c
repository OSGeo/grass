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
#include "local_proto.h"

int get_stats(struct maps_info *input_maps, struct Cell_stats *statf)
{
    CELL *cell;
    int row, nrows, ncols;
    int fd;
    int i;

    Rast_init_cell_stats(statf);

    for (i = 0; i < input_maps->num; i++) {
        fd = Rast_open_old(input_maps->names[i], input_maps->mapsets[i]);

        cell = Rast_allocate_c_buf();
        nrows = Rast_window_rows();
        ncols = Rast_window_cols();
        char *mname = G_fully_qualified_name(input_maps->names[i],
                                             input_maps->mapsets[i]);

        G_verbose_message(_("(%i/%i) Reading raster map <%s>..."), i + 1,
                          input_maps->num, mname);
        G_free(mname);

        for (row = 0; row < nrows; row++) {
            G_percent(row, nrows, 2);
            Rast_get_c_row(fd, cell, row);
            Rast_update_cell_stats(cell, ncols, statf);
        }
        G_percent(nrows, nrows, 2);
        Rast_close(fd);
        G_free(cell);
    }

    return 1;
}

void get_fp_stats(struct maps_info *input_maps, struct FP_stats *statf,
                  DCELL min, DCELL max, int geometric, int geom_abs, int type)
{
    DCELL *dcell = NULL;
    int row, col, depth, nrows, ncols, ndepths = 1;
    int fd;
    int i;
    char *name;
    char *mapset;
    RASTER3D_Map *map3d = NULL;

    statf->geometric = geometric;
    statf->geom_abs = geom_abs;
    statf->flip = 0;

    if (statf->geometric) {
        if (min * max < 0)
            G_fatal_error(
                _("Unable to use logarithmic scaling if range includes zero"));

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

    /* Loop over all input maps */
    for (i = 0; i < input_maps->num; i++) {
        name = input_maps->names[i];
        mapset = input_maps->mapsets[i];

        fd = -1;
        if (type == RASTER_TYPE) {
            fd = Rast_open_old(name, mapset);
            dcell = Rast_allocate_d_buf();
            nrows = Rast_window_rows();
            ncols = Rast_window_cols();
        }
        else {
            /* Initiate the default settings */
            Rast3d_init_defaults();

            map3d = Rast3d_open_cell_old(name, mapset, RASTER3D_DEFAULT_WINDOW,
                                         RASTER3D_TILE_SAME_AS_FILE,
                                         RASTER3D_USE_CACHE_DEFAULT);

            if (map3d == NULL)
                Rast3d_fatal_error(_("Error opening 3d raster map"));

            nrows = map3d->window.rows;
            ncols = map3d->window.cols;
            ndepths = map3d->window.depths;
        }
        char *mname = G_fully_qualified_name(name, mapset);
        G_verbose_message(_("(%i/%i) Reading map <%s>..."), i, input_maps->num,
                          mname);
        G_free(mname);

        for (depth = 0; depth < ndepths; depth++) {
            for (row = 0; row < nrows; row++) {
                G_percent(row, nrows, 2);

                if (type == RASTER_TYPE)
                    Rast_get_d_row(fd, dcell, row);

                for (col = 0; col < ncols; col++) {
                    DCELL x;
                    int j;

                    if (type == RASTER_TYPE)
                        x = dcell[col];
                    else
                        x = Rast3d_get_double(map3d, col, row, depth);

                    if (Rast_is_d_null_value(&x))
                        continue;

                    if (statf->flip)
                        x = -x;
                    if (statf->geometric)
                        x = log(x);
                    if (statf->geom_abs)
                        x = log(fabs(x) + 1);

                    j = (int)floor(statf->count * (x - statf->min) /
                                   (statf->max - statf->min));
                    statf->stats[j]++;
                    statf->total++;
                }
            }
        }

        G_percent(nrows, nrows, 2);

        if (type == RASTER_TYPE) {
            Rast_close(fd);
            if (dcell)
                G_free(dcell);
        }
        else {
            Rast3d_close(map3d);
            map3d = NULL;
        }
    }
}
