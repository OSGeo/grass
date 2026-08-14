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

/* Geographic poles that land inside the output map, each stored as its output
 * position and its input row. Empty when no pole lands inside. */
struct pole_set {
    int n;               /* active poles, 0 to 2 */
    double ox[2], oy[2]; /* pole coordinates in the output CRS */
    double pole_row[2];  /* pole input row index */
};

struct fp_cell {
    double rmin, rmax; /* rmax below rmin marks an empty cell */
};

struct footprint {
    int grows, nb;        /* grid rows and column blocks */
    int ocols;            /* output columns */
    int irows;            /* input rows */
    struct fp_cell *cell; /* grows by nb cells in row major order */
};

/* The samples can miss a curve between columns by a fraction of a row, so each
 * cell is widened by one row. */
#define FP_SAMPLING_MARGIN 1.0

/* Returns the first output column of block b. */
static int block_c0(const struct footprint *g, int b)
{
    return (int)((long)b * g->ocols / g->nb);
}

/* Returns the block that contains output column c. */
static int block_of_col(const struct footprint *g, int c)
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
static void fold_poles(const struct footprint *g, const struct Cell_head *ohd,
                       const struct pole_set *poles, int r, int b,
                       struct fp_cell *cell)
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
        if (poles->pole_row[k] < cell->rmin)
            cell->rmin = poles->pole_row[k];
        if (poles->pole_row[k] > cell->rmax)
            cell->rmax = poles->pole_row[k];
    }
}

/* For a lat/lon input, projects the north and south poles into the output and
 * records each pole's input row, clamped to the map. A pole is the highest or
 * lowest latitude, which the column samples can step over, so keeping its row
 * makes sure the loaded strip reaches it. Leaves the set empty when no pole
 * lands inside the output map. */
static void build_pole_set(const struct Cell_head *ihd,
                           const struct pj_info *oproj,
                           const struct pj_info *iproj,
                           const struct pj_info *tproj, struct pole_set *poles)
{
    poles->n = 0;
    if (ihd->proj != PROJECTION_LL)
        return;
    double polelat[2] = {90.0, -90.0};

    for (int p = 0; p < 2; p++) {
        double px = 0.0, py = polelat[p];

        if (GPJ_transform(oproj, iproj, tproj, PJ_INV, &px, &py, NULL) < 0 ||
            !isfinite(px) || !isfinite(py))
            continue;
        double ri = (ihd->north - polelat[p]) / ihd->ns_res;
        if (ri < 0)
            ri = 0;
        else if (ri > ihd->rows - 1)
            ri = ihd->rows - 1;
        poles->ox[poles->n] = px;
        poles->oy[poles->n] = py;
        poles->pole_row[poles->n] = ri;
        poles->n++;
    }
}

/* Widens every non-empty cell by the sampling margin. */
static void apply_sampling_margin(struct footprint *g)
{
    size_t n = (size_t)g->grows * g->nb, i;

    for (i = 0; i < n; i++) {
        struct fp_cell *cell = &g->cell[i];

        if (cell->rmax >= cell->rmin) {
            cell->rmin -= FP_SAMPLING_MARGIN;
            cell->rmax += FP_SAMPLING_MARGIN;
        }
    }
}

/* Builds the footprint from block boundary samples, folds in any poles, and
 * applies the sampling margin. */
struct footprint *fp_create(const struct Cell_head *ohd,
                            const struct Cell_head *ihd,
                            const struct pj_info *oproj,
                            const struct pj_info *iproj,
                            const struct pj_info *tproj, const double *y_center)
{
    struct footprint *g = G_malloc(sizeof(*g));
    struct pole_set poles;
    int r, b;
    double *bnd;

    build_pole_set(ihd, oproj, iproj, tproj, &poles);

    g->grows = ohd->rows;
    g->nb = ohd->cols < 32 ? ohd->cols : 32;
    g->ocols = ohd->cols;
    g->irows = ihd->rows;
    g->cell = G_malloc((size_t)g->grows * g->nb * sizeof(struct fp_cell));
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
            struct fp_cell *cell = &g->cell[(size_t)r * g->nb + b];
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
            fold_poles(g, ohd, &poles, r, b, cell);
        }
    }
    G_free(bnd);
    apply_sampling_margin(g);
    return g;
}

