
/****************************************************************************
 *
 * MODULE:       r.resamp.filter
 * AUTHOR(S):    Glynn Clements <glynn gclements.plus.com>
 * PURPOSE:      
 * COPYRIGHT:    (C) 2010 by Glynn Clements and the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

static double f_box(double x)
{
    return (x > 1) ? 0
	: 1;
}

static double f_bartlett(double x)
{
    return (x > 1) ? 0
	: 1 - x;
}

static double f_hermite(double x)
{
    return (x > 1) ? 0
	: (2 * x - 3) * x * x + 1;
}

static double f_gauss(double x)
{
    return exp(-2 * x * x) * sqrt(2 / M_PI);
}

static double f_normal(double x)
{
    return f_gauss(x/2) / 2;
}

static double f_sinc(double x)
{
    return (x == 0) ? 1 : sin(M_PI * x) / (M_PI * x);
}

static double lanczos(double x, int a)
{
    return (x > a) ? 0
	: f_sinc(x) * f_sinc(x / a);
}

static double f_lanczos1(double x)
{
    return lanczos(x, 1);
}

static double f_lanczos2(double x)
{
    return lanczos(x, 2);
}

static double f_lanczos3(double x)
{
    return lanczos(x, 3);
}

static double f_hann(double x)
{
    return cos(M_PI * x) / 2 + 0.5;
}

static double f_hamming(double x)
{
    return 0.46 * cos(M_PI * x) + 0.54;
}


static double f_blackman(double x)
{
    return cos(M_PI * x) / 2 + 0.08 * cos(2 * M_PI * x) + 0.42;
}

struct filter_type {
    const char *name;
    double (*func)(double);
    int radius;
};

static const struct filter_type menu[] = {
    {"box",       f_box,       1},
    {"bartlett",  f_bartlett,  1},
    {"gauss",     f_gauss,     0},
    {"normal",    f_normal,    0},
    {"hermite",   f_hermite,   1},
    {"sinc",      f_sinc,      0},
    {"lanczos1",  f_lanczos1,  1},
    {"lanczos2",  f_lanczos2,  2},
    {"lanczos3",  f_lanczos3,  3},
    {"hann",      f_hann,      0},
    {"hamming",   f_hamming,   0},
    {"blackman",  f_blackman,  0},
    {NULL},
};

static char *build_filter_list(void)
{
    char *buf = G_malloc(1024);
    char *p = buf;
    int i;

    for (i = 0; menu[i].name; i++) {
	const char *q;

	if (i)
	    *p++ = ',';
	for (q = menu[i].name; *q; p++, q++)
	    *p = *q;
    }
    *p = '\0';

    return buf;
}

static const struct filter_type *find_method(const char *name)
{
    int i;

    for (i = 0; menu[i].name; i++)
	if (strcmp(menu[i].name, name) == 0)
	    return &menu[i];

    G_fatal_error(_("Filter <%s> not found"), name);

    return NULL;
}

struct filter {
    double (*func)(double);
    double x_radius;
    double y_radius;
};

#define MAX_FILTERS 8

static int infile, outfile;
static struct filter filters[MAX_FILTERS];
static int num_filters;
static int nulls;
static struct Cell_head dst_w, src_w;
static double f_x_radius, f_y_radius;
static int row_scale, col_scale;
static DCELL *inbuf;
static DCELL *outbuf;
static DCELL **bufs;
static double *h_weights;
static double *v_weights;
static int *mapcol0, *mapcol1;
static int *maprow0, *maprow1;

static void make_h_weights(void)
{
    int col;

    h_weights = G_malloc(dst_w.cols * col_scale * sizeof(double));
    mapcol0 = G_malloc(dst_w.cols * sizeof(int));
    mapcol1 = G_malloc(dst_w.cols * sizeof(int));

    for (col = 0; col < dst_w.cols; col++) {
	double dx = Rast_col_to_easting(col + 0.5, &dst_w);
	/* do not use Rast_easting_to_col() because it does ll wrap */
	/*
	double x0 = Rast_easting_to_col(dx - f_x_radius, &src_w);
	double x1 = Rast_easting_to_col(dx + f_x_radius, &src_w);
	*/
	double x0 = (dx - f_x_radius - src_w.west) / src_w.ew_res;
	double x1 = (dx + f_x_radius - src_w.west) / src_w.ew_res;
	int col0 = (int)floor(x0);
	int col1 = (int)floor(x1) + 1;
	int cols = col1 - col0;
	int j;

	mapcol0[col] = col0;
	mapcol1[col] = col1;

	for (j = 0; j < cols; j++) {
	    double sx = Rast_col_to_easting(col0 + j + 0.5, &src_w);
	    double r = fabs(sx - dx);
	    double w = 1.0;
	    int k;

	    for (k = 0; k < num_filters; k++)
		w *= (*filters[k].func)(r / filters[k].x_radius);

	    h_weights[col * col_scale + j] = w;
	}

	for (j = cols; j < col_scale; j++)
	    h_weights[col * col_scale + j] = 0;
    }
}

