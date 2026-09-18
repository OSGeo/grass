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

/* The output is cut into nb column slices called blocks. A band is a
   chunk of consecutive output rows processed together. A tile is one
   band tall and k blocks wide and each tile is read as one strip. */

/* Geographic poles that land inside the output map, each stored as its output
 * position and its input row. Empty when no pole lands inside. */
struct pole_set {
    int n;               /* active poles, 0 to 2 */
    double ox[2], oy[2]; /* pole coordinates in the output CRS */
    double pole_row[2];  /* pole input row index */
};

struct fp_cell {
    /* lowest and highest input row one block of one output row needs */
    double rmin, rmax; /* rmax below rmin marks an empty cell */
};

struct footprint {
    int orows;            /* output rows, one grid row per output row */
    int nb;               /* output column blocks, at most 32 */
    int ocols;            /* output columns */
    int irows;            /* number of rows in the input map */
    struct fp_cell *cell; /* orows by nb cells in row major order */
};

/* A block's input row range comes from projecting its first and last
   column. Those two samples give the exact range only when the input
   row changes monotonically across the block. When it does not, a
   middle column can sit a fraction of a row outside the two samples.
   The margin below and the strip reload in main.c cover that case. */
#define FP_SAMPLING_MARGIN 1.0

/* When ocols does not divide evenly by nb, the extra columns are spread
   out so no two blocks differ by more than one column. */
/* Returns the first output column of block b. */
static int block_first_col(const struct footprint *g, int b)
{
    return (int)((long long)b * g->ocols / g->nb);
}

/* Returns the block that contains output column c. */
static int block_of_col(const struct footprint *g, int c)
{
    return (int)(((long long)(c + 1) * g->nb - 1) / g->ocols);
}

/* Projects the center of output cell (r, c) to an input row index. Returns 0 on
 * a failed transform and leaves ri unchanged. */