/* Returns the input row span covering the output rectangle. Includes every
 * block the rectangle touches and adds a two cell margin. The grid holds one
 * row per output row, so every output row in the rectangle indexes a grid row.
 */
void fp_span(const struct footprint *g, int obr0, int obr1, int obc0, int obc1,
             int *imin, int *imax)
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
            const struct fp_cell *cell = &g->cell[(size_t)r * g->nb + b];

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

/* Number of column blocks in the grid. */
int fp_num_blocks(const struct footprint *g)
{
    return g->nb;
}

/* First output column of block b. Block g->nb starts at the output width. */
int fp_block_start(const struct footprint *g, int b)
{
    return block_c0(g, b);
}

/* Worst strip among the tiles that pack k whole blocks each across the band. */
static int worst_ktile_rows(const struct footprint *g, int obr0, int obr1,
                            int k)
{
    int worst = 0, tb;

    for (tb = 0; tb < g->nb; tb += k) {
        int te = tb + k < g->nb ? tb + k : g->nb;
        int imin, imax, rows;

        fp_span(g, obr0, obr1, block_c0(g, tb), block_c0(g, te), &imin, &imax);
        rows = imax - imin + 1;
        if (rows > worst)
            worst = rows;
    }
    return worst;
}

/* Widest tile in whole blocks whose worst strip and the output fit the cap, or
 * zero when even one block per tile busts. */
static int tile_blocks_for_band(const struct footprint *g, int obr0, int obr1,
                                size_t cap_bytes, int out_mult, int cell_size,
                                int in_cols)
{
    size_t out_bytes = (size_t)(obr1 - obr0) * g->ocols * cell_size;
    int k;

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

/* Grows the band height by doubling, preferring full-width bands and tiling
 * only when even one full-width row busts the cap, and takes the last fitting
 * height with its widest tile. Reports the finest tile strip the fallback
 * message needs and returns zero when even one tiled row busts. */
int fp_band_geometry(const struct footprint *g, int obr0, size_t cap_bytes,
                     int out_mult, int cell_size, int in_cols,
                     int *tile_blocks_out, int *worst_block_rows)
{
    int remaining = g->grows - obr0;
    int best_h = 0, best_k = 0, h_cand;

    *worst_block_rows = worst_ktile_rows(g, obr0, obr0 + 1, 1);

    /* Prefer full-width bands, growing the height while the whole row still
     * fits the cap as a single tile. */
    for (h_cand = 1;; h_cand *= 2) {
        int h = h_cand < remaining ? h_cand : remaining;
        int worst = worst_ktile_rows(g, obr0, obr0 + h, g->nb);
        size_t strip_bytes =
            worst > 0 ? (size_t)worst * in_cols * cell_size : 0;
        size_t out_bytes = (size_t)h * g->ocols * cell_size;

        if (strip_bytes + out_mult * out_bytes > cap_bytes)
            break;
        best_h = h;
        if (h == remaining)
            break;
    }
    if (best_h > 0) {
        *tile_blocks_out = g->nb;
        return best_h;
    }

    /* One full-width row busts the cap, so grow while the exhaustive scan finds
     * any fitting whole-block tile. */
    for (h_cand = 1;; h_cand *= 2) {
        int h = h_cand < remaining ? h_cand : remaining;
        int k = tile_blocks_for_band(g, obr0, obr0 + h, cap_bytes, out_mult,
                                     cell_size, in_cols);

        if (k == 0)
            break;
        best_h = h;
        best_k = k;
        if (h == remaining)
            break;
    }
    if (best_h == 0) {
        *tile_blocks_out = 0;
        return 0;
    }
    *tile_blocks_out = best_k;
    return best_h;
}

void fp_free(struct footprint *g)
{
    if (!g)
        return;
    G_free(g->cell);
    G_free(g);
}
