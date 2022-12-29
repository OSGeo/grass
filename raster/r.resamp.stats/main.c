
/****************************************************************************
 *
 * MODULE:       r.resamp.stats
 * AUTHOR(S):    Glynn Clements <glynn gclements.plus.com> (original contributor)
 *               Hamish Bowman <hamish_b yahoo.com>
 * PURPOSE:      
 * COPYRIGHT:    (C) 2006-2007 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include <grass/stats.h>


static const struct menu
{
    stat_func *method;		/* routine to compute new value */
    stat_func_w *method_w;	/* routine to compute new value (weighted) */
    char *name;			/* method name */
    char *text;			/* menu display - full description */
} menu[] = {
    {c_ave,    w_ave,    "average",  "average (mean) value"},
    {c_median, w_median, "median",   "median value"},
    {c_mode,   w_mode,   "mode",     "most frequently occurring value"},
    {c_min,    w_min,    "minimum",  "lowest value"},
    {c_max,    w_max,    "maximum",  "highest value"},
    {c_range,  NULL,     "range",    "range value"},
    {c_quart1, w_quart1, "quart1",   "first quartile"},
    {c_quart3, w_quart3, "quart3",   "third quartile"},
    {c_perc90, w_perc90, "perc90",   "ninetieth percentile"},
    {c_sum,    w_sum,    "sum",      "sum of values"},
    {c_var,    w_var,    "variance", "variance value"},
    {c_stddev, w_stddev, "stddev",   "standard deviation"},
    {c_quant,  w_quant,  "quantile", "arbitrary quantile"},
    {c_count,  w_count,  "count",    "count of non-NULL values"},
    {c_divr,   NULL,     "diversity", "number of different values"},
    {NULL, NULL, NULL, NULL}
};

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
static int infile, outfile;
static struct Cell_head dst_w, src_w;
static DCELL *outbuf;
static DCELL **bufs;
static int method;
static const void *closure;
static int row_scale, col_scale;
static double quantile;

static void resamp_unweighted(void)
{
    stat_func *method_fn;
    DCELL *values;
    int *col_map, *row_map;
    int row, col;

    method_fn = menu[method].method;

    values = G_malloc(row_scale * col_scale * sizeof(DCELL));

    col_map = G_malloc((dst_w.cols + 1) * sizeof(int));
    row_map = G_malloc((dst_w.rows + 1) * sizeof(int));

    for (col = 0; col <= dst_w.cols; col++) {
	double x = Rast_col_to_easting(col, &dst_w);

	/* col_map[col] = (int)floor(Rast_easting_to_col(x, &src_w) + 0.5); */
	col_map[col] = (int)floor((x - src_w.west) / src_w.ew_res + 0.5);
    }

    for (row = 0; row <= dst_w.rows; row++) {
	double y = Rast_row_to_northing(row, &dst_w);

	row_map[row] = (int)floor(Rast_northing_to_row(y, &src_w) + 0.5);
    }

    for (row = 0; row < dst_w.rows; row++) {
	int maprow0 = row_map[row + 0];
	int maprow1 = row_map[row + 1];
	int count = maprow1 - maprow0;
	int i;

	G_percent(row, dst_w.rows, 4);

	for (i = 0; i < count; i++)
	    Rast_get_d_row(infile, bufs[i], maprow0 + i);

	for (col = 0; col < dst_w.cols; col++) {
	    int mapcol0 = col_map[col + 0];
	    int mapcol1 = col_map[col + 1];
	    int null = 0;
	    int n = 0;
	    int i, j;

	    for (i = maprow0; i < maprow1; i++)
		for (j = mapcol0; j < mapcol1; j++) {
		    DCELL *src = &bufs[i - maprow0][j];
		    DCELL *dst = &values[n++];

		    if (Rast_is_d_null_value(src)) {
			Rast_set_d_null_value(dst, 1);
			null = 1;
		    }
		    else
			*dst = *src;
		}

	    if (null && nulls)
		Rast_set_d_null_value(&outbuf[col], 1);
	    else
		(*method_fn) (&outbuf[col], values, n, closure);
	}

	Rast_put_d_row(outfile, outbuf);
    }
}

