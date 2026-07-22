/***************************************************************************
*
* MODULE:        r.proj
*
* AUTHOR(S):     Martin Schroeder
*                  University of Heidelberg
*                  Dept. of Geography
*                  emes@geo0.geog.uni-heidelberg.de
*
*                  (With the help of a lot of existing GRASS sources, in
*                  particular v.proj)
*
* PURPOSE:       r.proj converts a map to a new geographic projection. It reads
                 a map from a different location, projects it and write it out
*                to the current location. The projected data is resampled with
*                one of three different methods: nearest neighbor, bilinear and
*                cubic convolution.
*
* COPYRIGHT:     (C) 2001, 2011 by the GRASS Development Team
*
*                This program is free software under the GNU General Public
*                License (>=v2). Read the file COPYING that comes with GRASS
*                for details.
*
* Changes
*                Morten Hulden <morten@untamo.net>, Aug 2000:
*                - aborts if input map is outside current location.
*                - can handle projections (conic, azimuthal etc) where
*                part of the map may fall into areas where south is
*                upward and east is leftward.
*                - avoids passing location edge coordinates to PROJ
*                (they may be invalid in some projections).
*                - output map will be clipped to borders of the current region.
*                - output map cell edges and centers will coincide with those
*                of the current region.
*                - output map resolution (unless changed explicitly) will
*                match (exactly) the resolution of the current region.
*                - if the input map is smaller than the current region, the
*                output map will only cover the overlapping area.
*                - if the input map is larger than the current region, only the
*                 needed amount of memory will be allocated for the projection
*
*                Bugfixes 20050328: added floor() before (int) typecasts to in
*                avoid  asymmetrical rounding errors. Added missing offset
*                outcellhd.ew_res/2 to initial xcoord for each row in main
*                projection loop (we want to  project center of cell, not
*                border).
*
*                Glynn Clements 2006: Use G_interp_* functions, modified
*                version of r.proj which uses a tile cache instead  of loading
*                the entire map into memory.
*                Markus Metz 2010: lanczos and lanczos fallback interpolation
*                methods

*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/gprojects.h>
#include <grass/glocale.h>
#include <grass/gjson.h>
#include "r.proj.h"

#ifdef _OPENMP
#include <omp.h>
static inline double rproj_wtime(void)
{
    return omp_get_wtime();
}
#else
static inline double rproj_wtime(void)
{
    return 0.0;
}
#endif

/* modify this table to add new methods */
struct menu menu[] = {
    {p_nearest, "nearest", "nearest neighbor"},
    {p_bilinear, "bilinear", "bilinear interpolation"},
    {p_cubic, "bicubic", "bicubic interpolation"},
    {p_lanczos, "lanczos", "lanczos filter"},
    {p_bilinear_f, "bilinear_f", "bilinear interpolation with fallback"},
    {p_cubic_f, "bicubic_f", "bicubic interpolation with fallback"},
    {p_lanczos_f, "lanczos_f", "lanczos filter with fallback"},
    {NULL, NULL, NULL}};

static char *make_ipol_list(void);
static char *make_ipol_desc(void);

/* Nearest read from an in-RAM input STRIP holding input rows [imin, imax].
 * col_idx/row_idx are full-map input indices; the strip is addressed relative
 * to imin. A sample inside the full input map but outside the loaded strip
 * means the band footprint was under-sized: this is the stop-on-divergence
 * trip (must never fire if band_input_row_span is correct). Lock-free: reads
 * only, disjoint output slots per thread. */
static void interpolate_strip(void *strip, void *obufptr, int cell_type,
                              double col_idx, double row_idx,
                              struct Cell_head *incellhd, int imin, int imax)
{
    int c = (int)floor(col_idx);
    int r = (int)floor(row_idx);
    int cell_size = Rast_cell_size(cell_type);

    /* Outside the full input map: legitimate NULL (same as p_nearest). */
    if (r < 0 || r >= incellhd->rows || c < 0 || c >= incellhd->cols) {
        Rast_set_null_value(obufptr, 1, cell_type);
        return;
    }

    /* This input row is inside the input map (the check above already handled
     * coordinates that fall outside it), but it is not among the rows we
     * preloaded into this band's strip. That cannot happen if the band's
     * footprint estimate was right, so it means the estimate was wrong: a bug
     * in band sizing, not a normal case. Fail loudly rather than write a NULL
     * and silently produce wrong output. */
    if (r < imin || r > imax)
        G_fatal_error(_("Band strip under-sized: input row %d outside loaded "
                        "range [%d, %d] at column %d"),
                      r, imin, imax, c);

    unsigned char *src =
        (unsigned char *)strip +
        (((size_t)(r - imin) * incellhd->cols + c) * cell_size);
    memcpy(obufptr, src, cell_size);
}

/* Strip-based kernels for the banded compute path, in the same order as menu[]:
 * slot i is the strip counterpart of menu[i].method. Slot 0 is nearest
 * (interpolate_strip above); slots 1-6 are the interp_strip.c kernels. */
static const strip_func strip_kernels[] = {
    interpolate_strip, strip_bilinear, strip_cubic,    strip_lanczos,
    strip_bilinear_f,  strip_cubic_f,  strip_lanczos_f};

/* Geographic poles within the input map's latitude coverage.
 * band_input_row_span samples only the tile perimeter, so a tile whose interior
 * holds a pole has an input-row (latitude) extremum the perimeter misses; the
 * pole's row is folded into that tile's span. Each pole is stored as its
 * output-CRS coordinate (for a point-in-tile test) and its input row. Filled
 * once per map, and empty (n == 0) whenever no pole is in frame, so pole
 * handling is a no-op on such maps. Assumes a pole maps to a single output
 * point (azimuthal/stereographic); for a projection that images a pole as a
 * line or arc, the under-size guard remains the backstop. */
struct pole_set {
    int n;               /* active poles, 0..2 */
    double ox[2], oy[2]; /* pole coordinates in the output CRS */
    double ri[2];        /* pole input row index */
};

/* Dense edge-walk of an output tile's rectangle [obr0, obr1) x [obc0, obc1)
 * projected into input space; returns the min/max INPUT ROW touched, plus a
 * 2-cell margin, clamped to the input map. Samples the tile's top and bottom
 * rows across its columns [obc0, obc1) and its left and right columns across
 * its rows (bordwalk-style), so a curved transform's interior-edge extremum is
 * caught -- corner-only sampling can under-size the strip. A full-width band is
 * the case obc0=0, obc1=cols. Called serially, before the parallel region, so
 * the shared tproj is safe here. Returns imax < imin for a tile that projects
 * entirely outside the input. */
