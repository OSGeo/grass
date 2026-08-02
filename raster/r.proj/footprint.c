/*
 * footprint.c - grid of input row spans for the output map.
 *
 * Each cell covers one output row and one column block and holds the range of
 * input rows that block reaches.
 */

#include <float.h>
#include <math.h>

#include <grass/gis.h>
#include <grass/glocale.h>

#include "r.proj.h"

struct fg_cell {
    double rmin, rmax; /* rmax below rmin marks an empty cell */
};

struct footprint_grid {
    int grows, nb;        /* grid rows and column blocks */
    int ocols;            /* output columns */
    int irows;            /* input rows */
    struct fg_cell *cell; /* grows by nb cells in row major order */
};

/* The samples can miss a curve between columns by a fraction of a row, so each
 * cell is widened by one row. */
#define FG_SAMPLING_MARGIN 1.0

/* Returns the first output column of block b. */
static int block_c0(const struct footprint_grid *g, int b)
{
    return (int)((long)b * g->ocols / g->nb);
}

/* Returns the block that contains output column c. */
static int block_of_col(const struct footprint_grid *g, int c)
{
    int b;

    for (b = 0; b < g->nb - 1; b++)
        if (c < block_c0(g, b + 1))
            return b;
    return g->nb - 1;
}

/* Projects the center of output cell (r, c) to an input row index. Returns 0 on
 * a failed transform and leaves ri unchanged. */
static int sample_ri(const struct Cell_head *ohd, const struct Cell_head *ihd,
                     const struct pj_info *oproj, const struct pj_info *iproj,
                     const struct pj_info *tproj, const double *y_center, int r,
                     int c, double *ri)
{
    double xx = ohd->west + (c + 0.5) * ohd->ew_res;
    double yy = y_center[r];

    if (GPJ_transform(oproj, iproj, tproj, PJ_FWD, &xx, &yy, NULL) < 0)
        return 0;
    *ri = (ihd->north - yy) / ihd->ns_res;
    return 1;
}

/* Widens cell (r, b) to include any pole whose output point falls inside the
 * cell rectangle. */
static void fold_poles(const struct footprint_grid *g,
                       const struct Cell_head *ohd,
                       const struct pole_set *poles, int r, int b,
                       struct fg_cell *cell)
{
    int c0 = block_c0(g, b), c1 = block_c0(g, b + 1), k;
    double x_lo = ohd->west + c0 * ohd->ew_res;
    double x_hi = ohd->west + c1 * ohd->ew_res;
    double y_lo = ohd->north - (r + 1) * ohd->ns_res;
    double y_hi = ohd->north - r * ohd->ns_res;

    if (!poles)
        return;
    for (k = 0; k < poles->n; k++) {
        if (poles->ox[k] < x_lo || poles->ox[k] > x_hi || poles->oy[k] < y_lo ||
            poles->oy[k] > y_hi)
            continue;
        if (poles->ri[k] < cell->rmin)
            cell->rmin = poles->ri[k];
        if (poles->ri[k] > cell->rmax)
            cell->rmax = poles->ri[k];
    }
}

/* Builds the grid from block boundary samples. */
struct footprint_grid *
fg_build(const struct Cell_head *ohd, const struct Cell_head *ihd,
         const struct pj_info *oproj, const struct pj_info *iproj,
         const struct pj_info *tproj, const double *y_center,
         const struct pole_set *poles)
{
    struct footprint_grid *g = G_malloc(sizeof(*g));
    int r, b;
    double *bnd;

    g->grows = ohd->rows;
    g->nb = ohd->cols < 32 ? ohd->cols : 32;
    g->ocols = ohd->cols;
    g->irows = ihd->rows;
    g->cell = G_malloc((size_t)g->grows * g->nb * sizeof(struct fg_cell));
    bnd = G_malloc((size_t)(g->nb + 1) * sizeof(double));

    for (r = 0; r < g->grows; r++) {
        /* Sample the NB plus one block boundaries for this row. The last
         * boundary uses the final valid column. */
        int k;

        for (k = 0; k <= g->nb; k++) {
            int c = block_c0(g, k);

            if (c > g->ocols - 1)
                c = g->ocols - 1;
            if (!sample_ri(ohd, ihd, oproj, iproj, tproj, y_center, r, c,
                           &bnd[k]))
                bnd[k] = DBL_MAX; /* a failed sample is left out of the range */
        }
        for (b = 0; b < g->nb; b++) {
            struct fg_cell *cell = &g->cell[(size_t)r * g->nb + b];
            double lo = bnd[b] < bnd[b + 1] ? bnd[b] : bnd[b + 1];
            double hi = bnd[b] > bnd[b + 1] ? bnd[b] : bnd[b + 1];

            cell->rmin = DBL_MAX;
            cell->rmax = -DBL_MAX;
            if (bnd[b] != DBL_MAX && bnd[b + 1] != DBL_MAX) {
                cell->rmin = lo;
                cell->rmax = hi;
            }
            else if (bnd[b] != DBL_MAX) {
                cell->rmin = cell->rmax = bnd[b];
            }
            else if (bnd[b + 1] != DBL_MAX) {
                cell->rmin = cell->rmax = bnd[b + 1];
            }
            fold_poles(g, ohd, poles, r, b, cell);
        }
    }
    G_free(bnd);
    return g;
}

