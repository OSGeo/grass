/*****************************************************************************/
/***                                                                       ***/
/***                             process()                                 ***/
/***     Parallelized version using OpenMP and RAM preload                 ***/
/***     Original: Jo Wood, Project ASSIST, V1.0 7th February 1993        ***/
/***                                                                       ***/
/*****************************************************************************/

#include <stdlib.h>
#include <omp.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include <grass/gmath.h>
#include "param.h"
#include "nrutil.h"

void process(void)
{
    struct Cell_head region;
    int nrows, ncols, row, col, wind_row;
    double temp;

    G_get_window(&region);
    nrows = Rast_window_rows();
    ncols = Rast_window_cols();
    if ((region.ew_res / region.ns_res >= 1.01) ||
        (region.ns_res / region.ew_res >= 1.01)) {
        G_warning(_("E-W and N-S grid resolutions are different. Taking average."));
        resoln = (region.ns_res + region.ew_res) / 2;
    }
    else
        resoln = region.ns_res;

    /*-----------------------------------------------------------------------*/
    /* Load entire input raster into RAM before parallel region.             */
    /* The original sliding buffer requires sequential row access, making    */
    /* direct row-level parallelism impossible. By preloading the full map   */
    /* into a flat 2D array, each thread can independently access any        */
    /* neighborhood window without lock contention or sequential dependency. */
    /*-----------------------------------------------------------------------*/
    DCELL *full_map = (DCELL *)G_malloc((size_t)nrows * ncols * sizeof(DCELL));
    for (row = 0; row < nrows; row++)
        Rast_get_d_row(fd_in, full_map + (size_t)row * ncols, row);

    /*-----------------------------------------------------------------------*/
    /* Compute weights and normal equations once before parallel region.     */
    /* normal_ptr and index_ptr are read-only during processing — safe to    */
    /* share across threads. weight_ptr is also read-only after setup.       */
    /*-----------------------------------------------------------------------*/
    double *weight_ptr = (double *)G_malloc(SQR(wsize) * sizeof(double));
    double **normal_ptr = dmatrix(0, 5, 0, 5);
    int *index_ptr = ivector(0, 5);

    find_weight(weight_ptr);
    find_normal(normal_ptr, weight_ptr);

    if (constrained)
        G_ludcmp(normal_ptr, 5, index_ptr, &temp);
    else
        G_ludcmp(normal_ptr, 6, index_ptr,&temp);

    /*-----------------------------------------------------------------------*/
    /* Pre allocate one output buffer per row so threads can write           */
    /* independently. Rast_put_row is called serially after the parallel     */
    /* region to ensure correct row ordering.                                */
    /*-----------------------------------------------------------------------*/
    void **row_buffers = (void **)G_malloc(nrows * sizeof(void *));
    for (row = 0; row < nrows; row++) {
        if (mparam != FEATURE) {
            row_buffers[row] = Rast_allocate_buf(DCELL_TYPE);
            Rast_set_d_null_value(row_buffers[row], ncols);
        }
        else {
            row_buffers[row] = Rast_allocate_buf(CELL_TYPE);
            Rast_set_c_null_value(row_buffers[row], ncols);
        }
    }

    /*-----------------------------------------------------------------------*/
    /* Parallel row loop. Each thread gets its own local computation         */
    /* buffers (t_window, t_obs) to avoid write contention.                  */
    /*-----------------------------------------------------------------------*/
    #pragma omp parallel for schedule(dynamic) private(row, col, wind_row)
    for (row = EDGE; row < (nrows - EDGE); row++) {

        /* Per-thread local buffers — avoids shared state during computation */
        double *t_window = (double *)G_malloc(SQR(wsize) * sizeof(double));
        double *t_obs = dvector(0, 5);
        int wind_col, found_null;

        for (col = EDGE; col < (ncols - EDGE); col++) {

            /* Read central cell value directly from preloaded map */
            DCELL centre = full_map[(size_t)row * ncols + col];

            /* Propagate null values */
            if (Rast_is_d_null_value(&centre)) {
                if (mparam == FEATURE)
                    Rast_set_c_null_value((CELL *)row_buffers[row] + col, 1);
                else
                    Rast_set_d_null_value((DCELL *)row_buffers[row] + col, 1);
                continue;
            }

            /* Build local window relative to central elevation */
            found_null = 0;
            for (wind_row = 0; wind_row < wsize && !found_null; wind_row++) {
                for (wind_col = 0; wind_col < wsize; wind_col++) {
                    DCELL *cell = full_map +
                        (size_t)(row + wind_row - EDGE) * ncols +
                        col + wind_col - EDGE;
                    if (Rast_is_d_null_value(cell)) {
                        if (mparam == FEATURE)
                            Rast_set_c_null_value((CELL *)row_buffers[row] + col, 1);
                        else
                            Rast_set_d_null_value((DCELL *)row_buffers[row] + col, 1);
                        found_null = 1;
                        break;
                    }
                    t_window[wind_row * wsize + wind_col] = *cell - centre;
                }
            }

            if (found_null)
                continue;

            /* Solve normal equations using LU back substitution */
            find_obs(t_window, t_obs, weight_ptr);

            if (constrained)
                G_lubksb(normal_ptr, 5, index_ptr, t_obs);
            else
                G_lubksb(normal_ptr, 6, index_ptr, t_obs);

            /* Calculate terrain parameter from quadratic coefficients */
            if (mparam == FEATURE)
                *((CELL *)row_buffers[row] + col) = (CELL)feature(t_obs);
            else {
                *((DCELL *)row_buffers[row] + col) = param(mparam, t_obs);
                if (mparam == ELEV)
                    *((DCELL *)row_buffers[row] + col) += centre;
            }
        }

        G_free(t_window);
        free_dvector(t_obs, 0, 5);
    }

    /*-----------------------------------------------------------------------*/
    /* Write output sequentially — Rast_put_row is not thread-safe.         */
    /* Edge rows are written as NULL matching original behavior.             */
    /*-----------------------------------------------------------------------*/
    void *null_buf;
    if (mparam != FEATURE) {
        null_buf = Rast_allocate_buf(DCELL_TYPE);
        Rast_set_d_null_value(null_buf, ncols);
    }
    else {
        null_buf = Rast_allocate_buf(CELL_TYPE);
        Rast_set_c_null_value(null_buf, ncols);
    }

    for (wind_row = 0; wind_row < EDGE; wind_row++)
        Rast_put_row(fd_out, null_buf,
            mparam != FEATURE ? DCELL_TYPE : CELL_TYPE);

    for (row = EDGE; row < (nrows - EDGE); row++) {
        G_percent(row, nrows - EDGE, 2);
        Rast_put_row(fd_out, row_buffers[row],
            mparam != FEATURE ? DCELL_TYPE : CELL_TYPE);
        G_free(row_buffers[row]);
    }

    for (wind_row = 0; wind_row < EDGE; wind_row++)
        Rast_put_row(fd_out, null_buf,
            mparam != FEATURE ? DCELL_TYPE : CELL_TYPE);

    /*-----------------------------------------------------------------------*/
    /* Free all allocated memory                                             */
    /*-----------------------------------------------------------------------*/
    G_free(full_map);
    G_free(weight_ptr);
    G_free(row_buffers);
    G_free(null_buf);
    free_dmatrix(normal_ptr, 0, 5, 0, 5);
    free_ivector(index_ptr, 0, 5);
}