static void
band_input_row_span(const struct Cell_head *ohd, const struct Cell_head *ihd,
                    const struct pj_info *oproj, const struct pj_info *iproj,
                    const struct pj_info *tproj, const double *y_center,
                    int obr0, int obr1, int obc0, int obc1, int *imin,
                    int *imax, const struct pole_set *poles, int *pole_widened)
{
    double rmin = 1e300, rmax = -1e300;
    int e, r, c;

    /* top edge (row obr0) and bottom edge (row obr1-1), tile columns */
    for (e = 0; e < 2; e++) {
        int orow = (e == 0) ? obr0 : (obr1 - 1);
        double y = y_center[orow];
        for (c = obc0; c < obc1; c++) {
            double x = ohd->west + (c + 0.5) * ohd->ew_res;
            double xx = x, yy = y;
            if (GPJ_transform(oproj, iproj, tproj, PJ_FWD, &xx, &yy, NULL) < 0)
                continue;
            double ri = (ihd->north - yy) / ihd->ns_res;
            if (ri < rmin)
                rmin = ri;
            if (ri > rmax)
                rmax = ri;
        }
    }
    /* left edge (col obc0) and right edge (col obc1-1), all band rows */
    for (e = 0; e < 2; e++) {
        int ocol = (e == 0) ? obc0 : (obc1 - 1);
        double x = ohd->west + (ocol + 0.5) * ohd->ew_res;
        for (r = obr0; r < obr1; r++) {
            double y = y_center[r];
            double xx = x, yy = y;
            if (GPJ_transform(oproj, iproj, tproj, PJ_FWD, &xx, &yy, NULL) < 0)
                continue;
            double ri = (ihd->north - yy) / ihd->ns_res;
            if (ri < rmin)
                rmin = ri;
            if (ri > rmax)
                rmax = ri;
        }
    }

    /* Fold in any pole whose output point lies in this tile's rect: the
     * perimeter walk cannot see an interior latitude extremum. A pole exactly
     * on a tile edge (inclusive test) is caught by both adjacent tiles, which
     * is harmless -- it only widens a strip that is loaded anyway. Placed
     * before the empty-tile check so a pole inside an otherwise-outside tile
     * still yields a valid span. */
    if (poles) {
        double x_lo = ohd->west + obc0 * ohd->ew_res;
        double x_hi = ohd->west + obc1 * ohd->ew_res;
        double y_lo = ohd->north - obr1 * ohd->ns_res;
        double y_hi = ohd->north - obr0 * ohd->ns_res;
        int k;

        for (k = 0; k < poles->n; k++) {
            if (poles->ox[k] < x_lo || poles->ox[k] > x_hi ||
                poles->oy[k] < y_lo || poles->oy[k] > y_hi)
                continue;
            if (poles->ri[k] < rmin)
                rmin = poles->ri[k];
            if (poles->ri[k] > rmax)
                rmax = poles->ri[k];
            if (pole_widened)
                *pole_widened = k + 1; /* 1-based pole index, 0 == none */
        }
    }

    if (rmax < rmin) { /* band projects entirely outside the input */
        *imin = 0;
        *imax = -1;
        return;
    }

    int lo = (int)floor(rmin) - 2; /* 2-cell margin for interp stencils */
    int hi = (int)floor(rmax) + 2;
    if (lo < 0)
        lo = 0;
    if (hi > ihd->rows - 1)
        hi = ihd->rows - 1;
    *imin = lo;
    *imax = hi;
}

/* Largest input-row strip (in rows) among the column tiles of width tilew that
 * partition output columns [0, ohd->cols) for the band [obr0, obr1). Tiles are
 * loaded one at a time, so peak strip memory is set by the worst tile, not the
 * union of the band's tiles; the fit search sizes this against the cap. Every
 * call is a full tile edge-walk, so this is O(tiles * perimeter) -- paid in the
 * serial size phase, and only when column splitting is actually entered.
 * Returns 0 if every tile projects entirely outside the input. */
static int worst_tile_strip_rows(const struct Cell_head *ohd,
                                 const struct Cell_head *ihd,
                                 const struct pj_info *oproj,
                                 const struct pj_info *iproj,
                                 const struct pj_info *tproj,
                                 const double *y_center, int obr0, int obr1,
                                 int tilew, const struct pole_set *poles)
{
    int worst = 0, obc0;

    for (obc0 = 0; obc0 < ohd->cols; obc0 += tilew) {
        int obc1 = obc0 + tilew;
        int imin, imax, rows;

        if (obc1 > ohd->cols)
            obc1 = ohd->cols;
        band_input_row_span(ohd, ihd, oproj, iproj, tproj, y_center, obr0, obr1,
                            obc0, obc1, &imin, &imax, poles, NULL);
        rows = imax - imin + 1; /* imax < imin (empty) -> <= 0, ignored */
        if (rows > worst)
            worst = rows;
    }
    return worst;
}

#define TILE_PROBE 16 /* tiles sampled by the Phase-2 width-search estimate */

/* Cheap estimate of worst_tile_strip_rows: the largest input-row strip among
 * at most `probe` column tiles, evenly spaced across the band width and always
 * including the first and last. A subset max is a LOWER bound on the true
 * worst, so it only PRUNES the Phase-2 search; the chosen width is exact-
 * validated by worst_tile_strip_rows before use. */
static int est_worst_tile_strip_rows(
    const struct Cell_head *ohd, const struct Cell_head *ihd,
    const struct pj_info *oproj, const struct pj_info *iproj,
    const struct pj_info *tproj, const double *y_center, int obr0, int obr1,
    int tilew, int probe, const struct pole_set *poles)
{
    int ntiles = (ohd->cols + tilew - 1) / tilew;
    int worst = 0, k;

    if (probe < 1)
        probe = 1;
    if (probe > ntiles)
        probe = ntiles;
    for (k = 0; k < probe; k++) {
        int ti = (probe == 1) ? 0 : (int)((long)k * (ntiles - 1) / (probe - 1));
        int obc0 = ti * tilew;
        int obc1 = obc0 + tilew;
        int imin, imax, rows;

        if (obc1 > ohd->cols)
            obc1 = ohd->cols;
        band_input_row_span(ohd, ihd, oproj, iproj, tproj, y_center, obr0, obr1,
                            obc0, obc1, &imin, &imax, poles, NULL);
        rows = imax - imin + 1;
        if (rows > worst)
            worst = rows;
    }
    return worst;
}

/* Exact per-height fit test for the Phase-2 height search: 1 iff a band of
 * height h at obr0 has an output buffer within the cap AND some column-tile
 * width whose worst input strip fits (setting *acc_tilew to that width, via the
 * same upper-tier estimate then lower-tier exact validation the search uses);
 * 0 if no width fits or the output buffer alone exceeds the cap. */
static int phase2_width_fit(const struct Cell_head *ohd,
                            const struct Cell_head *ihd,
                            const struct pj_info *oproj,
                            const struct pj_info *iproj,
                            const struct pj_info *tproj, const double *y_center,
                            int obr0, int h, size_t cap_bytes, int cell_size,
                            int *acc_tilew, const struct pole_set *poles)
{
    size_t out_bytes = (size_t)h * ohd->cols * cell_size;
    int tilew, est_fit;

    if (out_bytes > cap_bytes)
        return 0;
    tilew = ohd->cols;
    est_fit = 0;
    for (;;) {
        int est =
            est_worst_tile_strip_rows(ohd, ihd, oproj, iproj, tproj, y_center,
                                      obr0, obr0 + h, tilew, TILE_PROBE, poles);
        size_t est_bytes = est > 0 ? (size_t)est * ihd->cols * cell_size : 0;
        if (est_bytes + out_bytes <= cap_bytes) {
            est_fit = 1;
            break;
        }
        if (tilew == 1)
            break;
        tilew = (tilew + 1) / 2;
    }
    if (est_fit) {
        for (;;) {
            int worst =
                worst_tile_strip_rows(ohd, ihd, oproj, iproj, tproj, y_center,
                                      obr0, obr0 + h, tilew, poles);
            size_t strip_bytes =
                worst > 0 ? (size_t)worst * ihd->cols * cell_size : 0;
            if (strip_bytes + out_bytes <= cap_bytes) {
                *acc_tilew = tilew;
                return 1;
            }
            if (tilew == 1)
                break;
            tilew = (tilew + 1) / 2;
        }
    }
    return 0;
}

/* Full-width fit test for the Phase-1 height search: 1 iff a band of height h
 * at obr0 has its full-width input strip plus output buffer within the cap.
 * Short-circuits on the output buffer alone (no edge walk) when it already
 * exceeds the cap. Used only by the seed peek; the walk keeps its inline test,
 * so the miss path is byte-for-byte today's execution. */
static int phase1_fits(const struct Cell_head *ohd, const struct Cell_head *ihd,
                       const struct pj_info *oproj, const struct pj_info *iproj,
                       const struct pj_info *tproj, const double *y_center,
                       int obr0, int h, size_t cap_bytes, int cell_size,
                       const struct pole_set *poles)
{
    int imin, imax, strip_rows;
    size_t out_bytes = (size_t)h * ohd->cols * cell_size, strip_bytes;

    if (out_bytes > cap_bytes)
        return 0;
    band_input_row_span(ohd, ihd, oproj, iproj, tproj, y_center, obr0, obr0 + h,
                        0, ohd->cols, &imin, &imax, poles, NULL);
    strip_rows = imax - imin + 1;
    strip_bytes =
        strip_rows > 0 ? (size_t)strip_rows * ihd->cols * cell_size : 0;
    return strip_bytes + out_bytes <= cap_bytes;
}

