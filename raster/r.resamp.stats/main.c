/****************************************************************************
 *
 * MODULE:       r.resamp.stats
 * AUTHOR(S):    Glynn Clements <glynn gclements.plus.com> (original
 *               contributor)
 *               Hamish Bowman <hamish_b yahoo.com>
 *               Chopra (OpenMP parallelization)
 * PURPOSE:
 * COPYRIGHT:    (C) 2006-2007 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 ****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <math.h>

#if defined(_OPENMP)
#include <omp.h>
#endif

#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/raster.h>
#include <grass/stats.h>

static const struct menu {
    stat_func *method;     /* routine to compute new value */
    stat_func_w *method_w; /* routine to compute new value (weighted) */
    char *name;            /* method name */
    char *text;            /* menu display - full description */
} menu[] = {{c_ave, w_ave, "average", "average (mean) value"},
            {c_median, w_median, "median", "median value"},
            {c_mode, w_mode, "mode", "most frequently occurring value"},
            {c_min, w_min, "minimum", "lowest value"},
            {c_max, w_max, "maximum", "highest value"},
            {c_range, NULL, "range", "range value"},
            {c_quart1, w_quart1, "quart1", "first quartile"},
            {c_quart3, w_quart3, "quart3", "third quartile"},
            {c_perc90, w_perc90, "perc90", "ninetieth percentile"},
            {c_sum, w_sum, "sum", "sum of values"},
            {c_var, w_var, "variance", "variance value"},
            {c_stddev, w_stddev, "stddev", "standard deviation"},
            {c_quant, w_quant, "quantile", "arbitrary quantile"},
            {c_count, w_count, "count", "count of non-NULL values"},
            {c_divr, NULL, "diversity", "number of different values"},
            {NULL, NULL, NULL, NULL}};

static char *build_method_list(void)
{
    char *buf = G_malloc(1024);
    char *p = buf;
    int i;

    for (i = 0; menu[i].name; i++) {
        char *q;

        if (i)
            *p++ = ',';
        for (q = menu[i].name; *q; p++, q++)
            *p = *q;
    }
    *p = '\0';

    return buf;
}

static int find_method(const char *name)
{
    int i;

    for (i = 0; menu[i].name; i++)
        if (strcmp(menu[i].name, name) == 0)
            return i;

    return -1;
}

static int nulls;
static int outfile;
static struct Cell_head dst_w, src_w;
static int method;
static const void *closure;
static int row_scale, col_scale;
static double quantile;

/* ----- GLOBAL PARALLEL VARIABLES ----- */
static int nprocs;
static int memory_mb;
static int *in_fd;
/* ------------------------------------- */

