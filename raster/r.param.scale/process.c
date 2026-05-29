/*****************************************************************************/
/***                             process()                                 ***/
/***   Parallel (per-thread file descriptors): each thread opens its own  ***/
/***   file descriptor and reads its strip + halo directly from disk      ***/
/***   into a private buffer. No full map in RAM. Memory-light path for  ***/
/***   maps that do not fit in RAM.                                      ***/
/***   Based on the serial version by Jo Wood, Project ASSIST, 1993.      ***/
/*****************************************************************************/

#include <stdlib.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include <grass/gmath.h>
#include "param.h"
#include "nrutil.h"

#if defined(_OPENMP)
#include <omp.h>
#endif

void process(void)
{
    struct Cell_head region; /* Structure to hold region information */
    int nrows, ncols, row;

    G_get_window(&region);
    nrows = Rast_window_rows();
    ncols = Rast_window_cols();

    if ((region.ew_res / region.ns_res >= 1.01) ||
        (region.ns_res / region.ew_res >= 1.01)) {
        G_warning(
            _("E-W and N-S grid resolutions are different. Taking average."));
        resoln = (region.ns_res + region.ew_res) / 2;
    }
    else
        resoln = region.ns_res;

    /*-----------------------------------------------------------------------*/
    /*              CALCULATE LEAST SQUARES COEFFICIENTS (ONCE)              */
    /* Same as serial. Weights, normal matrix, and LU decomposition depend  */
    /* only on window size, not on data. Computed once, read-only by all    */
    /* threads.                                                              */
    /*-----------------------------------------------------------------------*/

    double *weight_ptr = (double *)G_malloc(SQR(wsize) * sizeof(double));
    double **normal_ptr = dmatrix(0, 5, 0, 5);
    int *index_ptr = ivector(0, 5);
    double temp;

    find_weight(weight_ptr);
    find_normal(normal_ptr, weight_ptr);

    if (constrained)
        G_ludcmp(normal_ptr, 5, index_ptr, &temp);
    else
        G_ludcmp(normal_ptr, 6, index_ptr, &temp);

    /*-----------------------------------------------------------------------*/
    /* CHANGE 1: Per-thread input file descriptors.                          */
    /* Each thread opens its own descriptor to the input raster. Each thread */
    /*  will read its strip+halo directly from disk into its private buffer.  */
    /* This is the r.neighbors pattern. No full_map array exists.            */
    /*-----------------------------------------------------------------------*/

    int *fd_thread = (int *)G_malloc((size_t)nprocs * sizeof(int));
    for (int i = 0; i < nprocs; i++)
        fd_thread[i] = Rast_open_old(rast_in_name, "");

    /*-----------------------------------------------------------------------*/
    /* CHANGE 2: Output slots, one pointer per row.                          */
    /* Threads fill their rows into slots, a serial                          */
    /* loop writes them in order at the end (Rast_put_row is not            */
    /* thread-safe and must be called in row order).                         */
    /*-----------------------------------------------------------------------*/

    DCELL **row_out = (DCELL **)G_malloc((size_t)nrows * sizeof(DCELL *));
    CELL **featrow_out = (CELL **)G_malloc((size_t)nrows * sizeof(CELL *));
    for (row = 0; row < nrows; row++) {
        row_out[row] = NULL;
        featrow_out[row] = NULL;
    }

    /*-----------------------------------------------------------------------*/
    /* CHANGE 3: Parallel region.                                            */
    /* Each thread claims its output row range, then reads its strip + halo */
    /* from disk via its own descriptor into a private buffer, and does     */
    /* all window math reading from that private buffer.                     */
    /*-----------------------------------------------------------------------*/

#pragma omp parallel if (nprocs > 1)
    {
        int nthreads = 1;
        int tid = 0;
#if defined(_OPENMP)
        nthreads = omp_get_num_threads();
        tid = omp_get_thread_num();
#endif

        /* Split processable rows [EDGE .. nrows-EDGE) evenly across threads */
        int proc_first = EDGE;
        int proc_last = nrows - EDGE; /* exclusive */
        int proc_count = proc_last - proc_first;

        int my_start = proc_first + (int)((long)proc_count * tid / nthreads);
        int my_end =
            proc_first + (int)((long)proc_count * (tid + 1) / nthreads);

        if (my_end > my_start) {
            /* Input rows this thread needs: my_start-EDGE .. my_end-1+EDGE */
            int buf_first = my_start - EDGE;
            int buf_last = my_end - 1 + EDGE;
            int buf_rows = buf_last - buf_first + 1;

            /* Allocate private strip and read it from disk via THIS         */
            /* thread's own descriptor. No shared RAM map; each thread       */
            /* pulls only what it needs.                                     */
            DCELL *strip =
                (DCELL *)G_malloc((size_t)buf_rows * ncols * sizeof(DCELL));
            for (int r = 0; r < buf_rows; r++)
                Rast_get_d_row(fd_thread[tid], strip + (size_t)r * ncols,
                               buf_first + r);

            /* Per-thread math scratch */
            DCELL *window_ptr = (DCELL *)G_malloc(SQR(wsize) * sizeof(DCELL));
            double *obs_ptr = dvector(0, 5);

            for (int orow = my_start; orow < my_end; orow++) {
                DCELL *out_d = NULL;
                CELL *out_c = NULL;
                if (mparam != FEATURE) {
                    out_d = Rast_allocate_buf(DCELL_TYPE);
                    Rast_set_d_null_value(out_d, ncols);
                }
                else {
                    out_c = Rast_allocate_buf(CELL_TYPE);
                    Rast_set_c_null_value(out_c, ncols);
                }

                /* Position of this row inside the private strip */
                int srow = orow - buf_first;

                for (int col = EDGE; col < (ncols - EDGE); col++) {
                    DCELL centre = *(strip + (size_t)srow * ncols + col);
                    if (Rast_is_d_null_value(&centre)) {
                        if (mparam == FEATURE)
                            Rast_set_c_null_value(out_c + col, 1);
                        else
                            Rast_set_d_null_value(out_d + col, 1);
                        continue;
                    }

                    int found_null = FALSE;
                    for (int wind_row = 0; wind_row < wsize; wind_row++) {
                        if (found_null)
                            break;
                        for (int wind_col = 0; wind_col < wsize; wind_col++) {
                            DCELL *window_cell =
                                strip +
                                (size_t)(srow - EDGE + wind_row) * ncols + col +
                                wind_col - EDGE;
                            if (Rast_is_d_null_value(window_cell)) {
                                if (mparam == FEATURE)
                                    Rast_set_c_null_value(out_c + col, 1);
                                else
                                    Rast_set_d_null_value(out_d + col, 1);
                                found_null = TRUE;
                                break;
                            }
                            *(window_ptr + (wind_row * wsize) + wind_col) =
                                *(window_cell)-centre;
                        }
                    }
                    if (found_null)
                        continue;

                    /* Math block: unchanged from serial */
                    find_obs(window_ptr, obs_ptr, weight_ptr);
                    if (constrained)
                        G_lubksb(normal_ptr, 5, index_ptr, obs_ptr);
                    else
                        G_lubksb(normal_ptr, 6, index_ptr, obs_ptr);

                    if (mparam == FEATURE)
                        *(out_c + col) = (CELL)feature(obs_ptr);
                    else
                        *(out_d + col) = param(mparam, obs_ptr);

                    if (mparam == ELEV)
                        *(out_d + col) += centre;
                }

                if (mparam != FEATURE)
                    row_out[orow] = out_d;
                else
                    featrow_out[orow] = out_c;
            }

            G_free(strip);
            G_free(window_ptr);
            free_dvector(obs_ptr, 0, 5);
        }
    } /* end parallel */

    /*-----------------------------------------------------------------------*/
    /* CHANGE 4: Close per-thread descriptors.                               */
    /*-----------------------------------------------------------------------*/

    for (int i = 0; i < nprocs; i++)
        Rast_close(fd_thread[i]);
    G_free(fd_thread);

    /*-----------------------------------------------------------------------*/
    /* CHANGE 5: Serial in-order write to disk.                              */
    /* Top and bottom EDGE rows are written as NULL, then the computed rows */
    /* are written from row_out[]/featrow_out[] in order.                    */
    /*-----------------------------------------------------------------------*/

    DCELL *null_d = NULL;
    CELL *null_c = NULL;
    if (mparam != FEATURE) {
        null_d = Rast_allocate_buf(DCELL_TYPE);
        Rast_set_d_null_value(null_d, ncols);
    }
    else {
        null_c = Rast_allocate_buf(CELL_TYPE);
        Rast_set_c_null_value(null_c, ncols);
    }

    for (row = 0; row < nrows; row++) {
        if (row < EDGE || row >= nrows - EDGE) {
            if (mparam != FEATURE)
                Rast_put_row(fd_out, null_d, DCELL_TYPE);
            else
                Rast_put_row(fd_out, null_c, CELL_TYPE);
        }
        else {
            if (mparam != FEATURE) {
                Rast_put_row(fd_out, row_out[row], DCELL_TYPE);
                G_free(row_out[row]);
            }
            else {
                Rast_put_row(fd_out, featrow_out[row], CELL_TYPE);
                G_free(featrow_out[row]);
            }
        }
    }

    if (mparam != FEATURE)
        G_free(null_d);
    else
        G_free(null_c);
    G_free(row_out);
    G_free(featrow_out);
    G_free(weight_ptr);
    free_dmatrix(normal_ptr, 0, 5, 0, 5);
    free_ivector(index_ptr, 0, 5);
}