/* Serial tile-cache fallback for the large-halo/oblique corner: when even a
 * single output row's full-width input strip busts the memory cap (the bail in
 * the band loop), the banded strip path cannot proceed. This finishes the run
 * from output row obr0 onward using the classic readcell block cache (faults
 * blocks on demand, bounded by the same memory option via nblocks) and the
 * CVAL cache kernels (menu[].method), exactly as the serial r.proj does.
 *
 * Runs strictly serially: get_block mutates shared cache state and is not
 * thread-safe. Rows [0, obr0) were already written by the banded path; each
 * output row is independent of the others, so the banded prefix followed by
 * this serial suffix is bit-identical to a pure serial run. y_center supplies
 * the same output-row northings the banded prefix used (and that serial's
 * ycoord2 recurrence produces), so the seam at obr0 is seamless. A transform
 * failure sets NULL here (matching the banded strip path) rather than the old
 * serial fatal; identical on data where transforms succeed. */
static void
fallback_serial_cache(int fdi, int fdo, int cell_type, int method,
                      const struct pj_info *oproj, const struct pj_info *iproj,
                      const struct pj_info *tproj, struct Cell_head *incellhd,
                      struct Cell_head *outcellhd, const double *y_center,
                      int obr0, const char *memory)
{
    struct cache *ibuffer = readcell(fdi, memory);
    func interpolate = menu[method].method;
    void *obuffer = Rast_allocate_output_buf(cell_type);
    int cell_size = Rast_cell_size(cell_type);
    double local_x_start = outcellhd->west + (outcellhd->ew_res / 2);

    for (int row = obr0; row < outcellhd->rows; row++) {
        G_percent(row - obr0, outcellhd->rows - obr0, 5);
        for (int col = 0; col < outcellhd->cols; col++) {
            void *obufptr = (unsigned char *)obuffer + (size_t)col * cell_size;
            double x1 = local_x_start + col * outcellhd->ew_res;
            double y1 = y_center[row];

            if (GPJ_transform(oproj, iproj, tproj, PJ_FWD, &x1, &y1, NULL) <
                0) {
                Rast_set_null_value(obufptr, 1, cell_type);
            }
            else {
                double col_idx = (x1 - incellhd->west) / incellhd->ew_res;
                double row_idx = (incellhd->north - y1) / incellhd->ns_res;

                interpolate(ibuffer, obufptr, cell_type, col_idx, row_idx,
                            incellhd);
            }
        }
        Rast_put_row(fdo, obuffer, cell_type);
    }

    release_cache(ibuffer);
    G_free(obuffer);
}

int main(int argc, char **argv)
{
    char *mapname, /* ptr to name of output layer  */
        *setname,  /* ptr to name of input mapset  */
        *ipolname; /* name of interpolation method */

    int fdi,                       /* input map file descriptor    */
        fdo,                       /* output map file descriptor   */
        method,                    /* position of method in table  */
        permissions,               /* mapset permissions           */
        cell_type,                 /* output celltype              */
        cell_size,                 /* size of a cell in bytes      */
        row, col,                  /* counters                     */
        irows, icols,              /* original rows, cols          */
        orows, ocols, have_colors, /* Input map has a colour table */
        overwrite,                 /* Overwrite                    */
        curr_proj;                 /* output projection (see gis.h) */

    double onorth, osouth, /* save original border coords  */
        oeast, owest, inorth, isouth, ieast, iwest;
    char north_str[30], south_str[30], east_str[30], west_str[30];

    struct Colors colr; /* Input map colour table       */
    struct History history;

    struct pj_info iproj, /* input map proj parameters    */
        oproj,            /* output map proj parameters   */
        tproj;            /* transformation parameters   */

    struct Key_Value *in_proj_info, /* projection information of    */
        *in_unit_info,              /* input and output mapsets     */
        *out_proj_info, *out_unit_info;

    struct GModule *module;

    struct Flag *list,  /* list files in source location */
        *nocrop,        /* don't crop output map        */
        *print_bounds,  /* print output bounds and exit */
        *gprint_bounds; /* same but print shell style   */

    struct Option *imapset, /* name of input mapset         */
        *inmap,             /* name of input layer          */
        *inlocation,        /* name of input location       */
        *outmap,            /* name of output layer         */
        *indbase,           /* name of input database       */
        *interpol,          /* interpolation method         */
        *memory,            /* amount of memory for cache   */
        *res,               /* resolution of target map     */
        *format;            /* output format                */

    struct Option *pipeline;   /* name of custom PROJ pipeline */
    struct Cell_head incellhd, /* cell header of input map     */
        outcellhd;             /* and output map               */

    enum OutputFormat outputFormat;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("projection"));
    G_add_keyword(_("transformation"));
    G_add_keyword(_("import"));
    module->description = _("Re-projects a raster map from given project to "
                            "the current project.");

    inlocation = G_define_standard_option(G_OPT_M_LOCATION);
    inlocation->required = YES;
    inlocation->label = _("Project (location) containing input raster map");
    inlocation->guisection = _("Source");

    imapset = G_define_standard_option(G_OPT_M_MAPSET);
    imapset->label = _("Mapset containing input raster map");
    imapset->description = _("Default: name of current mapset");
    imapset->guisection = _("Source");

    inmap = G_define_standard_option(G_OPT_R_INPUT);
    inmap->description = _("Name of input raster map to re-project");
    inmap->required = NO;
    inmap->guisection = _("Source");

    indbase = G_define_standard_option(G_OPT_M_DBASE);
    indbase->label = _("Path to GRASS database of input project");

    outmap = G_define_standard_option(G_OPT_R_OUTPUT);
    outmap->required = NO;
    outmap->description =
        _("Name for output raster map (default: same as 'input')");
    outmap->guisection = _("Target");

    ipolname = make_ipol_list();

    interpol = G_define_option();
    interpol->key = "method";
    interpol->type = TYPE_STRING;
    interpol->required = NO;
    interpol->answer = "nearest";
    interpol->options = ipolname;
    interpol->description = _("Interpolation method to use");
    interpol->guisection = _("Target");
    interpol->descriptions = make_ipol_desc();

    memory = G_define_standard_option(G_OPT_MEMORYMB);

    res = G_define_option();
    res->key = "resolution";
    res->type = TYPE_DOUBLE;
    res->required = NO;
    res->description = _("Resolution of output raster map");
    res->guisection = _("Target");

    format = G_define_standard_option(G_OPT_F_FORMAT);
    format->options = "plain,shell,json";
    format->descriptions = _("plain;Human readable text output;"
                             "shell;shell script style text output;"
                             "json;JSON (JavaScript Object Notation);");
    format->guisection = _("Print");

    pipeline = G_define_option();
    pipeline->key = "pipeline";
    pipeline->type = TYPE_STRING;
    pipeline->required = NO;
    pipeline->description = _("PROJ pipeline for coordinate transformation");

    list = G_define_flag();
    list->key = 'l';
    list->description = _("List raster maps in input mapset and exit");
    list->guisection = _("Print");

    nocrop = G_define_flag();
    nocrop->key = 'n';
    nocrop->description =
        _("Do not perform region cropping optimization. See Notes if working "
          "with a global latitude-longitude projection");

    print_bounds = G_define_flag();
    print_bounds->key = 'p';
    print_bounds->description =
        _("Print input map's bounds in the current projection and exit");
    print_bounds->guisection = _("Print");

    gprint_bounds = G_define_flag();
    gprint_bounds->key = 'g';
    gprint_bounds->label = _("Print input map's bounds in the current "
                             "projection in shell script style [deprecated]");
    gprint_bounds->description =
        _("This flag is deprecated and will "
          "be removed in a future release. Use format=shell instead.");
    gprint_bounds->guisection = _("Print");

    /* The parser checks if the map already exists in current mapset,
       we switch out the check and do it
       in the module after the parser */
    overwrite = G_check_overwrite(argc, argv);

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    if (strcmp(format->answer, "json") == 0) {
        outputFormat = JSON;
    }
    else if (strcmp(format->answer, "shell") == 0) {
        outputFormat = SHELL;
    }
    else {
        outputFormat = PLAIN;
    }

    if (outputFormat != PLAIN && !print_bounds->answer && !list->answer) {
        G_fatal_error(
            _("The format option can only be used with -%c or -%c flags"),
            print_bounds->key, list->key);
    }

    if (gprint_bounds->answer) {
        G_verbose_message(
            _("Flag 'g' is deprecated and will be removed in a future "
              "release. Please use format=shell instead."));
        outputFormat = SHELL;
    }

    /* get the method */
    for (method = 0; (ipolname = menu[method].name); method++)
        if (strcmp(ipolname, interpol->answer) == 0)
            break;

    if (!ipolname)
        G_fatal_error(_("<%s=%s> unknown %s"), interpol->key, interpol->answer,
                      interpol->key);

    /* Resolve the strip kernel once; menu[] and strip_kernels[] share order. */
    strip_func interp = strip_kernels[method];

    mapname = outmap->answer ? outmap->answer : inmap->answer;
    if (mapname && !list->answer && !overwrite && !print_bounds->answer &&
        outputFormat != SHELL && G_find_raster(mapname, G_mapset()))
        G_fatal_error(_("option <%s>: <%s> exists. To overwrite, use the "
                        "--overwrite flag"),
                      "output", mapname);

    setname = imapset->answer ? imapset->answer : G_store(G_mapset());
    if (strcmp(inlocation->answer, G_location()) == 0 &&
        (!indbase->answer || strcmp(indbase->answer, G_gisdbase()) == 0))
