
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
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

#define MAX(a, b)	((a) > (b) ? (a) : (b))

static int size;
static int count;
static int (*neighbors)[2];

typedef int metric_fn(int, int);


static int distance_euclidean_squared(int dx, int dy)
{
    return dx * dx + dy * dy;
}

static int distance_maximum(int dx, int dy)
{
    return MAX(abs(dx), abs(dy));
}

static int distance_manhattan(int dx, int dy)
{
    return abs(dx) + abs(dy);
}

static void setup_neighbors(double radius, int limit, metric_fn * dist)
{
    int i, dx, dy;
    int n;

    size = (int)radius;

    n = size * 2 + 1;

    neighbors = G_malloc(n * n * 2 * sizeof(int));

    count = 0;

    for (i = 1; i <= limit; i++) {
	for (dy = -size; dy <= size; dy++) {
	    for (dx = -size; dx <= size; dx++) {
		if ((*dist) (dx, dy) != i)
		    continue;

		neighbors[count][0] = dx;
		neighbors[count][1] = dy;
		count++;
	    }
	}
    }
}

static void setup_neighbors_euclidean(double radius)
{
    int r2 = (int)(radius * radius);

    setup_neighbors(radius, r2, distance_euclidean_squared);
}

static void setup_neighbors_maximum(double radius)
{
    setup_neighbors(radius, (int)radius, distance_maximum);
}

static void setup_neighbors_manhattan(double radius)
{
    setup_neighbors(radius, (int)radius, distance_manhattan);
}

int main(int argc, char **argv)
{
    struct GModule *module;
    struct
    {
	struct Option *in, *out, *rad, *met, *old, *new;
    } opt;
    struct Colors colr;
    struct Categories cats;
    struct History history;
    int colrfile;
    char *in_name;
    char *out_name;
    double radius;
    int oldval = 0;
    int newval = 0;
    RASTER_MAP_TYPE type;
    int in_fd;
    int out_fd;
    DCELL **in_rows;
    DCELL *out_row;
    int nrows, row;
    int ncols, col;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("distance"));
    module->description =
	_("Generates a raster map layer "
	  "with contiguous areas grown by one cell.");

    opt.in = G_define_standard_option(G_OPT_R_INPUT);

    opt.out = G_define_standard_option(G_OPT_R_OUTPUT);

    opt.rad = G_define_option();
    opt.rad->key = "radius";
    opt.rad->type = TYPE_DOUBLE;
    opt.rad->required = NO;
    opt.rad->description = _("Radius of buffer in raster cells");
    opt.rad->answer = "1.01";

    opt.met = G_define_option();
    opt.met->key = "metric";
    opt.met->type = TYPE_STRING;
    opt.met->required = NO;
    opt.met->description = _("Metric");
    opt.met->options = "euclidean,maximum,manhattan";
    opt.met->answer = "euclidean";

    opt.old = G_define_option();
    opt.old->key = "old";
    opt.old->type = TYPE_INTEGER;
    opt.old->required = NO;
    opt.old->description =
	_("Value to write for input cells which are non-NULL (-1 => NULL)");

    opt.new = G_define_option();
    opt.new->key = "new";
    opt.new->type = TYPE_INTEGER;
    opt.new->required = NO;
    opt.new->description = _("Value to write for \"grown\" cells");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    in_name = opt.in->answer;
    out_name = opt.out->answer;

    radius = atof(opt.rad->answer);

    if (opt.old->answer)
	oldval = atoi(opt.old->answer);

    if (opt.new->answer)
	newval = atoi(opt.new->answer);

    nrows = Rast_window_rows();
    ncols = Rast_window_cols();

    if (strcmp(opt.met->answer, "euclidean") == 0)
	setup_neighbors_euclidean(radius);
    else if (strcmp(opt.met->answer, "maximum") == 0)
	setup_neighbors_maximum(radius);
    else if (strcmp(opt.met->answer, "manhattan") == 0)
	setup_neighbors_manhattan(radius);
    else
	G_fatal_error(_("Unknown metric: [%s]."), opt.met->answer);

    in_fd = Rast_open_old(in_name, "");

    type = Rast_get_map_type(in_fd);

    out_fd = Rast_open_new(out_name, type);

    if (Rast_read_cats(in_name, "", &cats) == -1) {
	G_warning(_("Error reading category file for <%s>"), in_name);
	Rast_init_cats("", &cats);
    }

    if (Rast_read_colors(in_name, "", &colr) == -1) {
	G_warning(_("Error in reading color file for <%s>"), in_name);
	colrfile = 0;
    }
    else
	colrfile = 1;

    if (opt.old->answer && oldval >= 0)
	Rast_set_c_cat(&oldval, &oldval, "original cells", &cats);

    if (opt.new->answer)
	Rast_set_c_cat(&newval, &newval,"grown cells", &cats);

    in_rows = G_malloc((size * 2 + 1) * sizeof(DCELL *));

    for (row = 0; row <= size * 2; row++)
	in_rows[row] = Rast_allocate_d_buf();

    out_row = Rast_allocate_d_buf();

    for (row = 0; row < size; row++)
	Rast_get_d_row(in_fd, in_rows[size + row], row);

    for (row = 0; row < nrows; row++) {
	DCELL *tmp;
	int i;

	if (row + size < nrows)
	    Rast_get_d_row(in_fd, in_rows[size * 2], row + size);

	for (col = 0; col < ncols; col++) {
	    DCELL *c = &in_rows[size][col];

	    if (!Rast_is_d_null_value(c)) {
		if (opt.old->answer) {
		    if (oldval < 0)
			Rast_set_d_null_value(&out_row[col], 1);
		    else
			out_row[col] = oldval;
		}
		else
		    out_row[col] = *c;

		continue;
	    }

	    for (i = 0; i < count; i++) {
		int dx = neighbors[i][0];
		int dy = neighbors[i][1];
		int x = col + dx;
		int y = row + dy;

		if (x < 0 || x >= ncols || y < 0 || y >= nrows)
		    continue;

		c = &in_rows[size + dy][x];

		if (!Rast_is_d_null_value(c)) {
		    out_row[col] = opt.new->answer ? newval : *c;
		    break;
		}
	    }

	    if (i == count)
		Rast_set_d_null_value(&out_row[col], 1);
	}

	Rast_put_d_row(out_fd, out_row);

	G_percent(row, nrows, 2);

	tmp = in_rows[0];
	for (i = 0; i < size * 2; i++)
	    in_rows[i] = in_rows[i + 1];
	in_rows[size * 2] = tmp;
    }

    G_percent(row, nrows, 2);

    Rast_close(in_fd);
    Rast_close(out_fd);

    Rast_write_cats(out_name, &cats);

    if (colrfile)
	Rast_write_colors(out_name, G_mapset(), &colr);

    Rast_short_history(out_name, "raster", &history);
    Rast_command_history(&history);
    Rast_write_history(out_name, &history);

    return EXIT_SUCCESS;
}