static void resamp_unweighted(void)
{
    stat_func *method_fn;
    int *col_map, *row_map;
    int row, col;
    int i, t, k;
    int chunk_size;
    int row_start;
    DCELL *chunk_buf;
    DCELL **t_values;
    DCELL ***t_bufs;

    method_fn = menu[method].method;

    col_map = G_malloc((dst_w.cols + 1) * sizeof(int));
    row_map = G_malloc((dst_w.rows + 1) * sizeof(int));

    /* Pre-calculate coordinate maps (serial) */
    for (col = 0; col <= dst_w.cols; col++) {
        double x = Rast_col_to_easting(col, &dst_w);

        col_map[col] = (int)floor((x - src_w.west) / src_w.ew_res + 0.5);
    }
    for (row = 0; row <= dst_w.rows; row++) {
        double y = Rast_row_to_northing(row, &dst_w);

        row_map[row] = (int)floor(Rast_northing_to_row(y, &src_w) + 0.5);
    }

    /* Calculate chunk size based on memory_mb */
    {
        size_t row_size = dst_w.cols * sizeof(DCELL);
        size_t total_mem_bytes = (size_t)memory_mb * 1024 * 1024;

        if (row_size > 0)
            chunk_size = total_mem_bytes / row_size;
        else
            chunk_size = dst_w.rows;
    }
    if (chunk_size < nprocs)
        chunk_size = nprocs;
    if (chunk_size > dst_w.rows)
        chunk_size = dst_w.rows;

    G_message(_("Parallel processing: Using %d threads, chunk size %d rows"),
              nprocs, chunk_size);

    /* Allocate the output chunk buffer */
    chunk_buf = G_malloc((size_t)chunk_size * dst_w.cols * sizeof(DCELL));

    /* Pre-allocate per-thread buffers */
    t_values = G_malloc(nprocs * sizeof(DCELL *));
    t_bufs = G_malloc(nprocs * sizeof(DCELL **));
    for (t = 0; t < nprocs; t++) {
        t_values[t] = G_malloc(row_scale * col_scale * sizeof(DCELL));
        t_bufs[t] = G_malloc(row_scale * sizeof(DCELL *));
        for (k = 0; k < row_scale; k++)
            t_bufs[t][k] = Rast_allocate_d_input_buf();
    }

    /* Main chunking loop */
    for (row_start = 0; row_start < dst_w.rows; row_start += chunk_size) {
        int current_chunk_rows = chunk_size;

        if (row_start + current_chunk_rows > dst_w.rows)
            current_chunk_rows = dst_w.rows - row_start;

        G_percent(row_start, dst_w.rows, 2);

/* PARALLEL REGION */
#pragma omp parallel for private(row, col, i, t, k) schedule(static)
        for (i = 0; i < current_chunk_rows; i++) {
            int global_row = row_start + i;
            int maprow0, maprow1, count;
            int my_infile;
            int mapcol0, mapcol1;
            int null, n, r, c;
            DCELL *values;
            DCELL **local_bufs;
            DCELL *out_row_ptr;

#if defined(_OPENMP)
            t = omp_get_thread_num();
#else
            t = 0;
#endif

            values = t_values[t];
            local_bufs = t_bufs[t];
            my_infile = in_fd[t];

            maprow0 = row_map[global_row + 0];
            maprow1 = row_map[global_row + 1];
            count = maprow1 - maprow0;

            /* Read input rows into pre-allocated per-thread buffers */
            for (k = 0; k < count; k++)
                Rast_get_d_row(my_infile, local_bufs[k], maprow0 + k);

            out_row_ptr = &chunk_buf[(size_t)i * dst_w.cols];

            for (col = 0; col < dst_w.cols; col++) {
                mapcol0 = col_map[col + 0];
                mapcol1 = col_map[col + 1];
                null = 0;
                n = 0;

                for (r = 0; r < count; r++) {
                    for (c = mapcol0; c < mapcol1; c++) {
                        DCELL *src = &local_bufs[r][c];
                        DCELL *dst = &values[n++];

                        if (Rast_is_d_null_value(src)) {
                            Rast_set_d_null_value(dst, 1);
                            null = 1;
                        }
                        else {
                            *dst = *src;
                        }
                    }
                }

                if (null && nulls)
                    Rast_set_d_null_value(&out_row_ptr[col], 1);
                else
                    (*method_fn)(&out_row_ptr[col], values, n, closure);
            }
        } /* End parallel for */

        /* Sequential write to disk */
        for (i = 0; i < current_chunk_rows; i++)
            Rast_put_d_row(outfile, &chunk_buf[(size_t)i * dst_w.cols]);
    }

    /* Free per-thread buffers */
    for (t = 0; t < nprocs; t++) {
        for (k = 0; k < row_scale; k++)
            G_free(t_bufs[t][k]);
        G_free(t_bufs[t]);
        G_free(t_values[t]);
    }
    G_free(t_bufs);
    G_free(t_values);
    G_free(chunk_buf);
    G_free(row_map);
    G_free(col_map);
}