#if 0
        G_fatal_error(_("Input and output locations can not be the same"));
#else
        G_warning(_("Input and output projects are the same"));
#endif
    G_get_window(&outcellhd);

    if (outputFormat == SHELL && !print_bounds->answer)
        print_bounds->answer = 1;
    curr_proj = G_projection();

    /* Get projection info for output mapset */
    if ((out_proj_info = G_get_projinfo()) == NULL)
        G_fatal_error(_("Unable to get projection info of output raster map"));

    if ((out_unit_info = G_get_projunits()) == NULL)
        G_fatal_error(_("Unable to get projection units of output raster map"));

    if (pj_get_kv(&oproj, out_proj_info, out_unit_info) < 0)
        G_fatal_error(
            _("Unable to get projection key values of output raster map"));

    oproj.srid = G_get_projsrid();
    oproj.wkt = G_get_projwkt();

    /* Change the location           */
    G_create_alt_env();
    G_setenv_nogisrc("GISDBASE",
                     indbase->answer ? indbase->answer : G_gisdbase());
    G_setenv_nogisrc("LOCATION_NAME", inlocation->answer);
    G_setenv_nogisrc("MAPSET", setname);

    permissions = G_mapset_permissions(setname);
    if (permissions < 0) /* can't access mapset       */
        G_fatal_error(_("Mapset <%s> in input project <%s> - %s"), setname,
                      inlocation->answer,
                      permissions == 0 ? _("permission denied")
                                       : _("not found"));

    /* if requested, list the raster maps in source location - MN 5/2001 */
    if (list->answer) {
        int i;
        char **srclist;
        G_JSON_Array *maps_array = NULL;
        G_JSON_Value *maps_value = NULL;

        if (outputFormat == JSON) {
            maps_value = G_json_value_init_array();
            if (maps_value == NULL) {
                G_fatal_error(
                    _("Failed to initialize JSON array. Out of memory?"));
            }
            maps_array = G_json_array(maps_value);
        }

        G_verbose_message(_("Checking project <%s> mapset <%s>"),
                          inlocation->answer, setname);
        srclist = G_list(G_ELEMENT_RASTER, G_getenv_nofatal("GISDBASE"),
                         G_getenv_nofatal("LOCATION_NAME"), setname);
        for (i = 0; srclist[i]; i++) {
            switch (outputFormat) {
            case SHELL:
            case PLAIN:
                fprintf(stdout, "%s\n", srclist[i]);
                break;

            case JSON:
                G_json_array_append_string(maps_array, srclist[i]);
                break;
            }
        }
        if (outputFormat == JSON) {
            char *serialized_string = NULL;
            serialized_string = G_json_serialize_to_string_pretty(maps_value);
            if (serialized_string == NULL) {
                G_fatal_error(_("Failed to initialize pretty JSON string."));
            }
            puts(serialized_string);
            G_json_free_serialized_string(serialized_string);
            G_json_value_free(maps_value);
        }
        else {
            fflush(stdout);
        }
        exit(EXIT_SUCCESS); /* leave r.proj after listing */
    }

    if (!inmap->answer)
        G_fatal_error(_("Required parameter <%s> not set"), inmap->key);

    if (!G_find_raster(inmap->answer, setname))
        G_fatal_error(
            _("Raster map <%s> in project <%s> in mapset <%s> not found"),
            inmap->answer, inlocation->answer, setname);

    /* Read input map colour table */
    have_colors = Rast_read_colors(inmap->answer, setname, &colr);

    /* Get projection info for input mapset */
    if ((in_proj_info = G_get_projinfo()) == NULL)
        G_fatal_error(_("Unable to get projection info of input map"));

    /* apparently the +over switch must be set in the input projection,
     * not the output latlon projection
     * TODO: for PROJ 6+, the +over switch must be added to the
     * transformation pipeline if authority:name or WKt are used as
     * crs definition */
    if (curr_proj == PROJECTION_LL)
        G_set_key_value("over", "defined", in_proj_info);

    if ((in_unit_info = G_get_projunits()) == NULL)
        G_fatal_error(_("Unable to get projection units of input map"));

    if (pj_get_kv(&iproj, in_proj_info, in_unit_info) < 0)
        G_fatal_error(_("Unable to get projection key values of input map"));

    iproj.srid = G_get_projsrid();
    iproj.wkt = G_get_projwkt();

    tproj.pj = NULL;
    tproj.def = NULL;
    if (pipeline->answer) {
        tproj.def = G_store(pipeline->answer);
    }

    G_free_key_value(in_proj_info);
    G_free_key_value(in_unit_info);
    G_free_key_value(out_proj_info);
    G_free_key_value(out_unit_info);
    if (G_verbose() > G_verbose_std())
        pj_print_proj_params(&iproj, &oproj);

    /* this call causes r.proj to read the entire map into memory */
    Rast_get_cellhd(inmap->answer, setname, &incellhd);

    if (G_projection() == PROJECTION_XY)
        G_fatal_error(_("Unable to work with unprojected data (xy project)"));

    /* Save default borders so we can show them later */
    inorth = incellhd.north;
    isouth = incellhd.south;
    ieast = incellhd.east;
    iwest = incellhd.west;
    irows = incellhd.rows;
    icols = incellhd.cols;

    onorth = outcellhd.north;
    osouth = outcellhd.south;
    oeast = outcellhd.east;
    owest = outcellhd.west;
    orows = outcellhd.rows;
    ocols = outcellhd.cols;

    if (print_bounds->answer) {
        G_JSON_Value *root_value = NULL;
        G_JSON_Object *root_object = NULL;

        G_message(_("Input map <%s@%s> in project <%s>:"), inmap->answer,
                  setname, inlocation->answer);

        /* reproject input raster extents from input to output */
        G_set_window(&incellhd);

        G_debug(1, "input window north: %.8f", incellhd.north);
        G_debug(1, "input window south: %.8f", incellhd.south);
        G_debug(1, "input window east: %.8f", incellhd.east);
        G_debug(1, "input window west: %.8f", incellhd.west);

        if (GPJ_init_transform(&iproj, &oproj, &tproj) < 0)
            G_fatal_error(_("Unable to initialize coordinate transformation"));

        outcellhd.north = -1e9;
        outcellhd.south = 1e9;
        outcellhd.east = -1e9;
        outcellhd.west = 1e9;
        bordwalk_edge(&incellhd, &outcellhd, &iproj, &oproj, &tproj, PJ_FWD);
        inorth = outcellhd.north;
        isouth = outcellhd.south;
        ieast = outcellhd.east;
        iwest = outcellhd.west;

        G_format_northing(inorth, north_str, curr_proj);
        G_format_northing(isouth, south_str, curr_proj);
        G_format_easting(ieast, east_str, curr_proj);
        G_format_easting(iwest, west_str, curr_proj);

        if (outputFormat == JSON) {
            root_value = G_json_value_init_object();
            if (root_value == NULL) {
                G_fatal_error(
                    _("Failed to initialize JSON object. Out of memory?"));
            }
            root_object = G_json_object(root_value);
        }

        switch (outputFormat) {
        case PLAIN:
            fprintf(stdout, "Source cols: %d\n", icols);
            fprintf(stdout, "Source rows: %d\n", irows);
            fprintf(stdout, "Local north: %s\n", north_str);
            fprintf(stdout, "Local south: %s\n", south_str);
            fprintf(stdout, "Local west: %s\n", west_str);
            fprintf(stdout, "Local east: %s\n", east_str);
            break;

        case SHELL:
            fprintf(stdout, "n=%s s=%s w=%s e=%s rows=%d cols=%d\n", north_str,
                    south_str, west_str, east_str, irows, icols);
            break;

        case JSON:
            if (isfinite(inorth)) {
                G_json_object_set_number(root_object, "north", inorth);
            }
            else {
                G_json_object_set_null(root_object, "north");
            }

            if (isfinite(isouth)) {
                G_json_object_set_number(root_object, "south", isouth);
            }
            else {
                G_json_object_set_null(root_object, "south");
            }

            if (isfinite(iwest)) {
                G_json_object_set_number(root_object, "west", iwest);
            }
            else {
                G_json_object_set_null(root_object, "west");
            }

            if (isfinite(ieast)) {
                G_json_object_set_number(root_object, "east", ieast);
            }
            else {
                G_json_object_set_null(root_object, "east");
            }

            G_json_object_set_number(root_object, "rows", irows);
            G_json_object_set_number(root_object, "cols", icols);
            break;
        }

        if (outputFormat == JSON) {
            char *serialized_string = NULL;
            serialized_string = G_json_serialize_to_string_pretty(root_value);
            if (serialized_string == NULL) {
                G_fatal_error(_("Failed to initialize pretty JSON string."));
            }
            puts(serialized_string);
            G_json_free_serialized_string(serialized_string);
            G_json_value_free(root_value);
        }

        exit(EXIT_SUCCESS);
    }

    /* Cut non-overlapping parts of input map */
    if (!nocrop->answer) {
        /* reproject current region from output to input */
        /* switch back to current location,
         * initialize transformation pipeline */
        G_switch_env();
        G_unset_window();
        G_set_window(&outcellhd);
        tproj.def = NULL;
        tproj.pj = NULL;
        if (pipeline->answer) {
            tproj.def = G_store(pipeline->answer);
        }
        if (GPJ_init_transform(&oproj, &iproj, &tproj) < 0)
            G_fatal_error(_("Unable to initialize coordinate transformation"));

        /* switch to input location */
        G_switch_env();

        /* update cellhead of input map */
        bordwalk(&outcellhd, &incellhd, &oproj, &iproj, &tproj, PJ_FWD);
    }

    /* Add 2 cells on each side for bilinear/cubic & future interpolation
     * methods */
    /* (should probably be a factor based on input and output resolution) */
    incellhd.north += 2 * incellhd.ns_res;
    incellhd.east += 2 * incellhd.ew_res;
    incellhd.south -= 2 * incellhd.ns_res;
    incellhd.west -= 2 * incellhd.ew_res;
    if (incellhd.north > inorth)
        incellhd.north = inorth;
    if (incellhd.east > ieast)
        incellhd.east = ieast;
    if (incellhd.south < isouth)
        incellhd.south = isouth;
    if (incellhd.west < iwest)
        incellhd.west = iwest;

    /* switch to current location */

    G_switch_env();

    /* Adjust borders of output map */

    if (!nocrop->answer) {
        /* reproject from input to output */
        /* switch input location,
         * initialize transformation pipeline */
        G_switch_env();
        G_unset_window();
        G_set_window(&incellhd);
        tproj.def = NULL;
        tproj.pj = NULL;
        if (pipeline->answer) {
            tproj.def = G_store(pipeline->answer);
        }
        if (GPJ_init_transform(&iproj, &oproj, &tproj) < 0)
            G_fatal_error(_("Unable to initialize coordinate transformation"));

        /* switch to output location */
        G_switch_env();

        /* reduce output region */
        bordwalk(&incellhd, &outcellhd, &iproj, &oproj, &tproj, PJ_FWD);
    }

