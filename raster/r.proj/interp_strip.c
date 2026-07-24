/*
 * interp_strip.c - strip-based interpolation kernels for the banded r.proj
 *   compute path. These mirror the cache-based kernels (bilinear.c, cubic.c,
 *   lanczos.c and their _f variants) but read an in-RAM FCELL band strip
 *   holding input rows [imin, imax] instead of the readcell block cache.
 *   Nearest is handled by interpolate_strip() in main.c and is not duplicated.
 */

#include <math.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/raster.h>
#include "r.proj.h"

/* Read one FCELL from the band strip. The strip holds full-width input rows
 * [imin, imax] contiguously; input row r maps to strip row (r - imin), the same
 * addressing as interpolate_strip(). Every read is guarded by the same
 * under-size tripwire as interpolate_strip: a stencil row inside the input map
 * but outside the loaded strip means a sizing/indexing bug, so fail loudly
 * rather than read out of bounds. Each kernel runs its full-map bounds check
 * first (setting NULL for out-of-map stencils), so this tripwire only ever
 * fires on a bug. */
static inline FCELL strip_val(const void *strip, int r, int c, int imin,
                              int imax, int cols)
{
    if (r < imin || r > imax)
        G_fatal_error(_("Band strip under-sized: input row %d outside loaded "
                        "range [%d, %d] at column %d"),
                      r, imin, imax, c);
    return ((const FCELL *)strip)[(size_t)(r - imin) * cols + c];
}

void strip_bilinear(void *strip, void *obufptr, int cell_type, double col_idx,
                    double row_idx, struct Cell_head *incellhd, int imin,
                    int imax)
{
    int row, col, i, j;
    FCELL t, u, result;
    FCELL c[2][2];

    row = (int)floor(row_idx - 0.5);
    col = (int)floor(col_idx - 0.5);

    /* Full-map bounds check runs before any strip read: an out-of-map stencil
     * sets NULL and returns, so strip_val is never reached out of range. */
    if (row < 0 || row + 1 >= incellhd->rows || col < 0 ||
        col + 1 >= incellhd->cols) {
        Rast_set_null_value(obufptr, 1, cell_type);
        return;
    }

    for (i = 0; i < 2; i++)
        for (j = 0; j < 2; j++) {
            const FCELL cell =
                strip_val(strip, row + i, col + j, imin, imax, incellhd->cols);

            if (Rast_is_f_null_value(&cell)) {
                Rast_set_null_value(obufptr, 1, cell_type);
                return;
            }
            c[i][j] = cell;
        }

    t = col_idx - 0.5 - col;
    u = row_idx - 0.5 - row;

    result = Rast_interp_bilinear(t, u, c[0][0], c[0][1], c[1][0], c[1][1]);

    Rast_set_f_value(obufptr, result, cell_type);
}

void strip_cubic(void *strip, void *obufptr, int cell_type, double col_idx,
                 double row_idx, struct Cell_head *incellhd, int imin, int imax)
{
    int row, col, i, j;
    FCELL t, u, result;
    FCELL val[4];
    FCELL c[4][4];

    row = (int)floor(row_idx - 0.5);
    col = (int)floor(col_idx - 0.5);

    /* Full-map bounds check runs before any strip read. */
    if (row - 1 < 0 || row + 2 >= incellhd->rows || col - 1 < 0 ||
        col + 2 >= incellhd->cols) {
        Rast_set_null_value(obufptr, 1, cell_type);
        return;
    }

    for (i = 0; i < 4; i++)
        for (j = 0; j < 4; j++) {
            const FCELL cell = strip_val(strip, row - 1 + i, col - 1 + j, imin,
                                         imax, incellhd->cols);

            if (Rast_is_f_null_value(&cell)) {
                Rast_set_null_value(obufptr, 1, cell_type);
                return;
            }
            c[i][j] = cell;
        }

    t = col_idx - 0.5 - col;
    u = row_idx - 0.5 - row;

    for (i = 0; i < 4; i++) {
        const FCELL *tmp = c[i];

        val[i] = Rast_interp_cubic(t, tmp[0], tmp[1], tmp[2], tmp[3]);
    }

    result = Rast_interp_cubic(u, val[0], val[1], val[2], val[3]);

    Rast_set_f_value(obufptr, result, cell_type);
}

