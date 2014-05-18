
/****************************************************************************
 *
 * MODULE:       r.stream.slope
 * AUTHOR(S):    Jarek Jasiewicz jarekj amu.edu.pl
 *               
 * PURPOSE:      Supplementary module for r.stream.distance for slope subsystem, 
 * 		 calculate local downstream elevation change 
 * 		 and local downstream minimum and maximum curvature
        
 * COPYRIGHT:    (C) 2002, 2010-2014 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *   	    	 License (>=v2). Read the file COPYING that comes with GRASS
 *   	         for details.
 *
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#define MAIN


#define SQRT2 1.414214
#define NR(x) r + nextr[(x)]
#define NC(x) c + nextc[(x)]
#define DIAG(x) (((x) + 4) > 8 ? ((x) - 4) : ((x) + 4))
#define NOT_IN_REGION(x) (r+nextr[(x)] < 0 || r+nextr[(x)] > 2 || c+nextc[(x)] < 0 || c+nextc[(x)] > (ncols-1))

int nextr[9] = { 0, -1, -1, -1, 0, 1, 1, 1, 0 };
int nextc[9] = { 0, 1, 0, -1, -1, -1, 0, 1, 1 };

int nrows, ncols;
CELL **dir_rows;
DCELL **elev_rows;
struct Cell_head window;

DCELL calculate_difference(int r, int c);
DCELL calculate_gradient(int r, int c);
DCELL calculate_max_curvature(int r, int c);
DCELL calculate_min_curvature(int r, int c);

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct Option *in_dir_opt,	/* options */
     *in_elev_opt,
	*out_differnce_opt,
	*out_gradient_opt, *out_max_curv_opt, *out_min_curv_opt;
    struct Cell_head cellhd;
    struct History history;

    int r, c, /* d, */ i, cur_row;
    int elev_map_type, elev_data_size;
    /* int gradient; */

    int in_dir_fd, in_elev_fd;
    int out_difference_fd, out_gradient_fd, out_max_curv_fd, out_min_curv_fd;
    /* double cellsize; */
    char *mapset;
    void *tmp_buffer;
    DCELL *tmp_elev_buf;
    CELL *tmp_dir_buf;
    DCELL *out_difference_buf, *out_gradient_buf, *out_max_curv_buf,
	*out_min_curv_buf;

    G_gisinit(argv[0]);
    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("hydrology"));
    G_add_keyword(_("stream network"));
    G_add_keyword(_("stream local parameters"));
    module->description = _("Calculates local parameters for slope subsystem.");

    in_dir_opt = G_define_standard_option(G_OPT_R_INPUT);
    in_dir_opt->key = "direction";
    in_dir_opt->description = _("Name of input raster map with flow direction");

    in_elev_opt = G_define_standard_option(G_OPT_R_INPUT);

    out_differnce_opt = G_define_standard_option(G_OPT_R_OUTPUT);
    out_differnce_opt->key = "difference";
    out_differnce_opt->required = NO;
    out_differnce_opt->description =
      _("Name for output local downstream elevation difference raster map");
    out_differnce_opt->guisection = _("Output maps");

    out_gradient_opt = G_define_standard_option(G_OPT_R_OUTPUT);
    out_gradient_opt->key = "gradient";
    out_gradient_opt->required = NO;
    out_gradient_opt->description = _("Name for output local downstream gradient raster map");
    out_gradient_opt->guisection = _("Output maps");

    out_max_curv_opt = G_define_standard_option(G_OPT_R_OUTPUT);
    out_max_curv_opt->key = "maxcurv";
    out_max_curv_opt->required = NO;
    out_max_curv_opt->description =
        _("Name for output local downstream maximum curvature raster map");
    out_max_curv_opt->guisection = _("Output maps");

    out_min_curv_opt = G_define_standard_option(G_OPT_R_OUTPUT);
    out_min_curv_opt->key = "mincurv";
    out_min_curv_opt->required = NO;
    out_min_curv_opt->description =
	_("Name for output local downstream minimum curvature raster map");
    out_min_curv_opt->guisection = _("Output maps");

    if (G_parser(argc, argv))	/* parser */
	exit(EXIT_FAILURE);

    /* open map */
    mapset = (char *)G_find_raster2(in_dir_opt->answer, "");
    if (mapset == NULL)
	G_fatal_error(_("Raster map <%s> not found"), in_dir_opt->answer);

    G_get_window(&window);
    Rast_get_cellhd(in_dir_opt->answer, mapset, &cellhd);
    if (window.ew_res != cellhd.ew_res || window.ns_res != cellhd.ns_res)
          G_fatal_error(_("Region resolution and raster map <%s> resolution differs. "
                          "Run 'g.region rast=%s' to set proper region resolution."),
                        in_dir_opt->answer, in_dir_opt->answer);

    if (Rast_map_type(in_dir_opt->answer, mapset) != CELL_TYPE)
	G_fatal_error(_("Raster <%s> is not of type CELL"), in_dir_opt->answer);

    in_dir_fd = Rast_open_old(in_dir_opt->answer, mapset);

    mapset = (char *)G_find_raster2(in_elev_opt->answer, "");
    if (mapset == NULL)
	G_fatal_error(_("Raster map <%s> not found"), in_elev_opt->answer);

    elev_map_type = Rast_map_type(in_elev_opt->answer, mapset);
    elev_data_size = Rast_cell_size(elev_map_type);
    in_elev_fd = Rast_open_old(in_elev_opt->answer, mapset);

    nrows = Rast_window_rows();
    ncols = Rast_window_cols();

    G_begin_distance_calculations();

    if (out_differnce_opt->answer) {
	out_difference_fd =
	    Rast_open_new(out_differnce_opt->answer, DCELL_TYPE);
	out_difference_buf = Rast_allocate_d_buf();
    }

    if (out_gradient_opt->answer) {
	out_gradient_fd = Rast_open_new(out_gradient_opt->answer, DCELL_TYPE);
	out_gradient_buf = Rast_allocate_d_buf();
    }

    if (out_max_curv_opt->answer) {
	out_max_curv_fd = Rast_open_new(out_max_curv_opt->answer, DCELL_TYPE);
	out_max_curv_buf = Rast_allocate_d_buf();
    }

    if (out_min_curv_opt->answer) {
	out_min_curv_fd = Rast_open_new(out_min_curv_opt->answer, DCELL_TYPE);
	out_min_curv_buf = Rast_allocate_d_buf();
    }

    dir_rows = (CELL **) G_malloc(3 * sizeof(CELL *));
    elev_rows = (DCELL **) G_malloc(3 * sizeof(DCELL *));

    /* init shift buffer */
    tmp_buffer = Rast_allocate_buf(elev_map_type);

    for (i = 0; i < 3; ++i) {
	dir_rows[i] = Rast_allocate_c_buf();
	elev_rows[i] = Rast_allocate_d_buf();
	Rast_get_row(in_dir_fd, dir_rows[i], i, CELL_TYPE);
	Rast_get_row(in_elev_fd, tmp_buffer, i, elev_map_type);
	for (c = 0; c < ncols; ++c)
	    elev_rows[i][c] =
		Rast_get_d_value(tmp_buffer + c * elev_data_size,
				 elev_map_type);
    }

    for (r = 0; r < nrows; ++r) {	/*main loop */

	G_percent(r, nrows, 2);

	if (r == 0)
	    cur_row = 0;
	else if (r == (nrows - 1))
	    cur_row = 2;
	else
	    cur_row = 1;

	for (c = 0; c < ncols; ++c) {
	    if (out_differnce_opt->answer)
		out_difference_buf[c] = calculate_difference(cur_row, c);
	    if (out_gradient_opt->answer)
		out_gradient_buf[c] = calculate_gradient(cur_row, c);
	    if (out_max_curv_opt->answer)
		out_max_curv_buf[c] = calculate_max_curvature(cur_row, c);
	    if (out_min_curv_opt->answer)
		out_min_curv_buf[c] = calculate_min_curvature(cur_row, c);
	}

	if (out_differnce_opt->answer)
	    Rast_put_row(out_difference_fd, out_difference_buf, DCELL_TYPE);
	if (out_gradient_opt->answer)
	    Rast_put_row(out_gradient_fd, out_gradient_buf, DCELL_TYPE);
	if (out_max_curv_opt->answer)
	    Rast_put_row(out_max_curv_fd, out_max_curv_buf, DCELL_TYPE);
	if (out_min_curv_opt->answer)
	    Rast_put_row(out_min_curv_fd, out_min_curv_buf, DCELL_TYPE);

	/* shift buffer */

	if (r != 0 && r < nrows - 2) {

	    tmp_elev_buf = elev_rows[0];
	    tmp_dir_buf = dir_rows[0];

	    for (i = 1; i < 3; ++i) {
		dir_rows[i - 1] = dir_rows[i];
		elev_rows[i - 1] = elev_rows[i];
	    }

	    dir_rows[2] = tmp_dir_buf;
	    elev_rows[2] = tmp_elev_buf;
	    Rast_get_row(in_dir_fd, dir_rows[2], r + 2, CELL_TYPE);
	    Rast_get_row(in_elev_fd, tmp_buffer, r + 2, elev_map_type);

	    for (c = 0; c < ncols; ++c)
		elev_rows[2][c] =
		    Rast_get_d_value(tmp_buffer + c * elev_data_size,
				     elev_map_type);
	}
    }
    G_percent(r, nrows, 2);

    if (out_differnce_opt->answer) {
	G_free(out_difference_buf);
	Rast_close(out_difference_fd);
	Rast_short_history(out_differnce_opt->answer, "raster", &history);
	Rast_command_history(&history);
	Rast_write_history(out_differnce_opt->answer, &history);
    }

    if (out_gradient_opt->answer) {
	G_free(out_gradient_buf);
	Rast_close(out_gradient_fd);
	Rast_short_history(out_gradient_opt->answer, "raster", &history);
	Rast_command_history(&history);
	Rast_write_history(out_gradient_opt->answer, &history);
    }

    if (out_max_curv_opt->answer) {
	G_free(out_max_curv_buf);
	Rast_close(out_max_curv_fd);
	Rast_short_history(out_max_curv_opt->answer, "raster", &history);
	Rast_command_history(&history);
	Rast_write_history(out_max_curv_opt->answer, &history);
    }

    if (out_min_curv_opt->answer) {
	G_free(out_min_curv_buf);
	Rast_close(out_min_curv_fd);
	Rast_short_history(out_min_curv_opt->answer, "raster", &history);
	Rast_command_history(&history);
	Rast_write_history(out_min_curv_opt->answer, &history);
    }

    exit(EXIT_SUCCESS);
}