#if 0
    outcellhd.west = outcellhd.south = HUGE_VAL;
    outcellhd.east = outcellhd.north = -HUGE_VAL;
    for (row = 0; row < incellhd.rows; row++) {
        ycoord1 = Rast_row_to_northing((double)(row + 0.5), &incellhd);
        for (col = 0; col < incellhd.cols; col++) {
            xcoord1 = Rast_col_to_easting((double)(col + 0.5), &incellhd);
            if (GPJ_transform(&iproj, &oproj, &tproj, PJ_FWD,
                              &xcoord1, &ycoord1, NULL) < 0)
                G_fatal_error(_("Error in %s"), "GPJ_transform()");
            if (xcoord1 > outcellhd.east)
                outcellhd.east = xcoord1;
            if (ycoord1 > outcellhd.north)
                outcellhd.north = ycoord1;
            if (xcoord1 < outcellhd.west)
                outcellhd.west = xcoord1;
            if (ycoord1 < outcellhd.south)
                outcellhd.south = ycoord1;
        }
    }
#endif

    if (res->answer != NULL) /* set user defined resolution */
        outcellhd.ns_res = outcellhd.ew_res = atof(res->answer);

    G_adjust_Cell_head(&outcellhd, 0, 0);
    Rast_set_output_window(&outcellhd);

    G_message(" ");
    G_message(_("Input:"));
    G_message(_("Cols: %d (original: %d)"), incellhd.cols, icols);
    G_message(_("Rows: %d (original: %d)"), incellhd.rows, irows);
    G_message(_("North: %f (original: %f)"), incellhd.north, inorth);
    G_message(_("South: %f (original: %f)"), incellhd.south, isouth);
    G_message(_("West: %f (original: %f)"), incellhd.west, iwest);
    G_message(_("East: %f (original: %f)"), incellhd.east, ieast);
    G_message(_("EW-res: %f"), incellhd.ew_res);
    G_message(_("NS-res: %f"), incellhd.ns_res);
    G_message(" ");

    G_message(_("Output:"));
    G_message(_("Cols: %d (original: %d)"), outcellhd.cols, ocols);
    G_message(_("Rows: %d (original: %d)"), outcellhd.rows, orows);
    G_message(_("North: %f (original: %f)"), outcellhd.north, onorth);
    G_message(_("South: %f (original: %f)"), outcellhd.south, osouth);
    G_message(_("West: %f (original: %f)"), outcellhd.west, owest);
    G_message(_("East: %f (original: %f)"), outcellhd.east, oeast);
    G_message(_("EW-res: %f"), outcellhd.ew_res);
    G_message(_("NS-res: %f"), outcellhd.ns_res);
    G_message(" ");

    /* Open the input map (input location env). Banding loads only per-band
     * input strips, not the whole map, so fdi stays open across the band loop.
     */
    G_switch_env();
    Rast_set_input_window(&incellhd);
    fdi = Rast_open_old(inmap->answer, setname);
    cell_type = Rast_get_map_type(fdi);
    if (strcmp(interpol->answer, "nearest") != 0)
        cell_type = FCELL_TYPE;
    cell_size = Rast_cell_size(cell_type);

    /* Parallel input reads: decide the read-thread count here, in the INPUT
     * env, so the mask guard checks the source mapset's mask (the mask that
     * would apply to Rast_get_row on fdi). Rast_disable_omp_on_mask returns 1
     * (serial) if a mask is present or without OpenMP, and does NOT touch the
     * thread count when no mask exists (lib/raster/mask_info.c:226-231), so the
     * compute region's threads are unperturbed in the common case. When
     * read_nprocs > 1 we open that many FRESH read fds (one per thread); fdi is
     * used only by the serial fallback.
     * INFERRED-safe (not yet runtime-verified; the gate converts it):
     * concurrent Rast_open_old fds on the same map across locations is sound
     * from the r.neighbors same-location precedent (in_fd[t]) plus the Stage 1
     * fcb analysis (each fd carries its own cur_row/data/data_fd; reads depend
     * only on the fcb and R__.rd_window). */
