/*
 * interp_strip.c - strip versions of the resampling methods. They read the
 *   input rows from a strip held in memory as floats, instead of the block
 *   cache. Nearest is handled by strip_nearest() in main.c.
 */

#include <math.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/raster.h>
#include "r.proj.h"

/* Reads one value from the strip. A row outside the loaded range records the
 * needed row through need_lo/need_hi and returns a null, so the caller nulls
 * the cell and the band reloads and recomputes. */
static inline FCELL strip_val(const struct strip *s, int r, int c, int *need_lo,
                              int *need_hi)
{
    if (r < s->imin || r > s->imax) {
        FCELL null_val;

        if (r < s->imin && r < *need_lo)
            *need_lo = r;
        if (r > s->imax && r > *need_hi)
            *need_hi = r;
        Rast_set_f_null_value(&null_val, 1);
        return null_val;
    }
    return ((const FCELL *)s->data)[(size_t)(r - s->imin) * s->cols + c];
}

void strip_bilinear(const struct strip *s, void *obufptr, int cell_type,
                    double col_idx, double row_idx, struct Cell_head *incellhd,
                    int *need_lo, int *need_hi)
{
    int row, col, i, j;
    FCELL t, u, result;
    FCELL c[2][2];

    row = (int)floor(row_idx - 0.5);
    col = (int)floor(col_idx - 0.5);

    /* Full-map bounds check runs before any strip read. A sample outside the
     * input map is set to NULL and returned, so strip_val is never asked for a
     * row outside the map. */
    if (row < 0 || row + 1 >= incellhd->rows || col < 0 ||
        col + 1 >= incellhd->cols) {
        Rast_set_null_value(obufptr, 1, cell_type);
        return;
    }

    for (i = 0; i < 2; i++)
        for (j = 0; j < 2; j++) {
            const FCELL cell = strip_val(s, row + i, col + j, need_lo, need_hi);

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

void strip_cubic(const struct strip *s, void *obufptr, int cell_type,
                 double col_idx, double row_idx, struct Cell_head *incellhd,
                 int *need_lo, int *need_hi)
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
            const FCELL cell =
                strip_val(s, row - 1 + i, col - 1 + j, need_lo, need_hi);

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

void strip_lanczos(const struct strip *s, void *obufptr, int cell_type,
                   double col_idx, double row_idx, struct Cell_head *incellhd,
                   int *need_lo, int *need_hi)
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
            const FCELL cell =
                strip_val(s, row - 2 + i, col - 2 + j, need_lo, need_hi);

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

void strip_bilinear_f(const struct strip *s, void *obufptr, int cell_type,
                      double col_idx, double row_idx,
                      struct Cell_head *incellhd, int *need_lo, int *need_hi)
{
    int row, col;
    FCELL cell;

    row = (int)floor(row_idx);
    col = (int)floor(col_idx);

    if (row < 0 || row >= incellhd->rows || col < 0 || col >= incellhd->cols) {
        Rast_set_null_value(obufptr, 1, cell_type);
        return;
    }

    cell = strip_val(s, row, col, need_lo, need_hi);
    /* if nearest is null, all the other interps will be null */
    if (Rast_is_f_null_value(&cell)) {
        Rast_set_null_value(obufptr, 1, cell_type);
        return;
    }

    strip_bilinear(s, obufptr, cell_type, col_idx, row_idx, incellhd, need_lo,
                   need_hi);
    /* fallback to nearest if bilinear is null */
    if (Rast_is_f_null_value(obufptr))
        Rast_set_f_value(obufptr, cell, cell_type);
}

void strip_cubic_f(const struct strip *s, void *obufptr, int cell_type,
                   double col_idx, double row_idx, struct Cell_head *incellhd,
                   int *need_lo, int *need_hi)
{
    int row, col;
    FCELL cell;

    row = (int)floor(row_idx);
    col = (int)floor(col_idx);

    if (row < 0 || row >= incellhd->rows || col < 0 || col >= incellhd->cols) {
        Rast_set_null_value(obufptr, 1, cell_type);
        return;
    }

    cell = strip_val(s, row, col, need_lo, need_hi);
    /* if nearest is null, all the other interps will be null */
    if (Rast_is_f_null_value(&cell)) {
        Rast_set_null_value(obufptr, 1, cell_type);
        return;
    }

    strip_cubic(s, obufptr, cell_type, col_idx, row_idx, incellhd, need_lo,
                need_hi);
    /* fallback to bilinear if cubic is null */
    if (Rast_is_f_null_value(obufptr)) {
        strip_bilinear(s, obufptr, cell_type, col_idx, row_idx, incellhd,
                       need_lo, need_hi);
        /* fallback to nearest if bilinear is null */
        if (Rast_is_f_null_value(obufptr))
            Rast_set_f_value(obufptr, cell, cell_type);
    }
}

void strip_lanczos_f(const struct strip *s, void *obufptr, int cell_type,
                     double col_idx, double row_idx, struct Cell_head *incellhd,
                     int *need_lo, int *need_hi)
{
    int row, col;
    FCELL cell;

    row = (int)floor(row_idx);
    col = (int)floor(col_idx);

    if (row < 0 || row >= incellhd->rows || col < 0 || col >= incellhd->cols) {
        Rast_set_null_value(obufptr, 1, cell_type);
        return;
    }

    cell = strip_val(s, row, col, need_lo, need_hi);
    /* if nearest is null, all the other interps will be null */
    if (Rast_is_f_null_value(&cell)) {
        Rast_set_null_value(obufptr, 1, cell_type);
        return;
    }

    strip_lanczos(s, obufptr, cell_type, col_idx, row_idx, incellhd, need_lo,
                  need_hi);
    /* fallback to bicubic if lanczos is null */
    if (Rast_is_f_null_value(obufptr)) {
        strip_cubic(s, obufptr, cell_type, col_idx, row_idx, incellhd, need_lo,
                    need_hi);
        /* fallback to bilinear if cubic is null */
        if (Rast_is_f_null_value(obufptr)) {
            strip_bilinear(s, obufptr, cell_type, col_idx, row_idx, incellhd,
                           need_lo, need_hi);
            /* fallback to nearest if bilinear is null */
            if (Rast_is_f_null_value(obufptr))
                Rast_set_f_value(obufptr, cell, cell_type);
        }
    }
}
