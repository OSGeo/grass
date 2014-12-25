
/****************************************************************************
 *
 * MODULE:       d.profile
 * AUTHOR(S):    Dave Johnson (original contributor)
 *               DBA Systems, Inc. 10560 Arrowhead Drive Fairfax, VA 22030
 *               Markus Neteler <neteler itc.it>, 
 *               Bernhard Reiter <bernhard intevation.de>, 
 *               Huidae Cho <grass4u gmail.com>, 
 *               Eric G. Miller <egm2 jps.net>, 
 *               Glynn Clements <glynn gclements.plus.com>
 * PURPOSE:      user chooses transects path, and profile of raster data drawn
 * COPYRIGHT:    (C) 1999-2007 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/display.h>
#include <grass/glocale.h>

static char *mapname;
static double min, max;

struct point
{
    double x, y;
    double d;
};

static void get_region_range(int fd)
{
    DCELL *buf = Rast_allocate_d_buf();

    int nrows = Rast_window_rows();
    int ncols = Rast_window_cols();
    int row, col;

    min = 1e300;
    max = -1e300;

    for (row = 0; row < nrows; row++) {
	Rast_get_d_row(fd, buf, row);
	for (col = 0; col < ncols; col++) {
	    if (min > buf[col])
		min = buf[col];
	    if (max < buf[col])
		max = buf[col];
	}
    }
}

static void get_map_range(void)
{
    if (Rast_map_type(mapname, "") == CELL_TYPE) {
	struct Range range;
	CELL xmin, xmax;

	if (Rast_read_range(mapname, "", &range) <= 0)
	    G_fatal_error(_("Unable to read range for %s"), mapname);

	Rast_get_range_min_max(&range, &xmin, &xmax);

	max = xmax;
	min = xmin;
    }
    else {
	struct FPRange fprange;

	if (Rast_read_fp_range(mapname, "", &fprange) <= 0)
	    G_fatal_error(_("Unable to read FP range for %s"), mapname);

	Rast_get_fp_range_min_max(&fprange, &min, &max);
    }
}

static void plot_axes(void)
{
    char str[64];
    double scale;
    double t, b, l, r;

    D_use_color(D_translate_color("red"));

    D_begin();
    D_move_abs(0, 1);
    D_cont_abs(0, 0);
    D_cont_abs(1, 0);
    D_end();
    D_stroke();

    D_use_color(D_translate_color(DEFAULT_FG_COLOR));

    /* set text size for y-axis labels */
    scale = fabs(D_get_u_to_d_yconv());
    D_text_size(scale * 0.04, scale * 0.05);

    /* plot y-axis label (bottom) */
    sprintf(str, "%.1f", min);
    D_get_text_box(str, &t, &b, &l, &r);
    D_pos_abs(-0.02 - (r - l), 0 - (t - b) / 2);
    D_text(str);

    /* plot y-axis label (top) */
    sprintf(str, "%.1f", max);
    D_get_text_box(str, &t, &b, &l, &r);
    D_pos_abs(-0.02 - (r - l), 1 - (t - b) / 2);
    D_text(str);
}

static int get_cell(DCELL *result, int fd, double x, double y)
{
    static DCELL *row1, *row2;
    static int cur_row = -1;
    static int row, col;
    DCELL *tmp;

    if (!row1) {
	row1 = Rast_allocate_d_buf();
	row2 = Rast_allocate_d_buf();
    }

    col = (int)floor(x - 0.5);
    row = (int)floor(y - 0.5);
    x -= col + 0.5;
    y -= row + 0.5;

    if (row < 0 || row + 1 >= Rast_window_rows() ||
	col < 0 || col + 1 >= Rast_window_cols()) {
	Rast_set_d_null_value(result, 1);
	return 0;
    }

    if (cur_row != row) {
	if (cur_row == row + 1) {
	    tmp = row1; row1 = row2; row2 = tmp;
	    Rast_get_d_row(fd, row1, row);
	}
	else if (cur_row == row - 1) {
	    tmp = row1; row1 = row2; row2 = tmp;
	    Rast_get_d_row(fd, row2, row + 1);
	}
	else {
	    Rast_get_d_row(fd, row1, row);
	    Rast_get_d_row(fd, row2, row + 1);
	}
	cur_row = row;
    }

    if (Rast_is_d_null_value(&row1[col]) || Rast_is_d_null_value(&row1[col+1]) ||
	Rast_is_d_null_value(&row2[col]) || Rast_is_d_null_value(&row2[col+1])) {
	Rast_set_d_null_value(result, 1);
	return 0;
    }

    *result = Rast_interp_bilinear(x, y,
				row1[col], row1[col+1],
				row2[col], row2[col+1]);

    return 1;
}

