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

/* Rotate the per-thread ring: the oldest slot (0, smallest row index) is     */
/* recycled to become the newest slot (n-1). Mirrors r.neighbors rotate_bufs  */
/* (bufs.c:32-45). Pointer shuffle only, no data copy. */
static void rotate_ring(DCELL **ring, int n)
{
    DCELL *temp = ring[0];
    for (int i = 1; i < n; i++)
        ring[i - 1] = ring[i];
    ring[n - 1] = temp;
}

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
    /* Band height from the memory cap. Reserve wsize input rows per thread  */
    /* (the eventual ring-buffer cost), spend the rest on the band.          */
    /*-----------------------------------------------------------------------*/

    size_t in_buf_size =
        (size_t)ncols * sizeof(DCELL) * (size_t)wsize * (size_t)nprocs;
    size_t out_buf_size = (size_t)memory * (1 << 20);
    if (out_buf_size <= in_buf_size)
        out_buf_size = 0;
    else
        out_buf_size -= in_buf_size;
    int brows = (int)(out_buf_size / (sizeof(DCELL) * (size_t)ncols));
    int proc_rows = nrows - 2 * EDGE; /* processable rows */
    if (brows > proc_rows)
        brows = proc_rows;
    if (brows < nprocs)
        brows = nprocs;
    if (brows < 1)
        brows = 1;

    /*-----------------------------------------------------------------------*/
    /* CHANGE 2: One shared band buffer (brows rows), reused each band, plus */
    /* a single NULL row for the EDGE borders. Threads write disjoint        */
    /* band-relative rows; a serial loop writes each band in row order.      */
    /*-----------------------------------------------------------------------*/

    DCELL *band_d = NULL, *null_d = NULL;
    CELL *band_c = NULL, *null_c = NULL;
    if (mparam != FEATURE) {
        band_d = (DCELL *)G_malloc((size_t)brows * ncols * sizeof(DCELL));
        null_d = Rast_allocate_buf(DCELL_TYPE);
        Rast_set_d_null_value(null_d, ncols);
    }
    else {
        band_c = (CELL *)G_malloc((size_t)brows * ncols * sizeof(CELL));
        null_c = Rast_allocate_buf(CELL_TYPE);
        Rast_set_c_null_value(null_c, ncols);
    }

    /* Top EDGE border rows are written as NULL. top_border is clamped to nrows
     */
    /* so a region with fewer than 2*EDGE+1 rows cannot overflow (see bottom).
     */
    int top_border = (EDGE < nrows) ? EDGE : nrows;
    for (row = 0; row < top_border; row++) {
        if (mparam != FEATURE)
            Rast_put_row(fd_out, null_d, DCELL_TYPE);
        else
            Rast_put_row(fd_out, null_c, CELL_TYPE);
    }

    /*-----------------------------------------------------------------------*/
    /* Per-thread scratch, allocated ONCE before the band loop and indexed by */
    /* tid (like r.neighbors allocate_bufs), instead of churning the allocator
     */
    /* on every band: a wsize-row ring, the window matrix, and the obs vector.
     */
    /*-----------------------------------------------------------------------*/

    DCELL ***rings = (DCELL ***)G_malloc((size_t)nprocs * sizeof(DCELL **));
    DCELL **window_ptrs = (DCELL **)G_malloc((size_t)nprocs * sizeof(DCELL *));
    double **obs_ptrs = (double **)G_malloc((size_t)nprocs * sizeof(double *));
    for (int t = 0; t < nprocs; t++) {
        rings[t] = (DCELL **)G_malloc((size_t)wsize * sizeof(DCELL *));
        for (int k = 0; k < wsize; k++)
            rings[t][k] = (DCELL *)G_malloc((size_t)ncols * sizeof(DCELL));
        window_ptrs[t] = (DCELL *)G_malloc(SQR(wsize) * sizeof(DCELL));
        obs_ptrs[t] = dvector(0, 5);
    }

    /*-----------------------------------------------------------------------*/
    /* CHANGE 3: Two-level structure. Outer serial loop over bands of brows  */
    /* rows; inner parallel region splits the band across threads. Each      */
    /* thread reads its sub-band + halo from disk into a private strip.       */
    /*-----------------------------------------------------------------------*/

    for (int band_start = EDGE; band_start < nrows - EDGE;
         band_start += brows) {
        int band_end = band_start + brows;
        if (band_end > nrows - EDGE)
            band_end = nrows - EDGE; /* exclusive */
        int band_count = band_end - band_start;

#pragma omp parallel if (nprocs > 1)
        {
            int nthreads = 1;
            int tid = 0;
#if defined(_OPENMP)
            nthreads = omp_get_num_threads();
            tid = omp_get_thread_num();
#endif

            /* Split this band's rows [band_start .. band_end) across threads */
            int my_start =
                band_start + (int)((long)band_count * tid / nthreads);
            int my_end =
                band_start + (int)((long)band_count * (tid + 1) / nthreads);

            if (my_end > my_start) {
                /* Per-thread scratch, allocated once before the band loop. */
                DCELL **ring = rings[tid];
                DCELL *window_ptr = window_ptrs[tid];
                double *obs_ptr = obs_ptrs[tid];

                /* Seed the ring with the first wsize-1 window rows; one new */
                /* row is read per output row below (read-once, mirrors */
                /* r.neighbors main.c:522-528). readrow = next disk row. */
                int readrow = my_start - EDGE;
                for (int k = 0; k < wsize - 1; k++) {
                    rotate_ring(ring, wsize);
                    Rast_get_d_row(fd_thread[tid], ring[wsize - 1], readrow++);
                }

                for (int orow = my_start; orow < my_end; orow++) {
                    size_t boff = (size_t)(orow - band_start) * ncols;
                    DCELL *out_d = NULL;
                    CELL *out_c = NULL;
                    if (mparam != FEATURE) {
                        out_d = band_d + boff;
                        Rast_set_d_null_value(out_d, ncols);
                    }
                    else {
                        out_c = band_c + boff;
                        Rast_set_c_null_value(out_c, ncols);
                    }

                    /* Advance the ring by one row: rotate out the oldest, */
                    /* read row orow+EDGE into the recycled newest slot;
                     * ring[k]*/
                    /* then holds map row orow-EDGE+k (same layout as 3.1). */
                    rotate_ring(ring, wsize);
                    Rast_get_d_row(fd_thread[tid], ring[wsize - 1], readrow++);

                    for (int col = EDGE; col < (ncols - EDGE); col++) {
                        DCELL centre = *(ring[EDGE] + col);
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
                            for (int wind_col = 0; wind_col < wsize;
                                 wind_col++) {
                                DCELL *window_cell =
                                    ring[wind_row] + col + wind_col - EDGE;
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
                }
            }
        } /* end parallel */

        /* Serial in-order write of this band's computed rows. */
        if (mparam != FEATURE)
            for (int orow = band_start; orow < band_end; orow++)
                Rast_put_row(fd_out,
                             band_d + (size_t)(orow - band_start) * ncols,
                             DCELL_TYPE);
        else
            for (int orow = band_start; orow < band_end; orow++)
                Rast_put_row(fd_out,
                             band_c + (size_t)(orow - band_start) * ncols,
                             CELL_TYPE);
    } /* end band loop */

    /*-----------------------------------------------------------------------*/
    /* CHANGE 4: Close per-thread descriptors.                               */
    /*-----------------------------------------------------------------------*/

    for (int i = 0; i < nprocs; i++)
        Rast_close(fd_thread[i]);
    G_free(fd_thread);

    /*-----------------------------------------------------------------------*/
    /* CHANGE 5: Bottom EDGE border rows are written as NULL. The computed   */
    /* rows were written per band inside the loop above.                     */
    /*-----------------------------------------------------------------------*/

    /* Bottom border: the remaining rows are NULL. With band_rows = max(0, */
    /* nrows-2*EDGE) computed rows, top_border + band_rows + bottom always sums
     */
    /* to exactly nrows, even when the window is taller than the region. */
    int band_rows = (proc_rows > 0) ? proc_rows : 0;
    for (row = top_border + band_rows; row < nrows; row++) {
        if (mparam != FEATURE)
            Rast_put_row(fd_out, null_d, DCELL_TYPE);
        else
            Rast_put_row(fd_out, null_c, CELL_TYPE);
    }

    if (mparam != FEATURE) {
        G_free(band_d);
        G_free(null_d);
    }
    else {
        G_free(band_c);
        G_free(null_c);
    }
    for (int t = 0; t < nprocs; t++) {
        for (int k = 0; k < wsize; k++)
            G_free(rings[t][k]);
        G_free(rings[t]);
        G_free(window_ptrs[t]);
        free_dvector(obs_ptrs[t], 0, 5);
    }
    G_free(rings);
    G_free(window_ptrs);
    G_free(obs_ptrs);
    G_free(weight_ptr);
    free_dmatrix(normal_ptr, 0, 5, 0, 5);
    free_ivector(index_ptr, 0, 5);
}