static void resamp_weighted(void)
{
    stat_func_w *method_fn;
    double *col_map, *row_map;
    int row, col;
    int i, t, k;
    int chunk_size;
    int row_start;
    DCELL *chunk_buf;
    DCELL(**t_values)[2];
    DCELL ***t_bufs;

    method_fn = menu[method].method_w;

    col_map = G_malloc((dst_w.cols + 1) * sizeof(double));
    row_map = G_malloc((dst_w.rows + 1) * sizeof(double));

    /* Pre-calculate coordinate maps (serial) */
    for (col = 0; col <= dst_w.cols; col++) {
        double x = Rast_col_to_easting(col, &dst_w);

        col_map[col] = (x - src_w.west) / src_w.ew_res;
    }
    for (row = 0; row <= dst_w.rows; row++) {
        double y = Rast_row_to_northing(row, &dst_w);

        row_map[row] = Rast_northing_to_row(y, &src_w);
    }

    /* Calculate chunk size */
    {
        size_t row_size = dst_w.cols * sizeof(DCELL);
        size_t total_mem_bytes = (size_t)memory_mb * 1024 * 1024;

        if (row_size > 0)
            chunk_size = total_mem_bytes / row_size;
        else
            chunk_size = dst_w.rows;
    }
    if (chunk_size < nprocs)
        chunk_size = nprocs;
    if (chunk_size > dst_w.rows)
        chunk_size = dst_w.rows;

    G_message(
        _("Weighted parallel processing: Using %d threads, chunk size %d rows"),
        nprocs, chunk_size);

    /* Allocate output buffer */
    chunk_buf = G_malloc((size_t)chunk_size * dst_w.cols * sizeof(DCELL));

    /* Pre-allocate per-thread buffers */
    t_values = G_malloc(nprocs * sizeof(DCELL(*)[2]));
    t_bufs = G_malloc(nprocs * sizeof(DCELL **));
    for (t = 0; t < nprocs; t++) {
        t_values[t] = G_malloc(row_scale * col_scale * 2 * sizeof(DCELL));
        t_bufs[t] = G_malloc(row_scale * sizeof(DCELL *));
        for (k = 0; k < row_scale; k++)
            t_bufs[t][k] = Rast_allocate_d_input_buf();
    }

    /* Main chunking loop */
    for (row_start = 0; row_start < dst_w.rows; row_start += chunk_size) {
        int current_chunk_rows = chunk_size;

        if (row_start + current_chunk_rows > dst_w.rows)
            current_chunk_rows = dst_w.rows - row_start;

        G_percent(row_start, dst_w.rows, 2);

/* PARALLEL REGION */
#pragma omp parallel for private(row, col, i, t, k) schedule(static)
        for (i = 0; i < current_chunk_rows; i++) {
            int global_row = row_start + i;
            double y0, y1;
            int maprow0, maprow1, count;
            int my_infile;
            int mapcol0, mapcol1;
            int null, n, r, c;
            double x0, x1;
            DCELL(*values)[2];
            DCELL **local_bufs;
            DCELL *out_row_ptr;

#if defined(_OPENMP)
            t = omp_get_thread_num();
#else
            t = 0;
#endif

            values = t_values[t];
            local_bufs = t_bufs[t];
            my_infile = in_fd[t];

            y0 = row_map[global_row + 0];
            y1 = row_map[global_row + 1];
            maprow0 = (int)floor(y0);
            maprow1 = (int)ceil(y1);
            count = maprow1 - maprow0;

            /* Read input rows into pre-allocated per-thread buffers */
            for (k = 0; k < count; k++)
                Rast_get_d_row(my_infile, local_bufs[k], maprow0 + k);

            out_row_ptr = &chunk_buf[(size_t)i * dst_w.cols];

            for (col = 0; col < dst_w.cols; col++) {
                x0 = col_map[col + 0];
                x1 = col_map[col + 1];
                mapcol0 = (int)floor(x0);
                mapcol1 = (int)ceil(x1);
                null = 0;
                n = 0;

                for (r = maprow0; r < maprow1; r++) {
                    /* Calculate Y overlap weight */
                    double ky = (r == maprow0)       ? 1 - (y0 - maprow0)
                                : (r == maprow1 - 1) ? 1 - (maprow1 - y1)
                                                     : 1;

                    for (c = mapcol0; c < mapcol1; c++) {
                        /* Calculate X overlap weight */
                        double kx = (c == mapcol0)       ? 1 - (x0 - mapcol0)
                                    : (c == mapcol1 - 1) ? 1 - (mapcol1 - x1)
                                                         : 1;

                        DCELL *src = &local_bufs[r - maprow0][c];
                        DCELL *dst = &values[n++][0];

                        if (Rast_is_d_null_value(src)) {
                            Rast_set_d_null_value(&dst[0], 1);
                            null = 1;
                        }
                        else {
                            dst[0] = *src;
                            dst[1] = kx * ky; /* Store weight */
                        }
                    }
                }

                if (null && nulls)
                    Rast_set_d_null_value(&out_row_ptr[col], 1);
                else
                    (*method_fn)(&out_row_ptr[col], values, n, closure);
            }
        } /* End parallel for */

        /* Sequential write to disk */
        for (i = 0; i < current_chunk_rows; i++)
            Rast_put_d_row(outfile, &chunk_buf[(size_t)i * dst_w.cols]);
    }

    /* Free per-thread buffers */
    for (t = 0; t < nprocs; t++) {
        for (k = 0; k < row_scale; k++)
            G_free(t_bufs[t][k]);
        G_free(t_bufs[t]);
        G_free(t_values[t]);
    }
    G_free(t_bufs);
    G_free(t_values);
    G_free(chunk_buf);
    G_free(row_map);
    G_free(col_map);
}

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct {
        struct Option *rastin, *rastout, *method, *quantile, *nprocs, *memory;
    } parm;
    struct {
        struct Flag *nulls, *weight;
    } flag;
    struct History history;
    char title[64];
    char buf_nsres[100], buf_ewres[100];
    struct Colors colors;
    int i;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("resample"));
    G_add_keyword(_("univariate statistics"));
    G_add_keyword(_("aggregation"));
    G_add_keyword(_("parallel"));
    module->description =
        _("Resamples raster map layers to a coarser grid using aggregation.");

    parm.rastin = G_define_standard_option(G_OPT_R_INPUT);

    parm.rastout = G_define_standard_option(G_OPT_R_OUTPUT);

    parm.method = G_define_option();
    parm.method->key = "method";
    parm.method->type = TYPE_STRING;
    parm.method->required = NO;
    parm.method->description = _("Aggregation method");
    parm.method->options = build_method_list();
    parm.method->answer = "average";

    parm.quantile = G_define_option();
    parm.quantile->key = "quantile";
    parm.quantile->type = TYPE_DOUBLE;
    parm.quantile->required = NO;
    parm.quantile->description = _("Quantile to calculate for method=quantile");
    parm.quantile->options = "0.0-1.0";
    parm.quantile->answer = "0.5";

    parm.nprocs = G_define_standard_option(G_OPT_M_NPROCS);
    parm.memory = G_define_standard_option(G_OPT_MEMORYMB);

    flag.nulls = G_define_flag();
    flag.nulls->key = 'n';
    flag.nulls->description = _("Propagate NULLs");

    flag.weight = G_define_flag();
    flag.weight->key = 'w';
    flag.weight->description = _("Weight according to area (slower)");

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    /* --- PARALLEL SETUP --- */
    nprocs = G_set_omp_num_threads(parm.nprocs);
    nprocs = Rast_disable_omp_on_mask(nprocs);
    if (nprocs < 1)
        G_fatal_error(_("<%d> is not valid number of nprocs."), nprocs);

    memory_mb = atoi(parm.memory->answer);
    if (memory_mb <= 0)
        memory_mb = 300;
    /* ---------------------------- */

    nulls = flag.nulls->answer;

    method = find_method(parm.method->answer);
    if (method < 0)
        G_fatal_error(_("Unknown method <%s>"), parm.method->answer);

    if (menu[method].method == c_quant) {
        quantile = atof(parm.quantile->answer);
        closure = &quantile;
    }

    G_get_set_window(&dst_w);

    /* set source window to old map */
    Rast_get_cellhd(parm.rastin->answer, "", &src_w);

    if (G_projection() == PROJECTION_LL) {
        /* try to shift source window to overlap with destination window */
        while (src_w.west >= dst_w.east && src_w.east - 360.0 > dst_w.west) {
            src_w.east -= 360.0;
            src_w.west -= 360.0;
        }
        while (src_w.east <= dst_w.west && src_w.west + 360.0 < dst_w.east) {
            src_w.east += 360.0;
            src_w.west += 360.0;
        }
    }

    /* adjust source window to cover destination window */
    {
        int r0 = (int)floor(Rast_northing_to_row(dst_w.north, &src_w));
        int r1 = (int)ceil(Rast_northing_to_row(dst_w.south, &src_w));

        /* do not use Rast_easting_to_col() because it does ll wrap */
        /*
           int c0 = (int)floor(Rast_easting_to_col(dst_w.west, &src_w));
           int c1 = (int)ceil(Rast_easting_to_col(dst_w.east, &src_w));
         */
        int c0 = (int)floor((dst_w.west - src_w.west) / src_w.ew_res);
        int c1 =
            src_w.cols + (int)ceil((dst_w.east - src_w.east) / src_w.ew_res);

        src_w.south -= src_w.ns_res * (r1 - src_w.rows);
        src_w.north += src_w.ns_res * (-r0);
        src_w.west -= src_w.ew_res * (-c0);
        src_w.east += src_w.ew_res * (c1 - src_w.cols);
        src_w.rows = r1 - r0;
        src_w.cols = c1 - c0;
    }

    Rast_set_input_window(&src_w);
    Rast_set_output_window(&dst_w);

    row_scale = 2 + ceil(dst_w.ns_res / src_w.ns_res);
    col_scale = 2 + ceil(dst_w.ew_res / src_w.ew_res);

    /* open old map (multiple times for thread safety) */
    in_fd = G_malloc(nprocs * sizeof(int));
    for (i = 0; i < nprocs; i++)
        in_fd[i] = Rast_open_old(parm.rastin->answer, "");

    /* open new map */
    outfile = Rast_open_new(parm.rastout->answer, DCELL_TYPE);

    if (flag.weight->answer && menu[method].method_w)
        resamp_weighted();
    else
        resamp_unweighted();

    G_percent(dst_w.rows, dst_w.rows, 2);

    /* Close all input file descriptors */
    for (i = 0; i < nprocs; i++)
        Rast_close(in_fd[i]);
    G_free(in_fd);
    Rast_close(outfile);

    /* record map metadata/history info */
    snprintf(title, sizeof(title), "Aggregate resample by %s",
             parm.method->answer);
    Rast_put_cell_title(parm.rastout->answer, title);

    Rast_short_history(parm.rastout->answer, "raster", &history);
    Rast_set_history(&history, HIST_DATSRC_1, parm.rastin->answer);
    G_format_resolution(src_w.ns_res, buf_nsres, src_w.proj);
    G_format_resolution(src_w.ew_res, buf_ewres, src_w.proj);
    Rast_format_history(&history, HIST_DATSRC_2,
                        "Source map NS res: %s   EW res: %s", buf_nsres,
                        buf_ewres);
    Rast_command_history(&history);
    Rast_write_history(parm.rastout->answer, &history);

    /* copy color table from source map */
    if (strcmp(parm.method->answer, "sum") != 0 &&
        strcmp(parm.method->answer, "range") != 0 &&
        strcmp(parm.method->answer, "count") != 0 &&
        strcmp(parm.method->answer, "diversity") != 0) {
        if (Rast_read_colors(parm.rastin->answer, "", &colors) < 0)
            G_fatal_error(_("Unable to read color table for %s"),
                          parm.rastin->answer);
        Rast_mark_colors_as_fp(&colors);
        Rast_write_colors(parm.rastout->answer, G_mapset(), &colors);
    }

    return (EXIT_SUCCESS);
}
