#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

#include "execute.h"

/* *************************************************************************************************
 *
 * Compute of the matrix S.G.L.D. (Spatial Gray-Level Dependence Matrices)
 *or co-occurrence matrix. The image is analyzed for piece, every piece is
 *naming moving window (s.w.). The s.w. must be square with number of size's
 *samples odd, that because we want the sample at the center of matrix.
 *
 ***************************************************************************************************/
int execute_texture(CELL **data, struct dimensions *dim,
                    struct menu *measure_menu, int *measure_idx,
                    struct output_setting *out_set)

{
    int size = dim->size;
    int dist = dim->dist;
    int nrows = dim->nrows;
    int ncols = dim->ncols;
    int n_outputs = dim->n_outputs;
    int n_measures = dim->n_measures;
    int *outfd = out_set->outfd;
    RASTER_MAP_TYPE out_data_type = out_set->out_data_type;
    struct Flag *flag_null = out_set->flag_null;
    struct Flag *flag_ind = out_set->flag_ind;

    int offset = size / 2;
    int i, j, row, col, first_row, first_col, last_row, last_col;
    int have_px, have_py, have_pxpys, have_pxpyd;
    FCELL **fbuf;
    FCELL measure; /* Containing measure done */
    struct matvec *mv;

    fbuf = G_malloc(n_outputs * sizeof(FCELL *));
    for (i = 0; i < n_outputs; i++)
        fbuf[i] = Rast_allocate_buf(out_data_type);

    mv = G_malloc(sizeof(struct matvec));
    alloc_vars(size, mv);

    /* variables needed */
    if (measure_menu[2].useme || measure_menu[11].useme ||
        measure_menu[12].useme)
        have_px = 1;
    else
        have_px = 0;
    if (measure_menu[11].useme || measure_menu[12].useme)
        have_py = 1;
    else
        have_py = 0;
    if (measure_menu[5].useme || measure_menu[6].useme || measure_menu[7].useme)
        have_pxpys = 1;
    else
        have_pxpys = 0;
    if (measure_menu[9].useme || measure_menu[10].useme)
        have_pxpyd = 1;
    else
        have_pxpyd = 0;

    if (!flag_null->answer) {
        first_row = first_col = offset;
        last_row = nrows - offset;
        last_col = ncols - offset;
    }
    else {
        /* no cropping at window margins */
        first_row = first_col = 0;
        last_row = nrows;
        last_col = ncols;
    }

    Rast_set_f_null_value(fbuf[0], ncols);

    for (row = 0; row < first_row; row++) {
        for (i = 0; i < n_outputs; i++) {
            Rast_put_row(outfd[i], fbuf[0], out_data_type);
        }
    }
    if (n_measures > 1)
        G_message(n_("Calculating %d texture measure",
                     "Calculating %d texture measures", n_measures),
                  n_measures);
    else
        G_message(_("Calculating %s..."), measure_menu[measure_idx[0]].desc);

    for (row = first_row; row < last_row; row++) {
        G_percent(row, nrows, 2);
        for (i = 0; i < n_outputs; i++)
            Rast_set_f_null_value(fbuf[i], ncols);

        /*process the data */
        for (col = first_col; col < last_col; col++) {
            if (!set_vars(mv, data, row, col, size, offset, dist,
                          flag_null->answer)) {
                for (i = 0; i < n_outputs; i++)
                    Rast_set_f_null_value(&(fbuf[i][col]), 1);
                continue;
            }
            /* for all angles (0, 45, 90, 135) */
            for (i = 0; i < 4; i++) {
                set_angle_vars(mv, i, have_px, have_py, have_pxpys, have_pxpyd);
                /* for all requested textural measures */
                for (j = 0; j < n_measures; j++) {

                    measure =
                        (FCELL)h_measure(measure_menu[measure_idx[j]].idx, mv);

                    if (flag_ind->answer) {
                        /* output for each angle separately */
                        fbuf[j * 4 + i][col] = measure;
                    }
                    else {
                        /* use average over all angles for each measure */
                        if (i == 0)
                            fbuf[j][col] = measure;
                        else if (i < 3)
                            fbuf[j][col] += measure;
                        else
                            fbuf[j][col] = (fbuf[j][col] + measure) / 4.0;
                    }
                }
            }
        }

        for (i = 0; i < n_outputs; i++)
            Rast_put_row(outfd[i], fbuf[i], out_data_type);
    }

    Rast_set_f_null_value(fbuf[0], ncols);
    for (row = last_row; row < nrows; row++) {
        for (i = 0; i < n_outputs; i++) {
            Rast_put_row(outfd[i], fbuf[0], out_data_type);
        }
    }
    G_percent(nrows, nrows, 1);

    for (i = 0; i < n_outputs; i++)
        G_free(fbuf[i]);
    G_free(fbuf);
    dealloc_vars(mv);
    G_free(mv);

    return 0;
}