static void make_v_weights(void)
{
    int row;

    v_weights = G_malloc(dst_w.rows * row_scale * sizeof(double));
    maprow0 = G_malloc(dst_w.rows * sizeof(int));
    maprow1 = G_malloc(dst_w.rows * sizeof(int));

    for (row = 0; row < dst_w.rows; row++) {
	double dy = Rast_row_to_northing(row + 0.5, &dst_w);
	double y0 = Rast_northing_to_row(dy + f_y_radius, &src_w);
	double y1 = Rast_northing_to_row(dy - f_y_radius, &src_w);
	int row0 = (int)floor(y0);
	int row1 = (int)floor(y1) + 1;
	int rows = row1 - row0;
	int i;

	maprow0[row] = row0;
	maprow1[row] = row1;

	for (i = 0; i < rows; i++) {
	    double sy = Rast_row_to_northing(row0 + i + 0.5, &src_w);
	    double r = fabs(sy - dy);
	    double w = 1.0;
	    int k;

	    for (k = 0; k < num_filters; k++)
		w *= (*filters[k].func)(r / filters[k].y_radius);

	    v_weights[row * row_scale + i] = w;
	}

	for (i = rows; i < row_scale; i++)
	    v_weights[row * row_scale + i] = 0;
    }
}

static void h_filter(DCELL *dst, const DCELL *src)
{
    int col;

    for (col = 0; col < dst_w.cols; col++) {
	int col0 = mapcol0[col];
	int col1 = mapcol1[col];
	int cols = col1 - col0;
	double numer = 0.0;
	double denom = 0.0;
	int null = 0;
	int j;

	for (j = 0; j < cols; j++) {
	    double w = h_weights[col * col_scale + j];
	    const DCELL *c = &src[col0 + j];

	    if (Rast_is_d_null_value(c)) {
		if (nulls) {
		    null = 1;
		    break;
		}
	    }
	    else {
		numer += w * (*c);
		denom += w;
	    }
	}

	if (null || denom == 0)
	    Rast_set_d_null_value(&dst[col], 1);
	else
	    dst[col] = numer / denom;
    }
}

static void v_filter(DCELL *dst, DCELL **src, int row, int rows)
{
    int col;

    for (col = 0; col < dst_w.cols; col++) {
	double numer = 0.0;
	double denom = 0.0;
	int null = 0;
	int i;

	for (i = 0; i < rows; i++) {
	    double w = v_weights[row * row_scale + i];
	    const DCELL *c = &src[i][col];

	    if (Rast_is_d_null_value(c)) {
		if (nulls) {
		    null = 1;
		    break;
		}
	    }
	    else {
		numer += w * (*c);
		denom += w;
	    }
	}

	if (null || denom == 0)
	    Rast_set_d_null_value(&dst[col], 1);
	else
	    dst[col] = numer / denom;
    }
}