DCELL calculate_difference(int r, int c)
{
    int d;

    d = dir_rows[r][c];

    if (NOT_IN_REGION(d))
	return 0.;
    return elev_rows[r][c] - elev_rows[NR(d)][NC(d)];
}


DCELL calculate_gradient(int r, int c)
{

    int d;
    double easting, northing, next_easting, next_northing;
    double distance;

    d = dir_rows[r][c];

    if (NOT_IN_REGION(d))
	return 0.;

    northing = window.north - (r + .5) * window.ns_res;
    easting = window.west + (c + .5) * window.ew_res;
    next_northing = window.north - (NR(d) + .5) * window.ns_res;
    next_easting = window.west + (NC(d) + .5) * window.ew_res;
    distance = G_distance(easting, northing, next_easting, next_northing);

    return (elev_rows[r][c] - elev_rows[NR(d)][NC(d)]) / distance;
}

DCELL calculate_max_curvature(int r, int c)
{

    int i, j = 0, d;
    double easting, northing, next_easting, next_northing;
    double elev_max = -1000;
    double diff_up, diff_down, diff_elev, first_derivative, second_derivative;
    double distance_up, distance_down, distance;

    d = dir_rows[r][c];

    if (NOT_IN_REGION(d))
	return 0.;

    for (i = 1; i < 9; ++i) {
	if (NOT_IN_REGION(i))
	    continue;
	if (dir_rows[NR(i)][NC(i)] == DIAG(i) &&
	    elev_rows[NR(i)][NC(i)] > elev_max) {
	    elev_max = elev_rows[NR(i)][NC(i)];
	    j = i;
	}
    }
    if (elev_max == -1000)
	elev_max = elev_rows[r][c];
    diff_up = elev_max - elev_rows[r][c];
    diff_down = elev_rows[r][c] - elev_rows[NR(d)][NC(d)];
    diff_elev = elev_max - elev_rows[NR(d)][NC(d)];
    if (diff_elev < 0)
	diff_elev = 0;

    northing = window.north - (r + .5) * window.ns_res;
    easting = window.west + (c + .5) * window.ew_res;
    next_northing = window.north - (NR(j) + .5) * window.ns_res;
    next_easting = window.west + (NC(j) + .5) * window.ew_res;
    distance_up = G_distance(easting, northing, next_easting, next_northing);

    northing = window.north - (r + .5) * window.ns_res;
    easting = window.west + (c + .5) * window.ew_res;
    next_northing = window.north - (NR(d) + .5) * window.ns_res;
    next_easting = window.west + (NC(d) + .5) * window.ew_res;
    distance_down =
	G_distance(easting, northing, next_easting, next_northing);
    distance = distance_up + distance_down;
    first_derivative = diff_elev / distance;
    second_derivative = (diff_up - diff_down) / distance;

    return second_derivative / pow((1 + first_derivative * first_derivative),
				   1.5);

}