static void resamp_weighted(void)
{
    stat_func_w *method_fn;

    DCELL(*values)[2];
    double *col_map, *row_map;
    int row, col;

    method_fn = menu[method].method_w;

    values = G_malloc(row_scale * col_scale * 2 * sizeof(DCELL));

    col_map = G_malloc((dst_w.cols + 1) * sizeof(double));
    row_map = G_malloc((dst_w.rows + 1) * sizeof(double));

    for (col = 0; col <= dst_w.cols; col++) {
	double x = Rast_col_to_easting(col, &dst_w);

	/* col_map[col] = Rast_easting_to_col(x, &src_w); */
	col_map[col] = (x - src_w.west) / src_w.ew_res;
    }

    for (row = 0; row <= dst_w.rows; row++) {
	double y = Rast_row_to_northing(row, &dst_w);

	row_map[row] = Rast_northing_to_row(y, &src_w);
    }

    for (row = 0; row < dst_w.rows; row++) {
	double y0 = row_map[row + 0];
	double y1 = row_map[row + 1];
	int maprow0 = (int)floor(y0);
	int maprow1 = (int)ceil(y1);
	int count = maprow1 - maprow0;
	int i;

	G_percent(row, dst_w.rows, 4);

	for (i = 0; i < count; i++)
	    Rast_get_d_row(infile, bufs[i], maprow0 + i);

	for (col = 0; col < dst_w.cols; col++) {
	    double x0 = col_map[col + 0];
	    double x1 = col_map[col + 1];
	    int mapcol0 = (int)floor(x0);
	    int mapcol1 = (int)ceil(x1);
	    int null = 0;
	    int n = 0;
	    int i, j;

	    for (i = maprow0; i < maprow1; i++) {
		double ky = (i == maprow0) ? 1 - (y0 - maprow0)
		    : (i == maprow1 - 1) ? 1 - (maprow1 - y1)
		    : 1;

		for (j = mapcol0; j < mapcol1; j++) {
		    double kx = (j == mapcol0) ? 1 - (x0 - mapcol0)
			: (j == mapcol1 - 1) ? 1 - (mapcol1 - x1)
			: 1;

		    DCELL *src = &bufs[i - maprow0][j];
		    DCELL *dst = &values[n++][0];

		    if (Rast_is_d_null_value(src)) {
			Rast_set_d_null_value(&dst[0], 1);
			null = 1;
		    }
		    else {
			dst[0] = *src;
			dst[1] = kx * ky;
		    }
		}
	    }

	    if (null && nulls)
		Rast_set_d_null_value(&outbuf[col], 1);
	    else
		(*method_fn) (&outbuf[col], values, n, closure);
	}

	Rast_put_d_row(outfile, outbuf);
    }
}

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct
    {
	struct Option *rastin, *rastout, *method, *quantile;
    } parm;
    struct
    {
	struct Flag *nulls, *weight;
    } flag;
    struct History history;
    char title[64];
    char buf_nsres[100], buf_ewres[100];
    struct Colors colors;
    int row;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("resample"));
    G_add_keyword(_("univariate statistics"));
    G_add_keyword(_("aggregation"));
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

    flag.nulls = G_define_flag();
    flag.nulls->key = 'n';
    flag.nulls->description = _("Propagate NULLs");

    flag.weight = G_define_flag();
    flag.weight->key = 'w';
    flag.weight->description = _("Weight according to area (slower)");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    nulls = flag.nulls->answer;

    method = find_method(parm.method->answer);
    if (method < 0)
	G_fatal_error(_("Unknown method <%s>"), parm.method->answer);

    if (menu[method].method == c_quant) {
	quantile = atoi(parm.quantile->answer);
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
	int c1 = src_w.cols + (int)ceil((dst_w.east - src_w.east) / src_w.ew_res);

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

    /* allocate buffers for input rows */
    bufs = G_malloc(row_scale * sizeof(DCELL *));
    for (row = 0; row < row_scale; row++)
	bufs[row] = Rast_allocate_d_input_buf();

    /* open old map */
    infile = Rast_open_old(parm.rastin->answer, "");

    /* allocate output buffer */
    outbuf = Rast_allocate_d_output_buf();

    /* open new map */
    outfile = Rast_open_new(parm.rastout->answer, DCELL_TYPE);

    if (flag.weight->answer && menu[method].method_w)
	resamp_weighted();
    else
	resamp_unweighted();

    G_percent(dst_w.rows, dst_w.rows, 2);

    Rast_close(infile);
    Rast_close(outfile);

    /* record map metadata/history info */
    sprintf(title, "Aggregate resample by %s", parm.method->answer);
    Rast_put_cell_title(parm.rastout->answer, title);

    Rast_short_history(parm.rastout->answer, "raster", &history);
    Rast_set_history(&history, HIST_DATSRC_1, parm.rastin->answer);
    G_format_resolution(src_w.ns_res, buf_nsres, src_w.proj);
    G_format_resolution(src_w.ew_res, buf_ewres, src_w.proj);
    Rast_format_history(&history, HIST_DATSRC_2,
			"Source map NS res: %s   EW res: %s",
			buf_nsres, buf_ewres);
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