#ifdef _OPENMP
    int want_nprocs = omp_get_max_threads();
#else
    int want_nprocs = 1;
#endif
    int read_nprocs = Rast_disable_omp_on_mask(want_nprocs);
    int *fd_read = NULL;
    if (read_nprocs > 1) {
        fd_read = G_malloc((size_t)read_nprocs * sizeof(int));
        for (int t = 0; t < read_nprocs; t++)
            fd_read[t] = Rast_open_old(inmap->answer, setname);
    }

    /* Back to the output location: set output window, init transform, open
     * output map. Both fds now stay open; rd_window/wr_window are set and
     * survive env switches, so reads/writes use the right windows throughout.
     */
    G_switch_env();
    Rast_set_output_window(&outcellhd);
    G_unset_window();
    G_set_window(&outcellhd);
    tproj.def = NULL;
    tproj.pj = NULL;
    if (pipeline->answer) {
        tproj.def = G_store(pipeline->answer);
    }
    if (GPJ_init_transform(&oproj, &iproj, &tproj) < 0)
        G_fatal_error(_("Unable to initialize coordinate transformation"));

    if (strcmp(interpol->answer, "nearest") == 0)
        fdo = Rast_open_new(mapname, cell_type);
    else
        fdo = Rast_open_fp_new(mapname);

    /* Banding (r.neighbors two-level structure): outer serial band loop ->
     * serial strip load -> parallel compute into a per-band buffer -> serial
     * in-order per-band write -> next band. Bounds peak memory by the cap
     * instead of the whole input map (Path A). */
    double cap_mb = atof(memory->answer);
    size_t cap_bytes = (size_t)(cap_mb * 1024.0 * 1024.0);
    double t_size = 0.0, t_fill = 0.0, t_compute = 0.0, t_write = 0.0;
    int n_bands = 0;
    int max_tiles = 1;          /* most column tiles used by any single band */
    int seed_h = 0, seed_w = 0; /* previous Phase-2 band's accepted sizing */
    int seed_hits = 0, phase2_bands = 0; /* seed hit rate on the Phase-2 path */
    int seed_h1 = 0;               /* previous Phase-1 band's accepted height */
    int p1_hits = 0, p1_bands = 0; /* seed hit rate on the Phase-1 path */

    /* Output-row center northings, precomputed once by the serial version's
     * recurrence: ycoord2 = north - ns_res/2, then ycoord2 -= ns_res per row.
     * The banded fill loop and the strip-sizing perimeter walk both read these
     * instead of computing north - ns_res/2 - row*ns_res directly. The direct
     * multiply and the accumulated subtraction differ by up to one ULP when
     * ns_res is not exactly representable; for non-nearest interpolation that
     * shifts the sampling weights and diverges from the serial result by up to
     * one FCELL ULP. The recurrence is reproduced here deliberately
     * (bug-compatible rounding) so the parallel output stays bitwise identical
     * to the serial reference; the direct multiply is the numerically cleaner
     * form, so any future change away from the recurrence should be made in
     * both code paths as an explicit accuracy decision. Both the fill loop and
     * the sizing walk read these values, so sizing and fill stay on the same y
     * and the loaded strip covers exactly the rows fill probes. */
    double *y_center = G_malloc((size_t)outcellhd.rows * sizeof(double));
    {
        double yc = outcellhd.north - (outcellhd.ns_res / 2);
        for (int r = 0; r < outcellhd.rows; r++) {
            y_center[r] = yc;
            yc -= outcellhd.ns_res;
        }
    }

    /* Pole footprint fix: a tile whose interior projects onto a geographic pole
     * has an input-row extremum the perimeter walk misses. This happens both
     * when the pole lies inside the input map and when the pole is outside the
     * input's latitude coverage but its projection still falls inside the
     * output frame (a pole-centered frame reading an input truncated below the
     * pole): the highest reachable input latitude is then the input's own edge
     * row, reached at the frame-center-proximal interior. So project both poles
     * (lat/lon input only, where a pole is at latitude +/- 90) and fold in the
     * pole's input row clamped to the input's edge row [0, rows-1]. The
     * point-in-rect test in band_input_row_span keeps this a no-op for frames
     * that do not image a pole. On transform failure or a non-finite result
     * (e.g. a cylindrical projection sending the pole to infinity) the pole is
     * skipped and the strip under-size guard stays the backstop. Uses the
     * adjusted incellhd, matching what band_input_row_span sees. */
    struct pole_set poles;

    poles.n = 0;
    if (incellhd.proj == PROJECTION_LL) {
        double polelat[2] = {90.0, -90.0};

        for (int p = 0; p < 2; p++) {
            double px = 0.0, py = polelat[p];

            if (GPJ_transform(&oproj, &iproj, &tproj, PJ_INV, &px, &py, NULL) <
                    0 ||
                !isfinite(px) || !isfinite(py))
                continue;
            double ri = (incellhd.north - polelat[p]) / incellhd.ns_res;
            if (ri < 0)
                ri = 0;
            else if (ri > incellhd.rows - 1)
                ri = incellhd.rows - 1;
            poles.ox[poles.n] = px;
            poles.oy[poles.n] = py;
            poles.ri[poles.n] = ri;
            poles.n++;
        }
    }

    G_important_message(_("Projecting (banded, per-thread PROJ context)..."));

    int used_fallback = 0; /* set when the serial tile-cache fallback runs */
    int force_tilecache = getenv("R_PROJ_FORCE_TILECACHE") != NULL;
    /* Rolling-window input residency (Anna review item 1): keep one band's
     * input strip resident and slide it down between consecutive single-tile
     * bands, reading only the rows a band adds rather than re-reading its whole
     * [imin,imax]. win holds input rows [win_imin, win_imax]; win_imax <
     * win_imin marks the window empty/invalid (forces a full read). win_cap is
     * the allocated byte size. Freed at fallback_done and after the band loop.
     */
    unsigned char *win = NULL;
    size_t win_cap = 0;
    int win_imin = 0, win_imax = -1;
    int obr0 = 0;
    while (obr0 < outcellhd.rows) {
        /* Fit search. Phase 1 (fast path, unchanged): halve the band height
         * until the FULL-WIDTH strip plus the band output buffer fit the cap;
         * the span is re-run per candidate height. Phase 2 (oblique fallback):
         * only if a single full-width output row still busts the cap, split the
         * row into column tiles and halve tile WIDTH until the worst tile's
         * strip fits. Strips are full input width (the raster API reads whole
         * rows), so width splitting shrinks a tile's input ROW span, not its
         * width. Easy pairs never leave Phase 1. */
        double ts = rproj_wtime();
        /* Band-0 early-out for the wide-input corner: if a single output row at
         * the finest tiling already busts the cap, take the serial fallback now
         * instead of running the height/width search only to bail. Uses the
         * same worst_tile_strip_rows(obr0, obr0+1, 1) the Phase-2 bail uses,
         * probed only at the first band so its O(cols) cost is paid once, not
         * per band. Later-band (pole) busts still fall through to the Phase-2
         * bail. force_tilecache is deliberately not handled here, so the forced
         * override keeps routing through that bail unchanged. */
        if (obr0 == 0) {
            size_t out1 = (size_t)outcellhd.cols * cell_size;
            int worst1 = worst_tile_strip_rows(&outcellhd, &incellhd, &oproj,
                                               &iproj, &tproj, y_center, obr0,
                                               obr0 + 1, 1, &poles);
            size_t strip1 =
                worst1 > 0 ? (size_t)worst1 * incellhd.cols * cell_size : 0;
            if (strip1 + out1 > cap_bytes) {
                int needed_mb =
                    (int)ceil((double)(strip1 + out1) / (1024.0 * 1024.0)) + 1;
                G_warning(_("Memory cap (%.1f MB) is below what one output row "
                            "needs (input footprint %d rows, %.1f MB). Falling "
                            "back to the serial tile-cache path for output "
                            "rows %d-%d; this path is slower. Raise memory= to "
                            "at least %d MB to use the parallel path."),
                          cap_mb, worst1,
                          (double)(strip1 + out1) / (1024.0 * 1024.0), obr0,
                          outcellhd.rows - 1, needed_mb);
                fallback_serial_cache(fdi, fdo, cell_type, method, &oproj,
                                      &iproj, &tproj, &incellhd, &outcellhd,
                                      y_center, obr0, memory->answer);
                used_fallback = 1;
                goto fallback_done;
            }
        }
        int tilew = outcellhd.cols;
        int imin = 0, imax = -1;
        /* Phase-1 neighbor seed (hit path): seed_h1 (previous Phase-1 accepted
         * height) is close to this band's. Take g_seed, the grid height just
         * ABOVE seed_h1 on this band's descending lattice; if it does not fit
         * then (height-monotone span) nothing taller fits, so the tallest
         * fitting height is at or below (g_seed+1)/2 and the walk can start
         * there, skipping the tall full-width edge walks. Any miss (no seed,
         * seed_h1 too tall, or g_seed fits) starts from the full remaining
         * height -- byte-for-byte the walk below. Same lattice, same acceptance
         * line -> identical accepted height and partition; the hit only skips
         * heights it has shown cannot fit. */
        int band_orows = outcellhd.rows - obr0;
        int p1_seeded = 0;
        if (seed_h1 > 0 && seed_h1 < band_orows) {
            int gs = band_orows;

            while ((gs + 1) / 2 > seed_h1)
                gs = (gs + 1) / 2;
            if (!phase1_fits(&outcellhd, &incellhd, &oproj, &iproj, &tproj,
                             y_center, obr0, gs, cap_bytes, cell_size,
                             &poles)) {
                band_orows = (gs + 1) / 2;
                p1_seeded = 1;
            }
        }
        for (;;) {
            band_input_row_span(&outcellhd, &incellhd, &oproj, &iproj, &tproj,
                                y_center, obr0, obr0 + band_orows, 0,
                                outcellhd.cols, &imin, &imax, &poles, NULL);
            int strip_rows = imax - imin + 1;
            size_t strip_bytes =
                strip_rows > 0 ? (size_t)strip_rows * incellhd.cols * cell_size
                               : 0;
            size_t out_bytes = (size_t)band_orows * outcellhd.cols * cell_size;
            if (!force_tilecache && strip_bytes + out_bytes <= cap_bytes)
                break;
            if (band_orows == 1)
                break; /* height exhausted: fall through to column splitting */
            band_orows = (band_orows + 1) / 2; /* halve (round up), re-sample */
        }
        if (band_orows > 1) { /* Phase-1 accepted a full-width band */
            seed_h1 = band_orows;
            p1_bands++;
            if (p1_seeded)
                p1_hits++;
        }
        if (band_orows == 1) {
            /* Phase 2 (oblique only): find the tallest band height on the
             * descending grid whose worst column tile fits the cap, then that
             * height's widest fitting tile width. Neighbor seed (hit path): the
             * previous Phase-2 band's height (seed_h) is close to this band's
             * H*. Take g_seed, the grid height just ABOVE seed_h; if it does
             * not fit then (for an input-row span monotone in band height)
             * nothing taller fits, so H* is at or below g_seed and the walk can
             * start there, skipping the tall no-fit heights. On a miss (no
             * seed, seed_h too tall, or g_seed fits) start from the full
             * remaining height -- byte-for-byte the unseeded walk. Both starts
             * lie on the same grid and accept via the same phase2_width_fit, so
             * H*, W* and the partition are identical; the hit path only skips
             * heights it has shown cannot fit. */
            phase2_bands++;
            int start_h = outcellhd.rows - obr0;
            if (seed_w > 0 && seed_h < start_h) {
                int gs = start_h, w;

                while ((gs + 1) / 2 > seed_h)
                    gs = (gs + 1) / 2;
                if (!phase2_width_fit(&outcellhd, &incellhd, &oproj, &iproj,
                                      &tproj, y_center, obr0, gs, cap_bytes,
                                      cell_size, &w, &poles)) {
                    start_h = (gs + 1) / 2;
                    seed_hits++;
                }
            }
            band_orows = start_h;
            for (;;) {
                if (!force_tilecache &&
                    phase2_width_fit(&outcellhd, &incellhd, &oproj, &iproj,
                                     &tproj, y_center, obr0, band_orows,
                                     cap_bytes, cell_size, &tilew, &poles))
                    break;
                if (band_orows == 1) {
                    /* Single output row at minimum width still over cap =
                     * singular/large-halo; take the serial tile-cache path.
                     * Also reached from band 0 when R_PROJ_FORCE_TILECACHE is
                     * set, which routes normal data through this identical
                     * block for testing. */
                    if (force_tilecache) {
                        G_warning(
                            _("R_PROJ_FORCE_TILECACHE is set: taking the "
                              "serial tile-cache path for all output rows "
                              "(testing override)."));
                    }
                    else {
                        size_t out1 = (size_t)outcellhd.cols * cell_size;
                        int worst = worst_tile_strip_rows(
                            &outcellhd, &incellhd, &oproj, &iproj, &tproj,
                            y_center, obr0, obr0 + 1, 1, &poles);
                        size_t strip_bytes =
                            worst > 0
                                ? (size_t)worst * incellhd.cols * cell_size
                                : 0;
                        int needed_mb = (int)ceil((double)(strip_bytes + out1) /
                                                  (1024.0 * 1024.0)) +
                                        1;
                        G_warning(
                            _("Memory cap (%.1f MB) is below what one output "
                              "row needs (input footprint %d rows, %.1f MB). "
                              "Falling back to the serial tile-cache path for "
                              "output rows %d-%d; this path is slower. Raise "
                              "memory= to at least %d MB to use the parallel "
                              "path."),
                            cap_mb, worst,
                            (double)(strip_bytes + out1) / (1024.0 * 1024.0),
                            obr0, outcellhd.rows - 1, needed_mb);
                    }
                    fallback_serial_cache(fdi, fdo, cell_type, method, &oproj,
                                          &iproj, &tproj, &incellhd, &outcellhd,
                                          y_center, obr0, memory->answer);
                    used_fallback = 1;
                    goto fallback_done;
                }
                band_orows = (band_orows + 1) / 2;
            }
            seed_h = band_orows;
            seed_w = tilew;
        }
        t_size += rproj_wtime() - ts;

        int obr1 = obr0 + band_orows;
        n_bands++;
        int n_tiles = (outcellhd.cols + tilew - 1) / tilew;
        if (n_tiles > max_tiles)
            max_tiles = n_tiles;

        /* Per-band output buffer, lock-free disjoint row slots, filled column
         * tile by column tile and written once after all tiles. Full width
         * regardless of tiling. */
        void *band_out =
            G_malloc((size_t)band_orows * outcellhd.cols * cell_size);

        /* Column tiles processed one at a time: only the current tile's strip
         * is resident, so peak strip memory is the worst tile, not the band's
         * union. tilew == cols is the single-tile fast path (obc0=0,
         * obc1=cols), identical to un-tiled banding. */
        for (int obc0 = 0; obc0 < outcellhd.cols; obc0 += tilew) {
            int obc1 = obc0 + tilew;
            if (obc1 > outcellhd.cols)
                obc1 = outcellhd.cols;

            /* Per-tile input row span (full-width strip: the raster API reads
             * whole rows, so columns are not cropped). */
            int pole_widened = 0;

            band_input_row_span(&outcellhd, &incellhd, &oproj, &iproj, &tproj,
                                y_center, obr0, obr1, obc0, obc1, &imin, &imax,
                                &poles, &pole_widened);
            if (pole_widened)
                G_verbose_message(
                    _("Pole (input row %d) in output tile rows [%d, %d) cols "
                      "[%d, %d): input strip extended to reach it"),
                    (int)poles.ri[pole_widened - 1], obr0, obr1, obc0, obc1);
            int strip_rows = imax - imin + 1;

            /* Serial strip load (single fd -> get_row not thread-safe). EMPTY
             * TILE: strip_rows <= 0 -> projects outside input, no read; cells
             * become NULL via interpolate_strip's out-of-map path, and the
             * window is invalidated so the next band re-reads in full. */
            void *strip = NULL;
            if (strip_rows > 0) {
                size_t need = (size_t)strip_rows * incellhd.cols * cell_size;
                size_t row_bytes = (size_t)incellhd.cols * cell_size;
                /* Slide only for a serial-read, single-tile band whose window
                 * is valid and whose rows advance forward and still overlap the
                 * new span. read_nprocs == 1 is required: the parallel read
                 * chunks a full [imin, imax] span across per-thread fds, which
                 * this change does not re-certify for a partial tail (N>1 stays
                 * a full read). Tiled bands (spans jump per tile) and backward
                 * steps or gaps (pole/inverted frames) fall back to a full
                 * read, exactly today's behavior. */
                int can_slide = read_nprocs == 1 && n_tiles == 1 &&
                                win_imax >= 0 && imin >= win_imin &&
                                imin <= win_imax + 1;
                int read_from = imin;
                /* Grow FIRST, then memmove, then read the tail. G_realloc may
                 * move the buffer, so growing must precede the memmove that
                 * repositions the retained overlap inside it; realloc preserves
                 * the old rows at their old offsets, which the memmove then
                 * shifts to the new imin origin. When the span SHRINKS
                 * (imax < win_imax) the resident rows beyond imax are dropped
                 * from the window's accounting below, not kept -- a deliberate,
                 * conservative choice: a later band that needs them re-reads,
                 * and the window never claims rows it is not tracking. */
                if (need > win_cap) {
                    win = G_realloc(win, need);
                    win_cap = need;
                }
                if (can_slide && win_imax >= imin) {
                    memmove(win, win + (size_t)(imin - win_imin) * row_bytes,
                            (size_t)(win_imax - imin + 1) * row_bytes);
                    read_from = win_imax + 1; /* only new rows hit disk */
                }
                strip = win;
                double t0 = rproj_wtime();
                if (read_from <= imax) {
                    G_switch_env(); /* -> input */
                    if (read_nprocs > 1) {
#ifdef _OPENMP
                        /* Parallel read (full-span only; can_slide is false
                         * here): each thread reads a contiguous, disjoint block
                         * of rows through its OWN fd into its own disjoint
                         * strip slice. No two threads share an fd/row. */
#pragma omp parallel num_threads(read_nprocs)
                        {
                            int t = omp_get_thread_num();
#pragma omp for schedule(static)
                            for (int r = read_from; r <= imax; r++)
                                Rast_get_row(fd_read[t],
                                             (unsigned char *)strip +
                                                 (size_t)(r - imin) *
                                                     incellhd.cols * cell_size,
                                             r, cell_type);
                        }
#endif
                    }
                    else {
                        /* Serial read of the (possibly partial) tail. */
                        for (int r = read_from; r <= imax; r++)
                            Rast_get_row(fdi,
                                         (unsigned char *)strip +
                                             (size_t)(r - imin) *
                                                 incellhd.cols * cell_size,
                                         r, cell_type);
                    }
                    G_switch_env(); /* -> output */
                }
                t_fill += rproj_wtime() - t0;
                /* Record what the window now tracks. A tiled band leaves win
                 * holding only its last tile, so invalidate BOTH fields to
                 * force the next band's full read; validity then never depends
                 * on && short-circuit order. */
                if (n_tiles == 1) {
                    win_imin = imin;
                    win_imax = imax;
                }
                else {
                    win_imin = 0;
                    win_imax = -1;
                }
            }
            else {
                win_imin = 0;
                win_imax = -1; /* empty tile: nothing resident */
            }

            double t1 = rproj_wtime();
            /* One parallel region per tile. Not nested: the "omp for" divides
             * the band's output rows among this region's threads. Separate
             * directives so each thread clones its PROJ context before the row
             * loop and destroys it after. */
#pragma omp parallel
            {
                struct gpj_transform_clone tproj_local;
                GPJ_clone_transform(&tproj, &tproj_local);

#pragma omp for private(row, col) schedule(dynamic)
                for (row = obr0; row < obr1; row++) {
                    void *out_row =
                        (unsigned char *)band_out +
                        (size_t)(row - obr0) * outcellhd.cols * cell_size;
                    double local_y = y_center[row];
                    double local_x_start =
                        outcellhd.west + (outcellhd.ew_res / 2);

                    for (col = obc0; col < obc1; col++) {
                        void *obufptr =
                            (unsigned char *)out_row + (size_t)col * cell_size;
                        double x1 = local_x_start + (col * outcellhd.ew_res);
                        double y1 = local_y;

                        if (GPJ_transform(&oproj, &iproj, &tproj_local.info,
                                          PJ_FWD, &x1, &y1, NULL) < 0) {
                            Rast_set_null_value(obufptr, 1, cell_type);
                        }
                        else {
                            double c_idx =
                                (x1 - incellhd.west) / incellhd.ew_res;
                            double r_idx =
                                (incellhd.north - y1) / incellhd.ns_res;
                            interp(strip, obufptr, cell_type, c_idx, r_idx,
                                   &incellhd, imin, imax);
                        }
                    }
                }

                GPJ_free_transform_clone(&tproj_local);
            }
            t_compute += rproj_wtime() - t1;
            /* strip aliases the persistent window buffer (win); it is not freed
             * per tile -- freed once at fallback_done and after the band loop.
             */
        }

        /* Serial in-order write of the band's rows once all tiles filled
         * band_out (Rast_put_row sequential). */
        double t2 = rproj_wtime();
        for (row = obr0; row < obr1; row++)
            Rast_put_row(fdo,
                         (unsigned char *)band_out +
                             (size_t)(row - obr0) * outcellhd.cols * cell_size,
                         cell_type);
        t_write += rproj_wtime() - t2;

        G_percent(obr1, outcellhd.rows, 5);

        G_free(band_out);
        obr0 = obr1;
    }

