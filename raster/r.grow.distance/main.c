
/****************************************************************************
 *
 * MODULE:       r.grow.distance
 *
 * AUTHOR(S):    Marjorie Larson - CERL
 *               Glynn Clements
 *
 * PURPOSE:      Generates a raster map layer with contiguous areas 
 *               grown by one cell.
 *
 * COPYRIGHT:    (C) 2006, 2010 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

static struct Cell_head window;
static int nrows, ncols;
static DCELL *in_row;
static CELL *old_x_row, *old_y_row;
static CELL *new_x_row, *new_y_row;
static DCELL *dist_row;
static DCELL *old_val_row, *new_val_row;
static double (*distance) (double dx, double dy);
static double xres, yres;

#undef MAX
#define MAX(a, b)	((a) > (b) ? (a) : (b))

static double distance_euclidean_squared(double dx, double dy)
{
    return dx * dx + dy * dy;
}

static double distance_maximum(double dx, double dy)
{
    return MAX(abs(dx), abs(dy));
}

static double distance_manhattan(double dx, double dy)
{
    return abs(dx) + abs(dy);
}

static double geodesic_distance(int x1, int y1, int x2, int y2)
{
    double lat1 = Rast_row_to_northing(y1 + 0.5, &window);
    double lat2 = Rast_row_to_northing(y2 + 0.5, &window);
    double lon1 = Rast_col_to_easting(x1 + 0.5, &window);
    double lon2 = Rast_col_to_easting(x2 + 0.5, &window);

    return G_geodesic_distance(lon1, lat1, lon2, lat2);
}

void swap_rows(void)
{
    CELL *temp;
    DCELL *dtemp;

    temp = old_x_row;
    old_x_row = new_x_row;
    new_x_row = temp;

    temp = old_y_row;
    old_y_row = new_y_row;
    new_y_row = temp;

    dtemp = old_val_row;
    old_val_row = new_val_row;
    new_val_row = dtemp;
}

static void check(int row, int col, int dx, int dy)
{
    const CELL *xrow = dy ? old_x_row : new_x_row;
    const CELL *yrow = dy ? old_y_row : new_y_row;
    const DCELL *vrow = dy ? old_val_row : new_val_row;
    int x, y;
    double d, v;

    if (dist_row[col] == 0)
	return;

    if (col + dx < 0)
	return;

    if (col + dx >= ncols)
	return;

    if (Rast_is_c_null_value(&xrow[col + dx]))
	return;

    x = xrow[col + dx] + dx;
    y = yrow[col + dx] + dy;
    v = vrow[col + dx];
    d = distance
	? (*distance) (xres * x, yres * y)
	: geodesic_distance(col, row, col + x, row + y);

    if (!Rast_is_d_null_value(&dist_row[col]) && dist_row[col] < d)
	return;

    dist_row[col] = d;
    new_val_row[col] = v;
    new_x_row[col] = x;
    new_y_row[col] = y;
}

int main(int argc, char **argv)
{
    struct GModule *module;
    struct
    {
	struct Option *in, *dist, *val, *met;
    } opt;
    struct
    {
	struct Flag *m;
    } flag;
    const char *in_name;
    const char *dist_name;
    const char *val_name;
    int in_fd;
    int dist_fd, val_fd;
    char *temp_name;
    int temp_fd;
    int row, col;
    struct Colors colors;
    struct FPRange range;
    struct History hist;
    DCELL min, max;
    DCELL *out_row;
    double scale = 1.0;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("distance"));
    module->description =
	_("Generates a raster map of distance to features in input raster map.");

    opt.in = G_define_standard_option(G_OPT_R_INPUT);

    opt.dist = G_define_standard_option(G_OPT_R_OUTPUT);
    opt.dist->key = "distance";
    opt.dist->required = NO;
    opt.dist->description = _("Name for distance output raster map");
    opt.dist->guisection = _("Output");

    opt.val = G_define_standard_option(G_OPT_R_OUTPUT);
    opt.val->key = "value";
    opt.val->required = NO;
    opt.val->description = _("Name for value output raster map");
    opt.val->guisection = _("Output");

    opt.met = G_define_option();
    opt.met->key = "metric";
    opt.met->type = TYPE_STRING;
    opt.met->required = NO;
    opt.met->description = _("Metric");
    opt.met->options = "euclidean,squared,maximum,manhattan,geodesic";
    opt.met->answer = "euclidean";

    flag.m = G_define_flag();
    flag.m->key = 'm';
    flag.m->description = _("Output distances in meters instead of map units");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    in_name = opt.in->answer;
    dist_name = opt.dist->answer;
    val_name = opt.val->answer;

    if (!dist_name && !val_name)
	G_fatal_error(_("At least one of distance= and value= must be given"));

    G_get_window(&window);

    if (strcmp(opt.met->answer, "euclidean") == 0)
	distance = &distance_euclidean_squared;
    else if (strcmp(opt.met->answer, "squared") == 0)
	distance = &distance_euclidean_squared;
    else if (strcmp(opt.met->answer, "maximum") == 0)
	distance = &distance_maximum;
    else if (strcmp(opt.met->answer, "manhattan") == 0)
	distance = &distance_manhattan;
    else if (strcmp(opt.met->answer, "geodesic") == 0) {
	double a, e2;
	if (window.proj != PROJECTION_LL)
	    G_fatal_error(_("metric=geodesic is only valid for lat/lon"));
	distance = NULL;
	G_get_ellipsoid_parameters(&a, &e2);
	G_begin_geodesic_distance(a, e2);
    }
    else
	G_fatal_error(_("Unknown metric: '%s'"), opt.met->answer);

    if (flag.m->answer) {
	if (window.proj == PROJECTION_LL && 
	    strcmp(opt.met->answer, "geodesic") != 0) {
	    G_fatal_error(_("Output distance in meters for lat/lon is only possible with '%s=%s'"),
	                  opt.met->key, "geodesic");
	}

	scale = G_database_units_to_meters_factor();
	if (strcmp(opt.met->answer, "squared") == 0)
	    scale *= scale;
    }

    in_fd = Rast_open_old(in_name, "");

    if (dist_name)
	dist_fd = Rast_open_new(dist_name, DCELL_TYPE);

    if (val_name)
	val_fd = Rast_open_new(val_name, DCELL_TYPE);

    temp_name = G_tempfile();
    temp_fd = open(temp_name, O_RDWR | O_CREAT | O_EXCL, 0700);
    if (temp_fd < 0)
	G_fatal_error(_("Unable to create temporary file <%s>"), temp_name);

    nrows = window.rows;
    ncols = window.cols;
    xres = window.ew_res;
    yres = window.ns_res;

    in_row = Rast_allocate_d_buf();

    old_val_row = Rast_allocate_d_buf();
    new_val_row = Rast_allocate_d_buf();

    old_x_row = Rast_allocate_c_buf();
    old_y_row = Rast_allocate_c_buf();
    new_x_row = Rast_allocate_c_buf();
    new_y_row = Rast_allocate_c_buf();

    dist_row = Rast_allocate_d_buf();

    if (dist_name && strcmp(opt.met->answer, "euclidean") == 0)
	out_row = Rast_allocate_d_buf();
    else
	out_row = dist_row;

    Rast_set_c_null_value(old_x_row, ncols);
    Rast_set_c_null_value(old_y_row, ncols);

    G_message(_("Reading raster map <%s>..."), opt.in->answer);
    for (row = 0; row < nrows; row++) {
	int irow = nrows - 1 - row;

	G_percent(row, nrows, 2);

	Rast_set_c_null_value(new_x_row, ncols);
	Rast_set_c_null_value(new_y_row, ncols);

	Rast_set_d_null_value(dist_row, ncols);

	Rast_get_d_row(in_fd, in_row, irow);

	for (col = 0; col < ncols; col++)
	    if (!Rast_is_d_null_value(&in_row[col])) {
		new_x_row[col] = 0;
		new_y_row[col] = 0;
		dist_row[col] = 0;
		new_val_row[col] = in_row[col];
	    }

	for (col = 0; col < ncols; col++)
	    check(irow, col, -1, 0);

	for (col = ncols - 1; col >= 0; col--)
	    check(irow, col, 1, 0);

	for (col = 0; col < ncols; col++) {
	    check(irow, col, -1, 1);
	    check(irow, col, 0, 1);
	    check(irow, col, 1, 1);
	}

	write(temp_fd, new_x_row, ncols * sizeof(CELL));
	write(temp_fd, new_y_row, ncols * sizeof(CELL));
	write(temp_fd, dist_row, ncols * sizeof(DCELL));
	write(temp_fd, new_val_row, ncols * sizeof(DCELL));

	swap_rows();
    }

    G_percent(row, nrows, 2);

    Rast_close(in_fd);

    Rast_set_c_null_value(old_x_row, ncols);
    Rast_set_c_null_value(old_y_row, ncols);

    G_message(_("Writing output raster maps..."));
    for (row = 0; row < nrows; row++) {
	int irow = nrows - 1 - row;
	off_t offset =
	    (off_t) irow * ncols * (2 * sizeof(CELL) + 2 * sizeof(DCELL));

	G_percent(row, nrows, 2);

	lseek(temp_fd, offset, SEEK_SET);

	read(temp_fd, new_x_row, ncols * sizeof(CELL));
	read(temp_fd, new_y_row, ncols * sizeof(CELL));
	read(temp_fd, dist_row, ncols * sizeof(DCELL));
	read(temp_fd, new_val_row, ncols * sizeof(DCELL));

	for (col = 0; col < ncols; col++) {
	    check(row, col, -1, -1);
	    check(row, col, 0, -1);
	    check(row, col, 1, -1);
	}

	for (col = 0; col < ncols; col++)
	    check(row, col, -1, 0);

	for (col = ncols - 1; col >= 0; col--)
	    check(row, col, 1, 0);

	if (dist_name) {
	    if (out_row != dist_row)
		for (col = 0; col < ncols; col++)
		    out_row[col] = sqrt(dist_row[col]);

	    if (scale != 1.0)
		for (col = 0; col < ncols; col++)
		    out_row[col] *= scale;

	    Rast_put_d_row(dist_fd, out_row);
	}

	if (val_name)
	    Rast_put_d_row(val_fd, new_val_row);

	swap_rows();
    }

    G_percent(row, nrows, 2);

    close(temp_fd);
    remove(temp_name);

    if (dist_name)
	Rast_close(dist_fd);
    if (val_name)
	Rast_close(val_fd);

    if (val_name) {
	if (Rast_read_colors(in_name, "", &colors) < 0)
	    G_fatal_error(_("Unable to read color table for raster map <%s>"), in_name);
	Rast_write_colors(val_name, G_mapset(), &colors);

	Rast_short_history(val_name, "raster", &hist);
	Rast_set_history(&hist, HIST_DATSRC_1, in_name);
	Rast_append_format_history(&hist, "value of nearest feature");
	Rast_command_history(&hist);
	Rast_write_history(val_name, &hist);
    }

    if (dist_name) {
	Rast_init_colors(&colors);
	Rast_read_fp_range(dist_name, G_mapset(), &range);
	Rast_get_fp_range_min_max(&range, &min, &max);
	Rast_make_fp_colors(&colors, "rainbow", min, max);
	Rast_write_colors(dist_name, G_mapset(), &colors);

	Rast_short_history(dist_name, "raster", &hist);
	Rast_set_history(&hist, HIST_DATSRC_1, in_name);
	Rast_append_format_history(&hist, "%s distance to nearest feature", opt.met->answer);
	Rast_command_history(&hist);
	Rast_write_history(dist_name, &hist);
    }

    return EXIT_SUCCESS;
}