int main(int argc, char **argv)
{
    struct GModule *module;
    struct Option *map, *profile;
    struct Flag *stored;
    struct Cell_head window;
    struct point *points = NULL;
    int num_points, max_points = 0;
    double length;
    double t, b, l, r;
    int fd;
    int i;
    double sx;
    int last;

    /* Initialize the GIS calls */
    G_gisinit(argv[0]);

    /* Set description */
    module = G_define_module();
    G_add_keyword(_("display"));
    G_add_keyword(_("profile"));
    G_add_keyword(_("raster"));
    module->description = _("Plots profile of a transect.");

    /* set up command line */
    map = G_define_standard_option(G_OPT_R_MAP);
    map->description = _("Raster map to be profiled");

    profile = G_define_standard_option(G_OPT_M_COORDS);
    profile->required = YES;
    profile->multiple = YES;
    profile->description = _("Profile coordinate pairs");

    stored = G_define_flag();
    stored->key = 'r';
    stored->description = _("Use map's range recorded range");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    mapname = map->answer;

    fd = Rast_open_old(mapname, "");

    if (stored->answer)
	get_map_range();
    else
	get_region_range(fd);

    G_get_window(&window);

    num_points = 0;
    length = 0;
    for (i = 0; profile->answers[i]; i += 2) {
	struct point *p;
	double x, y;

	if (num_points >= max_points) {
	    max_points = num_points + 100;
	    points = G_realloc(points, max_points * sizeof(struct point));
	}

	p = &points[num_points];

	G_scan_easting( profile->answers[i+0], &x, G_projection());
	G_scan_northing(profile->answers[i+1], &y, G_projection());

	p->x = Rast_easting_to_col (x, &window);
	p->y = Rast_northing_to_row(y, &window);

	if (num_points > 0) {
	    const struct point *prev = &points[num_points-1];
	    double dx = fabs(p->x - prev->x);
	    double dy = fabs(p->y - prev->y);
	    double d = sqrt(dx * dx + dy * dy);
	    length += d;
	    p->d = length;
	}

	num_points++;
    }
    points[0].d = 0;

    if (num_points < 2)
	G_fatal_error(_("At least two points are required"));

    /* establish connection with graphics driver */
    D_open_driver();
    
    D_setup2(1, 0, 1.05, -0.05, -0.15, 1.05);

    plot_axes();

    D_use_color(D_translate_color(DEFAULT_FG_COLOR));

    D_get_src(&t, &b, &l, &r);
    t -= 0.1 * (t - b);
    b += 0.1 * (t - b);
    l += 0.1 * (r - l);
    r -= 0.1 * (r - l);

    D_begin();

    i = 0;
    last = 0;
    for (sx = 0; sx < 1; sx += D_get_d_to_u_xconv()) {
	double d = length * (sx - l);
	const struct point *p, *next;
	double k, sy, x, y;
	DCELL v;

	for (;;) {
	    p = &points[i];
	    next = &points[i + 1];
	    k = (d - p->d) / (next->d - p->d);
	    if (k < 1)
		break;
	    i++;
	}

	x = p->x * (1 - k) + next->x * k;
	y = p->y * (1 - k) + next->y * k;

	if (!get_cell(&v, fd, x, y)) {
	    last = 0;
	    continue;
	}

	sy = (v - min) / (max - min);

	if (last)
	    D_cont_abs(sx, sy);
	else
	    D_move_abs(sx, sy);

	last = 1;
    }

    D_end();
    D_stroke();
    
    D_close_driver();

    exit(EXIT_SUCCESS);
}