fallback_done:
    G_free(y_center);
    /* Single free site for the rolling window: the band loop's only exits are
     * normal completion (falls through to here) and the two goto fallback_done
     * bails (band-0 early-out, Phase-2 width bust), all converging on this
     * label, so one free covers every path crossing the window's live range.
     * win is NULL if a bail fired before any band allocated it. */
    if (win)
        G_free(win);

    if (used_fallback)
        G_debug(1, "PHASE_TIMERS fallback=1 fallback_from_row=%d", obr0);
    else
        G_debug(1,
                "PHASE_TIMERS size=%.4f fill=%.4f compute=%.4f write=%.4f "
                "bands=%d tiles=%d seed_hits=%d phase2_bands=%d p1_hits=%d "
                "p1_bands=%d",
                t_size, t_fill, t_compute, t_write, n_bands, max_tiles,
                seed_hits, phase2_bands, p1_hits, p1_bands);

    /* Close input map in its own env, then the output map. */
    G_switch_env(); /* -> input */
    Rast_close(fdi);
    if (fd_read) {
        for (int t = 0; t < read_nprocs; t++)
            Rast_close(fd_read[t]);
        G_free(fd_read);
    }
    G_switch_env(); /* -> output */
    Rast_close(fdo);

    if (have_colors > 0) {
        Rast_write_colors(mapname, G_mapset(), &colr);
        Rast_free_colors(&colr);
    }

    Rast_short_history(mapname, "raster", &history);
    Rast_command_history(&history);
    Rast_write_history(mapname, &history);

    G_done_msg(" ");
    exit(EXIT_SUCCESS);
}

char *make_ipol_list(void)
{
    int size = 0;
    int i;
    char *buf;

    for (i = 0; menu[i].name; i++)
        size += strlen(menu[i].name) + 1;

    buf = G_malloc(size);
    *buf = '\0';

    for (i = 0; menu[i].name; i++) {
        if (i)
            strcat(buf, ",");
        strcat(buf, menu[i].name);
    }

    return buf;
}

char *make_ipol_desc(void)
{
    int size = 0;
    int i;
    char *buf;

    for (i = 0; menu[i].name; i++)
        size += strlen(menu[i].name) + strlen(menu[i].text) + 2;

    buf = G_malloc(size);
    *buf = '\0';

    for (i = 0; menu[i].name; i++) {
        if (i)
            strcat(buf, ";");
        strcat(buf, menu[i].name);
        strcat(buf, ";");
        strcat(buf, menu[i].text);
    }

    return buf;
}