static void filter(void)
{
    int cur_row = 0;
    int num_rows = 0;
    int row;

    make_h_weights();
    make_v_weights();

    for (row = 0; row < dst_w.rows; row++) {
	int row0 = maprow0[row];
	int row1 = maprow1[row];
	int rows = row1 - row0;
	int i;

	G_percent(row, dst_w.rows, 2);

	if (row0 >= cur_row && row0 < cur_row + num_rows) {
	    int m = row0 - cur_row;
	    int n = cur_row + num_rows - row0;
	    int i;

	    for (i = 0; i < n; i++) {
		DCELL *tmp = bufs[i];
		bufs[i] = bufs[m + i];
		bufs[m + i] = tmp;
	    }

	    cur_row = row0;
	    num_rows = n;
	}
	else {
	    cur_row = row0;
	    num_rows = 0;
	}

	for (i = num_rows; i < rows; i++) {
	    G_debug(5, "read: %p = %d", bufs[i], row0 + i);
	    /* enlarging the source window to the North and South is 
	     * not possible for global maps in ll */
	    if (row0 + i >= 0 && row0 + i < src_w.rows)
		Rast_get_d_row(infile, inbuf, row0 + i);
	    else
		Rast_set_d_null_value(inbuf, src_w.cols);
	    h_filter(bufs[i], inbuf);
	}

	num_rows = rows;

	v_filter(outbuf, bufs, row, rows);

	Rast_put_d_row(outfile, outbuf);
	G_debug(5, "write: %d", row);
    }

    G_percent(dst_w.rows, dst_w.rows, 2);
}

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct
    {
	struct Option *rastin, *rastout, *method,
	    *radius, *x_radius, *y_radius;
    } parm;
    struct
    {
	struct Flag *nulls;
    } flag;
    char title[64];
    int i;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("resample"));
    G_add_keyword(_("kernel filter"));
    G_add_keyword(_("filter"));
    G_add_keyword(_("convolution"));
    G_add_keyword(_("FIR"));
    G_add_keyword(_("bartlett"));
    G_add_keyword(_("blackman"));
    G_add_keyword(_("box"));
    G_add_keyword(_("gauss"));
    G_add_keyword(_("hamming"));
    G_add_keyword(_("hann"));
    G_add_keyword(_("hermite"));
    G_add_keyword(_("lanczos"));
    G_add_keyword(_("sinc"));

    module->description =
	_("Resamples raster map layers using an analytic kernel.");

    parm.rastin = G_define_standard_option(G_OPT_R_INPUT);

    parm.rastout = G_define_standard_option(G_OPT_R_OUTPUT);

    parm.method = G_define_option();
    parm.method->key = "filter";
    parm.method->type = TYPE_STRING;
    parm.method->required = YES;
    parm.method->multiple = YES;
    parm.method->description = _("Filter kernel(s)");
    parm.method->options = build_filter_list();

    parm.radius = G_define_option();
    parm.radius->key = "radius";
    parm.radius->type = TYPE_DOUBLE;
    parm.radius->required = NO;
    parm.radius->multiple = YES;
    parm.radius->description = _("Filter radius");

    parm.x_radius = G_define_option();
    parm.x_radius->key = "x_radius";
    parm.x_radius->type = TYPE_DOUBLE;
    parm.x_radius->required = NO;
    parm.x_radius->multiple = YES;
    parm.x_radius->description = _("Filter radius (horizontal)");

    parm.y_radius = G_define_option();
    parm.y_radius->key = "y_radius";
    parm.y_radius->type = TYPE_DOUBLE;
    parm.y_radius->required = NO;
    parm.y_radius->multiple = YES;
    parm.y_radius->description = _("Filter radius (vertical)");

    flag.nulls = G_define_flag();
    flag.nulls->key = 'n';
    flag.nulls->description = _("Propagate NULLs");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    if (parm.radius->answer) {
	if (parm.x_radius->answer || parm.y_radius->answer)
	    G_fatal_error(_("%s= and %s=/%s= are mutually exclusive"),
			    parm.radius->key, parm.x_radius->key, parm.y_radius->key);
    }
    else {
	if (!parm.x_radius->answer && !parm.y_radius->answer)
	    G_fatal_error(_("Either %s= or %s=/%s= required"),
			    parm.radius->key, parm.x_radius->key, parm.y_radius->key);
	if (!parm.x_radius->answer || !parm.y_radius->answer)
	    G_fatal_error(_("Both %s= and %s= required"),
			    parm.x_radius->key, parm.y_radius->key);
    }

    nulls = flag.nulls->answer;

    f_x_radius = f_y_radius = 1e100;

    for (i = 0; ; i++) {
	const char *filter_arg = parm.method->answers[i];
	const char *x_radius_arg = parm.radius->answer
	    ? parm.radius->answers[i]
	    : parm.x_radius->answers[i];
	const char *y_radius_arg = parm.radius->answer
	    ? parm.radius->answers[i]
	    : parm.y_radius->answers[i];
	const struct filter_type *type;
	struct filter *filter;

	if (!filter_arg && !x_radius_arg && !y_radius_arg)
	    break;

	if (!filter_arg || !x_radius_arg || !y_radius_arg)
	    G_fatal_error(_("Differing number of values for filter= and [xy_]radius="));

	if (num_filters >= MAX_FILTERS)
	    G_fatal_error(_("Too many filters (max: %d)"), MAX_FILTERS);

	filter = &filters[num_filters++];
	type = find_method(filter_arg);
	filter->func = type->func;
	filter->x_radius = fabs(atof(x_radius_arg));
	filter->y_radius = fabs(atof(y_radius_arg));
	if (type->radius) {
	    double rx = type->radius * filter->x_radius;
	    double ry = type->radius * filter->y_radius;
	    if (rx < f_x_radius)
		f_x_radius = rx;
	    if (ry < f_y_radius)
		f_y_radius = ry;
	}
    }

    if (f_x_radius > 1e99 || f_y_radius > 1e99)
	G_fatal_error(_("At least one filter must be finite"));

    G_get_set_window(&dst_w);

    /* set window to old map */
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

    /* enlarge source window */
    {
	double y0 = Rast_row_to_northing(0.5, &dst_w);
	double y1 = Rast_row_to_northing(dst_w.rows - 0.5, &dst_w);
	double x0 = Rast_col_to_easting(0.5, &dst_w);
	double x1 = Rast_col_to_easting(dst_w.cols - 0.5, &dst_w);
	int r0 = (int)floor(Rast_northing_to_row(y0 + f_y_radius, &src_w) - 0.1);
	int r1 = (int)ceil(Rast_northing_to_row(y1 - f_y_radius, &src_w) + 0.1);
	/* do not use Rast_easting_to_col() because it does ll wrap */
	/*
	int c0 = (int)floor(Rast_easting_to_col(x0 - f_x_radius, &src_w) - 0.1);
	int c1 = (int)ceil(Rast_easting_to_col(x1 + f_x_radius, &src_w) + 0.1);
	*/
	int c0 = (int)floor((x0 - f_x_radius - src_w.west) / src_w.ew_res - 0.1);
	int c1 = (int)ceil((x1 + f_x_radius - src_w.west) / src_w.ew_res + 0.1);

	if (G_projection() == PROJECTION_LL) {
	    while (src_w.north + src_w.ns_res * (-r0) > 90 + src_w.ns_res / 2.0) {
		r0++;
	    }
	    while (src_w.south - src_w.ns_res * (r1 - src_w.rows) < -90 - src_w.ns_res / 2.0) {
		r1--;
	    }
	}

	src_w.south -= src_w.ns_res * (r1 - src_w.rows);
	src_w.north += src_w.ns_res * (-r0);
	src_w.west -= src_w.ew_res * (-c0);
	src_w.east += src_w.ew_res * (c1 - src_w.cols);
	src_w.rows = r1 - r0;
	src_w.cols = c1 - c0;
    }

    row_scale = 2 + 2 * ceil(f_y_radius / src_w.ns_res);
    col_scale = 2 + 2 * ceil(f_x_radius / src_w.ew_res);

    /* allocate buffers for intermediate rows */
    bufs = G_malloc(row_scale * sizeof(DCELL *));
    for (i = 0; i < row_scale; i++)
	bufs[i] = Rast_allocate_d_buf();

    Rast_set_input_window(&src_w);
    Rast_set_output_window(&dst_w);

    inbuf = Rast_allocate_d_input_buf();
    outbuf = Rast_allocate_d_output_buf();

    infile = Rast_open_old(parm.rastin->answer, "");
    outfile = Rast_open_new(parm.rastout->answer, DCELL_TYPE);

    filter();

    Rast_close(infile);
    Rast_close(outfile);

    /* record map metadata/history info */
    sprintf(title, "Filter resample by %s", parm.method->answer);
    Rast_put_cell_title(parm.rastout->answer, title);

    {
	struct History history;
	char buf_nsres[100], buf_ewres[100];

	Rast_short_history(parm.rastout->answer, "raster", &history);
	Rast_set_history(&history, HIST_DATSRC_1, parm.rastin->answer);
	G_format_resolution(src_w.ns_res, buf_nsres, src_w.proj);
	G_format_resolution(src_w.ew_res, buf_ewres, src_w.proj);
	Rast_format_history(&history, HIST_DATSRC_2,
			    "Source map NS res: %s   EW res: %s",
			    buf_nsres, buf_ewres);
	Rast_command_history(&history);
	Rast_write_history(parm.rastout->answer, &history);
    }

    /* copy color table from source map */
    {
	struct Colors colors;

	if (Rast_read_colors(parm.rastin->answer, "", &colors) < 0)
	    G_fatal_error(_("Unable to read color table for %s"),
			  parm.rastin->answer);
	Rast_mark_colors_as_fp(&colors);
	Rast_write_colors(parm.rastout->answer, G_mapset(), &colors);
    }

    return EXIT_SUCCESS;
}
