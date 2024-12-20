#if defined(_OPENMP)
#include <omp.h>
#endif

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
                    struct output_setting *out_set, int threads)
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
    int trow;
    int have_px, have_py, have_pxpys, have_pxpyd;

    FCELL ***fbuf_threads; /* Buffer for each thread */
    FCELL measure;         /* Containing measure done */
    struct matvec **mvs;   /* matrices and vectors for each thread */

    /* allocate memory*/
    /* fbuf_threads[0] is used for writing out when program is not in loops*/
    fbuf_threads = (FCELL ***)G_malloc(sizeof(FCELL **) * threads);
    for (i = 0; i < threads; i++) {
        fbuf_threads[i] = (FCELL **)G_malloc(n_outputs * sizeof(FCELL *));
        for (j = 0; j < n_outputs; j++)
            fbuf_threads[i][j] = Rast_allocate_buf(out_data_type);
    }

    mvs = (struct matvec **)G_malloc(sizeof(struct matvec *) * threads);
    for (i = 0; i < threads; i++) {
        mvs[i] = G_malloc(sizeof(struct matvec));
        alloc_vars(size, mvs[i]);
    }

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

    Rast_set_f_null_value(fbuf_threads[0][0], ncols);

    for (row = 0; row < first_row; row++) {
        for (i = 0; i < n_outputs; i++) {
            Rast_put_row(outfd[i], fbuf_threads[0][0], out_data_type);
        }
    }
    if (n_measures > 1)
        G_message(n_("Calculating %d texture measure",
                     "Calculating %d texture measures", n_measures),
                  n_measures);
    else
        G_message(_("Calculating %s..."), measure_menu[measure_idx[0]].desc);

#pragma omp parallel private(row, col, i, j, measure, trow) default(shared)
    {
#pragma omp for schedule(static, 1) ordered
        for (row = first_row; row < last_row; row++) {
            trow = row % threads; /* Obtain thread row id */
            G_percent(row, nrows, 2);

            /* initialize the output row */
            for (i = 0; i < n_outputs; i++)
                Rast_set_f_null_value(fbuf_threads[trow][i], ncols);

            /*process the data */
            for (col = first_col; col < last_col; col++) {
                if (!set_vars(mvs[trow], data, row, col, size, offset, dist,
                              flag_null->answer)) {
                    for (i = 0; i < n_outputs; i++)
                        Rast_set_f_null_value(&(fbuf_threads[trow][i][col]), 1);
                    continue;
                }
                /* for all angles (0, 45, 90, 135) */
                for (i = 0; i < 4; i++) {
                    set_angle_vars(mvs[trow], i, have_px, have_py, have_pxpys,
                                   have_pxpyd);
                    /* for all requested textural measures */
                    for (j = 0; j < n_measures; j++) {
                        measure = (FCELL)h_measure(
                            measure_menu[measure_idx[j]].idx, mvs[trow]);
                        if (flag_ind->answer) {
                            /* output for each angle separately */
                            fbuf_threads[trow][j * 4 + i][col] = measure;
                        }
                        else {
                            /* use average over all angles for each measure */
                            if (i == 0)
                                fbuf_threads[trow][j][col] = measure;
                            else if (i < 3)
                                fbuf_threads[trow][j][col] += measure;
                            else
                                fbuf_threads[trow][j][col] =
                                    (fbuf_threads[trow][j][col] + measure) /
                                    4.0;
                        }
                    }
                }
            }
#pragma omp ordered
            {
                for (i = 0; i < n_outputs; i++)
                    Rast_put_row(outfd[i], fbuf_threads[trow][i],
                                 out_data_type);
            }
        }
    } /* end of parallel section */

    Rast_set_f_null_value(fbuf_threads[0][0], ncols);
    for (row = last_row; row < nrows; row++) {
        for (i = 0; i < n_outputs; i++) {
            Rast_put_row(outfd[i], fbuf_threads[0][0], out_data_type);
        }
    }
    G_percent(nrows, nrows, 1);

    for (i = 0; i < threads; i++) {
        for (j = 0; j < n_outputs; j++)
            G_free(fbuf_threads[i][j]);
        G_free(fbuf_threads[i]);
    }
    G_free(fbuf_threads);

    for (i = 0; i < threads; i++) {
        dealloc_vars(mvs[i]);
        G_free(mvs[i]);
    }
    G_free(mvs);

    return 0;
}
