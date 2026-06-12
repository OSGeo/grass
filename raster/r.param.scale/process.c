/*****************************************************************************/
/***                             process()                                 ***/
/***   Parallel (private-strip): full map preloaded into RAM, then each    ***/
/***   thread copies its own strip + halo into a private buffer and works  ***/
/***   only from that private copy.                                        ***/
/***   Based on the serial version by Jo Wood, Project ASSIST, 1993.       ***/
/*****************************************************************************/

#include <stdlib.h>
#include <string.h>
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
    int nrows,               /* Number of rows in the raster.        */
        ncols,               /* Number of columns in the raster.     */
        row;                 /* Counts through rows.                 */

    G_get_window(&region); /* Fill out the region structure.       */

    nrows = Rast_window_rows();
    ncols = Rast_window_cols();

    if ((region.ew_res / region.ns_res >= 1.01) || /* If EW and NS resolns  */
        (region.ns_res / region.ew_res >= 1.01)) { /* >1% different, warn.  */
        G_warning(
            _("E-W and N-S grid resolutions are different. Taking average."));
        resoln = (region.ns_res + region.ew_res) / 2;
    }
    else
        resoln = region.ns_res;

    /*-----------------------------------------------------------------------*/
    /*              CALCULATE LEAST SQUARES COEFFICIENTS (ONCE)              */
    /* Unchanged from serial. Weights and the LU-decomposed normal matrix   */
    /* depend only on window size, not on the data, so they are computed    */
    /* once before the parallel region and only ever READ by threads.       */
    /*-----------------------------------------------------------------------*/

    double *weight_ptr = (double *)G_malloc(SQR(wsize) * sizeof(double));
    double **normal_ptr = dmatrix(0, 5, 0, 5); /* Cross-products matrix.    */
    int *index_ptr = ivector(0, 5);            /* Row permutation vector.   */
    double temp;                               /* Unused (LU sign).         */

    find_weight(weight_ptr); /* Calculate weighting matrix.          */
    find_normal(normal_ptr, weight_ptr); /* Find normal equations.       */

    /* Apply LU decomposition to normal equations. Constrained mode ignores */
    /* coefficient f (last row/col) to force the quadratic through centre.  */
    if (constrained)
        G_ludcmp(normal_ptr, 5, index_ptr, &temp);
    else
        G_ludcmp(normal_ptr, 6, index_ptr, &temp);

    /*-----------------------------------------------------------------------*/
    /* CHANGE 1: Load the ENTIRE input map into one RAM array.              */
    /* Replaces the old row_in sliding buffer + the row-shuffle. With the */
    /* whole map resident, any row is directly addressable, so there is no  */
    /* sequential dependency between rows and threads can work in any order.*/
    /*-----------------------------------------------------------------------*/

    DCELL *full_map = (DCELL *)G_malloc((size_t)nrows * ncols * sizeof(DCELL));
    for (row = 0; row < nrows; row++)
        Rast_get_row(fd_in, full_map + (size_t)row * ncols, row, DCELL_TYPE);

    /*-----------------------------------------------------------------------*/
    /* CHANGE 2: One output slot per row, filled in parallel.              */
    /* Rast_put_row must be called in row order and is not thread-safe, so  */
    /* each thread fills its row's slot here and a serial loop writes them  */
    /* out in order at the end.                                             */
    /*-----------------------------------------------------------------------*/

    DCELL **row_out = (DCELL **)G_malloc((size_t)nrows * sizeof(DCELL *));
    CELL **featrow_out = (CELL **)G_malloc((size_t)nrows * sizeof(CELL *));
    for (row = 0; row < nrows; row++) {
        row_out[row] = NULL;
        featrow_out[row] = NULL;
    }

    /*-----------------------------------------------------------------------*/
    /* CHANGE 3: Parallel region with a PRIVATE STRIP per thread.          */
    /* Each thread owns a contiguous block of output rows. It copies that   */
    /* block PLUS EDGE halo rows on each side out of full_map into its own  */
    /* private `strip' buffer, then does all its window math reading only   */
    /* from `strip' (never from the shared full_map). This is what          */
    /* distinguishes this PR from the shared-array RAM preload approach.    */
    /*-----------------------------------------------------------------------*/