/* Returns the input row span covering the output rectangle. Includes every
 * block the rectangle touches and adds a two cell margin. The grid holds one
 * row per output row, so every output row in the rectangle indexes a grid row.
 */
void fg_span(const struct footprint_grid *g, int obr0, int obr1, int obc0,
             int obc1, int *imin, int *imax)
{
    double rmin = DBL_MAX, rmax = -DBL_MAX;
    int b_lo = block_of_col(g, obc0), b_hi = block_of_col(g, obc1 - 1);
    int r, b;

    if (obr1 > g->grows)
        G_fatal_error(_("Footprint grid has %d rows but output row %d was "
                        "requested"),
                      g->grows, obr1 - 1);

    for (r = obr0; r < obr1; r++)
        for (b = b_lo; b <= b_hi; b++) {
            const struct fg_cell *cell = &g->cell[(size_t)r * g->nb + b];

            if (cell->rmax < cell->rmin)
                continue; /* empty cell */
            if (cell->rmin < rmin)
                rmin = cell->rmin;
            if (cell->rmax > rmax)
                rmax = cell->rmax;
        }

    if (rmax < rmin) { /* every touched cell empty */
        *imin = 0;
        *imax = -1;
        return;
    }
    int lo = (int)floor(rmin) - 2;
    int hi = (int)floor(rmax) + 2;

    if (lo < 0)
        lo = 0;
    if (hi > g->irows - 1)
        hi = g->irows - 1;
    *imin = lo;
    *imax = hi;
}

/* Find the tallest band at obr0 whose strip and output still fit the cap, and
 * never return less than one row. */
int fg_band_height(const struct footprint_grid *g, int obr0, size_t cap_bytes,
                   int out_mult, int cell_size, int in_cols)
{
    double rmin = DBL_MAX, rmax = -DBL_MAX;
    int max_h = g->grows - obr0, accepted = 1, h, b;

    for (h = 0; h < max_h; h++) {
        int r = obr0 + h, strip_rows;
        size_t strip_bytes, out_bytes;

        for (b = 0; b < g->nb; b++) {
            const struct fg_cell *cell = &g->cell[(size_t)r * g->nb + b];

            if (cell->rmax < cell->rmin)
                continue;
            if (cell->rmin < rmin)
                rmin = cell->rmin;
            if (cell->rmax > rmax)
                rmax = cell->rmax;
        }
        if (rmax < rmin) {
            strip_rows = 0;
        }
        else {
            int lo = (int)floor(rmin) - 2;
            int hi = (int)floor(rmax) + 2;

            if (lo < 0)
                lo = 0;
            if (hi > g->irows - 1)
                hi = g->irows - 1;
            strip_rows = hi - lo + 1;
        }
        strip_bytes =
            strip_rows > 0 ? (size_t)strip_rows * in_cols * cell_size : 0;
        out_bytes = (size_t)(h + 1) * g->ocols * cell_size;
        if (!(strip_bytes + out_mult * out_bytes <= cap_bytes))
            break;
        accepted = h + 1;
    }
    return accepted;
}

/* Number of column blocks in the grid. */
int fg_num_blocks(const struct footprint_grid *g)
{
    return g->nb;
}

/* First output column of block b. Block g->nb starts at the output width. */
int fg_block_start(const struct footprint_grid *g, int b)
{
    return block_c0(g, b);
}

/* Worst strip among the tiles that pack k whole blocks each across the band. */
static int worst_ktile_rows(const struct footprint_grid *g, int obr0, int obr1,
                            int k)
{
    int worst = 0, tb;

    for (tb = 0; tb < g->nb; tb += k) {
        int te = tb + k < g->nb ? tb + k : g->nb;
        int imin, imax, rows;

        fg_span(g, obr0, obr1, block_c0(g, tb), block_c0(g, te), &imin, &imax);
        rows = imax - imin + 1;
        if (rows > worst)
            worst = rows;
    }
    return worst;
}

/* Widest tile in whole blocks whose worst strip and the output still fit the
 * cap, or zero when even one block per tile busts. Reports the worst single
 * block strip for the caller message. */
int fg_tile_blocks(const struct footprint_grid *g, int obr0, int obr1,
                   size_t cap_bytes, int out_mult, int cell_size, int in_cols,
                   int *worst_block_rows)
{
    size_t out_bytes = (size_t)(obr1 - obr0) * g->ocols * cell_size;
    int k;

    *worst_block_rows = worst_ktile_rows(g, obr0, obr1, 1);
    if (out_mult * out_bytes > cap_bytes)
        return 0;
    for (k = g->nb; k >= 1; k--) {
        int worst = worst_ktile_rows(g, obr0, obr1, k);
        size_t strip_bytes =
            worst > 0 ? (size_t)worst * in_cols * cell_size : 0;

        if (strip_bytes + out_mult * out_bytes <= cap_bytes)
            return k;
    }
    return 0;
}

/* Widens every non-empty cell by the sampling margin. */
void fg_apply_sampling_margin(struct footprint_grid *g)
{
    size_t n = (size_t)g->grows * g->nb, i;

    for (i = 0; i < n; i++) {
        struct fg_cell *cell = &g->cell[i];

        if (cell->rmax >= cell->rmin) {
            cell->rmin -= FG_SAMPLING_MARGIN;
            cell->rmax += FG_SAMPLING_MARGIN;
        }
    }
}

void fg_free(struct footprint_grid *g)
{
    if (!g)
        return;
    G_free(g->cell);
    G_free(g);
}