DCELL calculate_min_curvature(int r, int c)
{
    int i, j = 0, d;
    /* int next_r, next_c; */
    double easting, northing, next_easting, next_northing;
    double elev_min = 9999;
    double diff_up, diff_down, diff_elev, first_derivative, second_derivative;
    double distance_up, distance_down, distance;

    d = dir_rows[r][c];

    if (NOT_IN_REGION(d))
	return 0.;

    for (i = 1; i < 9; ++i) {
	if (NOT_IN_REGION(i))
	    continue;
	if (dir_rows[NR(i)][NC(i)] == DIAG(i) &&
	    elev_rows[NR(i)][NC(i)] < elev_min) {
	    elev_min = elev_rows[NR(i)][NC(i)];
	    j = i;
	}
    }


    if (elev_min == 9999)
	elev_min = elev_rows[r][c];
    diff_up = elev_min - elev_rows[r][c];
    diff_down = elev_rows[r][c] - elev_rows[NR(d)][NC(d)];
    diff_elev = elev_min - elev_rows[NR(d)][NC(d)];

    northing = window.north - (r + .5) * window.ns_res;
    easting = window.west + (c + .5) * window.ew_res;
    next_northing = window.north - (NR(j) + .5) * window.ns_res;
    next_easting = window.west + (NC(j) + .5) * window.ew_res;
    distance_up = G_distance(easting, northing, next_easting, next_northing);

    northing = window.north - (r + .5) * window.ns_res;
    easting = window.west + (c + .5) * window.ew_res;
    next_northing = window.north - (NR(d) + .5) * window.ns_res;
    next_easting = window.west + (NC(d) + .5) * window.ew_res;
    distance_down =
	G_distance(easting, northing, next_easting, next_northing);

    distance = distance_up + distance_down;
    first_derivative = diff_elev / distance;
    second_derivative = (diff_up - diff_down) / distance;
    return second_derivative / pow((1 + first_derivative * first_derivative),
				   1.5);

}