void strip_lanczos(void *strip, void *obufptr, int cell_type, double col_idx,
                   double row_idx, struct Cell_head *incellhd, int imin,
                   int imax)
{
    int row, col, i, j, k;
    double t, u;
    FCELL result;
    DCELL c[25];

    row = (int)floor(row_idx);
    col = (int)floor(col_idx);

    /* Full-map bounds check runs before any strip read. */
    if (row - 2 < 0 || row + 2 >= incellhd->rows || col - 2 < 0 ||
        col + 2 >= incellhd->cols) {
        Rast_set_null_value(obufptr, 1, cell_type);
        return;
    }

    k = 0;
    for (i = 0; i < 5; i++) {
        for (j = 0; j < 5; j++) {
            const FCELL cell = strip_val(strip, row - 2 + i, col - 2 + j, imin,
                                         imax, incellhd->cols);

            if (Rast_is_f_null_value(&cell)) {
                Rast_set_null_value(obufptr, 1, cell_type);
                return;
            }
            c[k++] = cell;
        }
    }

    t = col_idx - 0.5 - col;
    u = row_idx - 0.5 - row;

    result = Rast_interp_lanczos(t, u, c);

    Rast_set_f_value(obufptr, result, cell_type);
}

void strip_bilinear_f(void *strip, void *obufptr, int cell_type, double col_idx,
                      double row_idx, struct Cell_head *incellhd, int imin,
                      int imax)
{
    int row, col;
    FCELL cell;

    row = (int)floor(row_idx);
    col = (int)floor(col_idx);

    if (row < 0 || row >= incellhd->rows || col < 0 || col >= incellhd->cols) {
        Rast_set_null_value(obufptr, 1, cell_type);
        return;
    }

    cell = strip_val(strip, row, col, imin, imax, incellhd->cols);
    /* if nearest is null, all the other interps will be null */
    if (Rast_is_f_null_value(&cell)) {
        Rast_set_null_value(obufptr, 1, cell_type);
        return;
    }

    strip_bilinear(strip, obufptr, cell_type, col_idx, row_idx, incellhd, imin,
                   imax);
    /* fallback to nearest if bilinear is null */
    if (Rast_is_f_null_value(obufptr))
        Rast_set_f_value(obufptr, cell, cell_type);
}

void strip_cubic_f(void *strip, void *obufptr, int cell_type, double col_idx,
                   double row_idx, struct Cell_head *incellhd, int imin,
                   int imax)
{
    int row, col;
    FCELL cell;

    row = (int)floor(row_idx);
    col = (int)floor(col_idx);

    if (row < 0 || row >= incellhd->rows || col < 0 || col >= incellhd->cols) {
        Rast_set_null_value(obufptr, 1, cell_type);
        return;
    }

    cell = strip_val(strip, row, col, imin, imax, incellhd->cols);
    /* if nearest is null, all the other interps will be null */
    if (Rast_is_f_null_value(&cell)) {
        Rast_set_null_value(obufptr, 1, cell_type);
        return;
    }

    strip_cubic(strip, obufptr, cell_type, col_idx, row_idx, incellhd, imin,
                imax);
    /* fallback to bilinear if cubic is null */
    if (Rast_is_f_null_value(obufptr)) {
        strip_bilinear(strip, obufptr, cell_type, col_idx, row_idx, incellhd,
                       imin, imax);
        /* fallback to nearest if bilinear is null */
        if (Rast_is_f_null_value(obufptr))
            Rast_set_f_value(obufptr, cell, cell_type);
    }
}

void strip_lanczos_f(void *strip, void *obufptr, int cell_type, double col_idx,
                     double row_idx, struct Cell_head *incellhd, int imin,
                     int imax)
{
    int row, col;
    FCELL cell;

    row = (int)floor(row_idx);
    col = (int)floor(col_idx);

    if (row < 0 || row >= incellhd->rows || col < 0 || col >= incellhd->cols) {
        Rast_set_null_value(obufptr, 1, cell_type);
        return;
    }

    cell = strip_val(strip, row, col, imin, imax, incellhd->cols);
    /* if nearest is null, all the other interps will be null */
    if (Rast_is_f_null_value(&cell)) {
        Rast_set_null_value(obufptr, 1, cell_type);
        return;
    }

    strip_lanczos(strip, obufptr, cell_type, col_idx, row_idx, incellhd, imin,
                  imax);
    /* fallback to bicubic if lanczos is null */
    if (Rast_is_f_null_value(obufptr)) {
        strip_cubic(strip, obufptr, cell_type, col_idx, row_idx, incellhd, imin,
                    imax);
        /* fallback to bilinear if cubic is null */
        if (Rast_is_f_null_value(obufptr)) {
            strip_bilinear(strip, obufptr, cell_type, col_idx, row_idx,
                           incellhd, imin, imax);
            /* fallback to nearest if bilinear is null */
            if (Rast_is_f_null_value(obufptr))
                Rast_set_f_value(obufptr, cell, cell_type);
        }
    }
}