static int sample_row_index(const struct Cell_head *ohd,
                            const struct Cell_head *ihd,
                            const struct pj_info *oproj,
                            const struct pj_info *iproj,
                            const struct pj_info *tproj, const double *y_center,
                            int r, int c, double *ri)
{
    double xx = ohd->west + (c + 0.5) * ohd->ew_res;
    double yy = y_center[r];

    if (GPJ_transform(oproj, iproj, tproj, PJ_FWD, &xx, &yy, NULL) < 0 ||
        !isfinite(yy))
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
    if (!poles)
        return;

    int c0 = block_first_col(g, b), c1 = block_first_col(g, b + 1), k;
    double x_lo = ohd->west + c0 * ohd->ew_res;
    double x_hi = ohd->west + c1 * ohd->ew_res;
    double y_lo = ohd->north - (r + 1) * ohd->ns_res;
    double y_hi = ohd->north - r * ohd->ns_res;

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
        /* The pole sits outside the input's latitude range while its
           projected position still ends up inside an output cell. The two edge
           samples of that cell never reach the top of the input, so this line
           forces the first or last input row into the cell's range. Without
           this the strip for the cell holding the pole is too short. */
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
    size_t n = (size_t)g->orows * g->nb, i;

    for (i = 0; i < n; i++) {
        struct fp_cell *cell = &g->cell[i];

        /* A cell stays empty when both of its boundary columns fail to
           transform. */
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
    struct footprint *g = G_malloc(sizeof *g);
    struct pole_set poles;
    int r, b;
    double *bnd;

    build_pole_set(ihd, oproj, iproj, tproj, &poles);

    g->orows = ohd->rows;
    g->nb = MIN(ohd->cols, 32);
    g->ocols = ohd->cols;
    g->irows = ihd->rows;
    g->cell = G_malloc(sizeof *g->cell * (size_t)g->orows * g->nb);
    /* nb blocks need nb plus 1 boundary columns. */
    bnd = G_malloc(sizeof *bnd * ((size_t)g->nb + 1));

    for (r = 0; r < g->orows; r++) {
        /* Sample the number of blocks (g->nb) plus one boundary columns for
         * this row. The last boundary is the final column. */
        int k;

        for (k = 0; k <= g->nb; k++) {
            int c = k < g->nb ? block_first_col(g, k) : g->ocols - 1;

            if (!sample_row_index(ohd, ihd, oproj, iproj, tproj, y_center, r, c,
                                  &bnd[k]))
                bnd[k] = DBL_MAX; /* a failed sample is left out of the range */
        }
        for (b = 0; b < g->nb; b++) {
            struct fp_cell *cell = &g->cell[(size_t)r * g->nb + b];
            double lo = MIN(bnd[b], bnd[b + 1]);
            double hi = MAX(bnd[b], bnd[b + 1]);

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

/* Fills *imin and *imax with the input rows needed for the given output
   rows and columns. end_row is one past the last row and end_col is one
   past the last column, same as the loop bounds, so the last band ends
   with end_row equal to orows. Looks at every grid cell in that rectangle,
   skips the empty ones, takes the min and max, then adds two pad rows for
   the bicubic and lanczos method reads. */
void fp_span(const struct footprint *g, int first_row, int end_row,
             int first_col, int end_col, int *imin, int *imax)
{
    double rmin = DBL_MAX, rmax = -DBL_MAX;
    int b_lo = block_of_col(g, first_col), b_hi = block_of_col(g, end_col - 1);
    int r, b;

    if (end_row > g->orows)
        G_fatal_error(_("Footprint grid has %d rows but output row %d was "
                        "requested"),
                      g->orows, end_row - 1);

    for (r = first_row; r < end_row; r++)
        for (b = b_lo; b <= b_hi; b++) {
            const struct fp_cell *cell = &g->cell[(size_t)r * g->nb + b];

            if (cell->rmax < cell->rmin)
                continue; /* empty cell */
            if (cell->rmin < rmin)
                rmin = cell->rmin;
            if (cell->rmax > rmax)
                rmax = cell->rmax;
        }

    if (rmax < rmin) {
        /* An empty span reads as zero rows because the caller computes imax
           minus imin plus one. */
        *imin = 0;
        *imax = -1;
        return;
    }
    /* Two pad rows each way keep the bicubic and lanczos neighbor reads inside
       the strip. */
    int lo = (int)floor(rmin) - 2;
    int hi = (int)floor(rmax) + 2;

    /* The pad can step past the first or last input row near the map edges. */
    if (lo < 0)
        lo = 0;
    if (hi > g->irows - 1)
        hi = g->irows - 1;
    *imin = lo;
    *imax = hi;
}

/* Block count for main.c's tile loop. */
int fp_num_blocks(const struct footprint *g)
{
    return g->nb;
}

/* First output column of block b. b equal to nb gives the output width. */
int fp_block_start(const struct footprint *g, int b)
{
    return block_first_col(g, b);
}

/* Largest number of input rows any k block wide tile of this band needs. */
static int tallest_tile_rows(const struct footprint *g, int first_row,
                             int end_row, int k)
{
    int tallest = 0, tile_start;

    for (tile_start = 0; tile_start < g->nb; tile_start += k) {
        int tile_end = MIN(tile_start + k, g->nb);
        int imin, imax, rows;

        fp_span(g, first_row, end_row, block_first_col(g, tile_start),
                block_first_col(g, tile_end), &imin, &imax);
        rows = imax - imin + 1;
        if (rows > tallest)
            tallest = rows;
    }
    return tallest;
}

/* Finds the widest tile, counted in whole blocks, whose input strip plus
   output buffers fit under cap_bytes. Returns zero when even a single block
   tile is too big. first_row and end_row are the band's rows, with end_row
   one past the last. cell_size is the bytes per cell and in_cols is the
   input map width. out_mult is how many output band buffers exist at once.
   It is two when the previous band's write overlaps the next band's
   compute, otherwise one. */
static int tile_blocks_for_band(const struct footprint *g, int first_row,
                                int end_row, size_t cap_bytes, int out_mult,
                                int cell_size, int in_cols)
{
    size_t out_bytes = (size_t)(end_row - first_row) * g->ocols * cell_size;
    int k;

    if (out_mult * out_bytes > cap_bytes)
        return 0;
    for (k = g->nb; k >= 1; k--) {
        int worst = tallest_tile_rows(g, first_row, end_row, k);
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
int fp_band_geometry(const struct footprint *g, int first_row, size_t cap_bytes,
                     int out_mult, int cell_size, int in_cols,
                     int *tile_blocks_out, int *worst_block_rows)
{
    int remaining = g->orows - first_row;
    int best_h = 0, best_k = 0, try_height;

    /* Measures the single first row as a one block tile, the smallest read the
       module could ever do. If even that does not fit the cap, this number is
       used to tell the user how much memory the parallel path would need
       before the serial tile cache takes over. */
    *worst_block_rows = tallest_tile_rows(g, first_row, first_row + 1, 1);

    /* Growing a band can only add input rows, never remove them. So once a
       height is too big, every bigger height is too big as well. That means
       only the first height that fails matters, and doubling finds it in a
       few tries instead of counting up one at a time. The step after
       remaining / 2 goes straight to remaining instead of past it. */
    for (try_height = 1;;
         try_height = try_height > remaining / 2 ? remaining : try_height * 2) {
        /* band_h is the band height in rows. */
        int band_h = MIN(try_height, remaining);
        int worst = tallest_tile_rows(g, first_row, first_row + band_h, g->nb);
        size_t strip_bytes =
            worst > 0 ? (size_t)worst * in_cols * cell_size : 0;
        size_t out_bytes = (size_t)band_h * g->ocols * cell_size;

        if (strip_bytes + out_mult * out_bytes > cap_bytes)
            break;
        best_h = band_h;
        if (band_h == remaining)
            break;
    }
    if (best_h > 0) {
        /* tile_blocks is nb here, meaning one full width tile. */
        *tile_blocks_out = g->nb;
        return best_h;
    }

    /* One full-width row busts the cap, so grow while the exhaustive scan finds
     * any fitting whole-block tile. */
    for (try_height = 1;;
         try_height = try_height > remaining / 2 ? remaining : try_height * 2) {
        int band_h = MIN(try_height, remaining);
        int k = tile_blocks_for_band(g, first_row, first_row + band_h,
                                     cap_bytes, out_mult, cell_size, in_cols);

        if (k == 0)
            break;
        best_h = band_h;
        best_k = k;
        if (band_h == remaining)
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