#pragma omp parallel if (nprocs > 1)
    {
        int nthreads = 1;
        int tid = 0;
#if defined(_OPENMP)
        nthreads = omp_get_num_threads();
        tid = omp_get_thread_num();
#endif

        /* Processable output rows are EDGE .. nrows-EDGE-1. Split that      */
        /* range into nthreads contiguous strips; this thread owns          */
        /* [my_start, my_end).                                              */
        int proc_first = EDGE;
        int proc_last = nrows - EDGE; /* exclusive */
        int proc_count = proc_last - proc_first;

        int my_start = proc_first + (int)((long)proc_count * tid / nthreads);
        int my_end =
            proc_first + (int)((long)proc_count * (tid + 1) / nthreads);

        if (my_end > my_start) {
            /* Input rows needed: my_start-EDGE .. my_end-1+EDGE. The halo   */
            /* (EDGE rows each side) is why neighbouring windows never need  */
            /* to look outside this private buffer.                          */
            int buf_first = my_start - EDGE;
            int buf_last = my_end - 1 + EDGE; /* inclusive */
            int buf_rows = buf_last - buf_first + 1;

            /* PRIVATE BUFFER: copy this thread's strip+halo out of          */
            /* full_map once. After this, the thread reads ONLY from strip.  */
            DCELL *strip =
                (DCELL *)G_malloc((size_t)buf_rows * ncols * sizeof(DCELL));
            for (int r = 0; r < buf_rows; r++)
                memcpy(strip + (size_t)r * ncols,
                       full_map + (size_t)(buf_first + r) * ncols,
                       (size_t)ncols * sizeof(DCELL));

            /* Per-thread math scratch (one private copy each, never shared)*/
            DCELL *window_ptr = (DCELL *)G_malloc(SQR(wsize) * sizeof(DCELL));
            double *obs_ptr = dvector(0, 5);

            for (int orow = my_start; orow < my_end; orow++) {
                /* Allocate this row's output buffer. */
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

                /* Index of the centre row within the private strip.         */
                int srow = orow - buf_first;

                for (int col = EDGE; col < (ncols - EDGE); col++) {
                    /* Find central z value (from the private strip).        */
                    DCELL centre = *(strip + (size_t)srow * ncols + col);
                    /* Test for no data and propagate. */
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
                            /* Express all window values relative to the     */
                            /* central elevation. CHANGED: the window cell   */
                            /* is now addressed inside the private strip;    */
                            /* (srow - EDGE) is >= 0 because the strip        */
                            /* includes the EDGE halo rows.                  */
                            DCELL *window_cell =
                                strip +
                                (size_t)(srow - EDGE + wind_row) * ncols + col +
                                wind_col - EDGE;
                            /* Test for no data and propagate. */
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
                    /* Use LU back substitution to solve normal equations.   */
                    /* (Math block unchanged from serial.)                   */
                    find_obs(window_ptr, obs_ptr, weight_ptr);
                    if (constrained)
                        G_lubksb(normal_ptr, 5, index_ptr, obs_ptr);
                    else
                        G_lubksb(normal_ptr, 6, index_ptr, obs_ptr);

                    /* Calculate terrain parameter from quad. coefficients.  */
                    if (mparam == FEATURE)
                        *(out_c + col) = (CELL)feature(obs_ptr);
                    else
                        *(out_d + col) = param(mparam, obs_ptr);

                    if (mparam == ELEV)
                        *(out_d + col) += centre; /* Add central elev back. */
                }

                /* Store this row's finished buffer in its slot. */
                if (mparam != FEATURE)
                    row_out[orow] = out_d;
                else
                    featrow_out[orow] = out_c;
            }

            /* Free this thread's private buffers. */
            G_free(strip);
            G_free(window_ptr);
            free_dvector(obs_ptr, 0, 5);
        }
    } /* end parallel */

    /*-----------------------------------------------------------------------*/
    /* CHANGE 4: Serial, in-order write to disk.                           */
    /* The original 'shuffle rows down' loop is GONE. Edge rows (top and    */
    /* bottom EDGE rows) are written as NULL, exactly as the serial version */
    /* did with its leading/trailing edge writes.                           */
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
            /* Edge cells written as NULL. */
            if (mparam != FEATURE)
                Rast_put_row(fd_out, null_d, DCELL_TYPE);
            else
                Rast_put_row(fd_out, null_c, CELL_TYPE);
        }
        else {
            /* Write the buffered row, then free its slot. */
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

    /*-----------------------------------------------------------------------*/
    /*     FREE MEMORY USED FOR MAP, OUTPUT SLOTS, WINDOW AND MATRICES       */
    /*-----------------------------------------------------------------------*/

    if (mparam != FEATURE)
        G_free(null_d);
    else
        G_free(null_c);
    G_free(row_out);
    G_free(featrow_out);
    G_free(full_map);
    G_free(weight_ptr);
    free_dmatrix(normal_ptr, 0, 5, 0, 5);
    free_ivector(index_ptr, 0, 5);
}
