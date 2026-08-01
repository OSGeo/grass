/*
 * footprint.c - grid of input row spans for the output map.
 *
 * Each cell covers one output row and one column block and holds the range of
 * input rows that block reaches.
 */

#include <float.h>
#include <math.h>
#include <stdio.h>

#include <grass/gis.h>
#include <grass/glocale.h>

#include "r.proj.h"

struct fg_cell {
    double rmin, rmax; /* rmax below rmin marks an empty cell */
};

struct footprint_grid {
    int variant;          /* FG_BOUNDARY or FG_EXACT */
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

/* Builds the grid using boundary samples or every column. */
struct footprint_grid *
fg_build(const struct Cell_head *ohd, const struct Cell_head *ihd,
         const struct pj_info *oproj, const struct pj_info *iproj,
         const struct pj_info *tproj, const double *y_center,
         const struct pole_set *poles, int variant)
{
    struct footprint_grid *g = G_malloc(sizeof(*g));
    int r, b;
    double *bnd = NULL;

    g->variant = variant;
    g->grows = ohd->rows;
    g->nb = ohd->cols < 32 ? ohd->cols : 32;
    g->ocols = ohd->cols;
    g->irows = ihd->rows;
    g->cell = G_malloc((size_t)g->grows * g->nb * sizeof(struct fg_cell));

    if (variant == FG_BOUNDARY)
        bnd = G_malloc((size_t)(g->nb + 1) * sizeof(double));

    for (r = 0; r < g->grows; r++) {
        if (variant == FG_BOUNDARY) {
            /* Sample the NB plus one block boundaries for this row. The last
             * boundary uses the final valid column. */
            int k;

            for (k = 0; k <= g->nb; k++) {
                int c = block_c0(g, k);

                if (c > g->ocols - 1)
                    c = g->ocols - 1;
                if (!sample_ri(ohd, ihd, oproj, iproj, tproj, y_center, r, c,
                               &bnd[k]))
                    bnd[k] =
                        DBL_MAX; /* a failed sample is left out of the range */
            }
        }
        for (b = 0; b < g->nb; b++) {
            struct fg_cell *cell = &g->cell[(size_t)r * g->nb + b];

            cell->rmin = DBL_MAX;
            cell->rmax = -DBL_MAX;
            if (variant == FG_BOUNDARY) {
                double lo = bnd[b] < bnd[b + 1] ? bnd[b] : bnd[b + 1];
                double hi = bnd[b] > bnd[b + 1] ? bnd[b] : bnd[b + 1];

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
            }
            else {
                /* Scan every column in the block. */
                int c0 = block_c0(g, b), c1 = block_c0(g, b + 1), c;

                for (c = c0; c < c1; c++) {
                    double ri;

                    if (!sample_ri(ohd, ihd, oproj, iproj, tproj, y_center, r,
                                   c, &ri))
                        continue;
                    if (ri < cell->rmin)
                        cell->rmin = ri;
                    if (ri > cell->rmax)
                        cell->rmax = ri;
                }
            }
            fold_poles(g, ohd, poles, r, b, cell);
        }
    }
    if (bnd)
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

/* Reports how many cells the exact variant makes wider than the boundary
 * variant, with the largest widening on each side. */
void fg_compare_variants(const struct footprint_grid *b,
                         const struct footprint_grid *e)
{
    size_t n = (size_t)b->grows * b->nb, i;
    long differ = 0;
    double max_lo_gap = 0.0, max_hi_gap = 0.0;

    for (i = 0; i < n; i++) {
        const struct fg_cell *cb = &b->cell[i], *ce = &e->cell[i];
        double lo_gap, hi_gap;

        if (cb->rmax < cb->rmin && ce->rmax < ce->rmin)
            continue;
        lo_gap = cb->rmin - ce->rmin; /* exact reaches this much lower */
        hi_gap = ce->rmax - cb->rmax; /* exact reaches this much higher */
        if (lo_gap > 0.0 || hi_gap > 0.0) {
            differ++;
            if (lo_gap > max_lo_gap)
                max_lo_gap = lo_gap;
            if (hi_gap > max_hi_gap)
                max_hi_gap = hi_gap;
        }
    }
    fprintf(stderr,
            "FG_VAR cells=%ld differ=%ld max_lo_gap=%.3f max_hi_gap=%.3f\n",
            (long)n, differ, max_lo_gap, max_hi_gap);
}

/* Widens every non-empty cell of a boundary grid by the sampling margin. */
void fg_apply_sampling_margin(struct footprint_grid *g)
{
    size_t n = (size_t)g->grows * g->nb, i;

    if (g->variant != FG_BOUNDARY)
        return;
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
