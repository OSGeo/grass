
/****************************************************************************
 *
 * MODULE:       r.grow2
 *
 * AUTHOR(S):    Marjorie Larson - CERL
 *               Glynn Clements
 *
 * PURPOSE:      Generates a raster map layer with contiguous areas 
 *               grown by one cell.
 *
 * COPYRIGHT:    (C) 2006 by the GRASS Development Team
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
#include <grass/glocale.h>

static int nrows, ncols;
static char *in_row;
static CELL *old_x_row, *old_y_row;
static CELL *new_x_row, *new_y_row;
static DCELL *dist_row;
static double (*distance) (double dx, double dy);
static double xres, yres;

#define MAX(a, b)	((a) > (b) ? (a) : (b))

static double distance_euclidian_squared(double dx, double dy)
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

void swap_rows(void)
{
    CELL *temp;

    temp = old_x_row;
    old_x_row = new_x_row;
    new_x_row = temp;
    temp = old_y_row;
    old_y_row = new_y_row;
    new_y_row = temp;
}
static void check(int col, int dx, int dy)
{
    const CELL *xrow = dy ? old_x_row : new_x_row;
    const CELL *yrow = dy ? old_y_row : new_y_row;
    int x, y;
    double d;

    if (dist_row[col] == 0)
	return;

    if (col + dx < 0)
	return;

    if (col + dx >= ncols)
	return;

    if (G_is_c_null_value(&xrow[col + dx]))
	return;

    x = xrow[col + dx] + dx;
    y = yrow[col + dx] + dy;
    d = (*distance) (xres * x, yres * y);

    if (!G_is_d_null_value(&dist_row[col]) && dist_row[col] < d)
	return;

    dist_row[col] = d;
    new_x_row[col] = x;
    new_y_row[col] = y;
}

int main(int argc, char **argv)
{
    struct GModule *module;
    struct
    {
	struct Option *in, *out, *met;
    } opt;
    char *in_name;
    char *out_name;
    char *mapset;
    int in_fd;
    int out_fd;
    char *temp_name;
    int temp_fd;
    int row, col;
    struct Colors colors;
    struct FPRange range;
    DCELL min, max;
    DCELL *out_row;
    struct Cell_head window;

    G_gisinit(argv[0]);

    module = G_define_module();
    module->keywords = _("raster");
    module->description =
	_
	("Generates a raster map layer of distance to features in input layer.");

    opt.in = G_define_standard_option(G_OPT_R_INPUT);

    opt.out = G_define_standard_option(G_OPT_R_OUTPUT);

    opt.met = G_define_option();
    opt.met->key = "metric";
    opt.met->type = TYPE_STRING;
    opt.met->required = NO;
    opt.met->description = _("Metric");
    opt.met->options = "euclidian,squared,maximum,manhattan";
    opt.met->answer = "euclidian";

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    in_name = opt.in->answer;
    out_name = opt.out->answer;

    mapset = G_find_cell(in_name, "");
    if (!mapset)
	G_fatal_error(_("Raster map <%s> not found"), in_name);

    if (strcmp(opt.met->answer, "euclidian") == 0)
	distance = &distance_euclidian_squared;
    else if (strcmp(opt.met->answer, "squared") == 0)
	distance = &distance_euclidian_squared;
    else if (strcmp(opt.met->answer, "maximum") == 0)
	distance = &distance_maximum;
    else if (strcmp(opt.met->answer, "manhattan") == 0)
	distance = &distance_manhattan;
    else
	G_fatal_error(_("Unknown metric: [%s]."), opt.met->answer);

    in_fd = G_open_cell_old(in_name, mapset);
    if (in_fd < 0)
	G_fatal_error(_("Unable to open raster map <%s>"), in_name);

    out_fd = G_open_raster_new(out_name, DCELL_TYPE);
    if (out_fd < 0)
	G_fatal_error(_("Unable to create raster map <%s>"), out_name);

    temp_name = G_tempfile();
    temp_fd = open(temp_name, O_RDWR | O_CREAT | O_EXCL, 0700);
    if (temp_fd < 0)
	G_fatal_error(_("Unable to create temporary file <%s>"), temp_name);

    G_get_window(&window);

    nrows = window.rows;
    ncols = window.cols;
    xres = window.ew_res;
    yres = window.ns_res;

    in_row = G_allocate_null_buf();

    old_x_row = G_allocate_c_raster_buf();
    old_y_row = G_allocate_c_raster_buf();
    new_x_row = G_allocate_c_raster_buf();
    new_y_row = G_allocate_c_raster_buf();

    dist_row = G_allocate_d_raster_buf();

    if (strcmp(opt.met->answer, "euclidian") == 0)
	out_row = G_allocate_d_raster_buf();
    else
	out_row = dist_row;

    G_set_c_null_value(old_x_row, ncols);
    G_set_c_null_value(old_y_row, ncols);

    for (row = 0; row < nrows; row++) {
	int irow = nrows - 1 - row;

	G_percent(row, nrows, 2);

	G_set_c_null_value(new_x_row, ncols);
	G_set_c_null_value(new_y_row, ncols);

	G_set_d_null_value(dist_row, ncols);

	G_get_null_value_row(in_fd, in_row, irow);

	for (col = 0; col < ncols; col++)
	    if (!in_row[col]) {
		new_x_row[col] = 0;
		new_y_row[col] = 0;
		dist_row[col] = 0;
	    }

	for (col = 0; col < ncols; col++)
	    check(col, -1, 0);

	for (col = ncols - 1; col >= 0; col--)
	    check(col, 1, 0);

	for (col = 0; col < ncols; col++) {
	    check(col, -1, 1);
	    check(col, 0, 1);
	    check(col, 1, 1);
	}

	write(temp_fd, new_x_row, ncols * sizeof(CELL));
	write(temp_fd, new_y_row, ncols * sizeof(CELL));
	write(temp_fd, dist_row, ncols * sizeof(DCELL));

	swap_rows();
    }

    G_percent(row, nrows, 2);

    G_close_cell(in_fd);

    G_set_c_null_value(old_x_row, ncols);
    G_set_c_null_value(old_y_row, ncols);

    for (row = 0; row < nrows; row++) {
	int irow = nrows - 1 - row;
	off_t offset =
	    (off_t) irow * ncols * (2 * sizeof(CELL) + sizeof(DCELL));

	G_percent(row, nrows, 2);

	lseek(temp_fd, offset, SEEK_SET);

	read(temp_fd, new_x_row, ncols * sizeof(CELL));
	read(temp_fd, new_y_row, ncols * sizeof(CELL));
	read(temp_fd, dist_row, ncols * sizeof(DCELL));

	for (col = 0; col < ncols; col++) {
	    check(col, -1, -1);
	    check(col, 0, -1);
	    check(col, 1, -1);
	}

	if (out_row != dist_row)
	    for (col = 0; col < ncols; col++)
		out_row[col] = sqrt(dist_row[col]);

	G_put_d_raster_row(out_fd, out_row);

	swap_rows();
    }

    G_percent(row, nrows, 2);

    close(temp_fd);
    remove(temp_name);

    G_close_cell(out_fd);

    G_init_colors(&colors);
    G_read_fp_range(out_name, G_mapset(), &range);
    G_get_fp_range_min_max(&range, &min, &max);
    G_make_fp_colors(&colors, "rainbow", min, max);
    G_write_colors(out_name, G_mapset(), &colors);

    return EXIT_SUCCESS;
}
