
/***************************************************************
 *
 * MODULE:       v.neighbors
 * 
 * AUTHOR(S):    Radim Blazek, original code taken from r.neighbors/main.c
 *               
 * PURPOSE:      Category manipulations
 *               
 * COPYRIGHT:    (C) 2001 by the GRASS Development Team
 *
 *               This program is free software under the 
 *               GNU General Public License (>=v2). 
 *               Read the file COPYING that comes with GRASS
 *               for details.
 *
 **************************************************************/
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/Vect.h>
#include <grass/glocale.h>

/* TODO: add more methods */

int main(int argc, char *argv[])
{
    int out_fd;
    CELL *result, *rp;
    int nrows, ncols;
    int row, col;
    struct GModule *module;
    struct Option *in_opt, *out_opt;
    struct Option *method_opt, *size_opt;
    char *mapset;
    struct Map_info In;
    double radius;
    struct ilist *List;
    struct Cell_head region;
    BOUND_BOX box;
    struct line_pnts *Points;
    struct line_cats *Cats;

    G_gisinit(argv[0]);

    module = G_define_module();
    module->keywords = _("vector, raster, aggregation");
    module->description = "Makes each cell value a "
	"function of the attribute values assigned to the vector points or centroids "
	"around it, and stores new cell values in an output raster map layer.";

    in_opt = G_define_standard_option(G_OPT_V_INPUT);
    out_opt = G_define_standard_option(G_OPT_R_OUTPUT);

    method_opt = G_define_option();
    method_opt->key = "method";
    method_opt->type = TYPE_STRING;
    method_opt->required = YES;
    method_opt->options = "count";
    method_opt->answer = "count";
    method_opt->description = "Neighborhood operation";

    size_opt = G_define_option();
    size_opt->key = "size";
    size_opt->type = TYPE_DOUBLE;
    size_opt->required = YES;
    size_opt->description = "Neighborhood diameter in map units";

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    radius = atof(size_opt->answer) / 2;

    /* open input vector */
    if ((mapset = G_find_vector2(in_opt->answer, "")) == NULL) {
	G_fatal_error(_("Vector map <%s> not found in the current mapset"),
		      in_opt->answer);
    }

    Vect_set_open_level(2);
    Vect_open_old(&In, in_opt->answer, mapset);

    G_get_set_window(&region);
    nrows = G_window_rows();
    ncols = G_window_cols();

    result = G_allocate_raster_buf(CELL_TYPE);
    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();
    List = Vect_new_list();

    /*open the new cellfile */
    out_fd = G_open_raster_new(out_opt->answer, CELL_TYPE);
    if (out_fd < 0)
	G_fatal_error(_("Unable to create raster map <%s>"), out_opt->answer);

    box.T = PORT_DOUBLE_MAX;
    box.B = -PORT_DOUBLE_MAX;

    for (row = 0; row < nrows; row++) {
	double x, y;

	G_percent(row, nrows, 1);

	y = G_row_to_northing(row + 0.5, &region);
	box.N = y + radius;
	box.S = y - radius;

	G_set_null_value(result, ncols, CELL_TYPE);
	rp = result;

	for (col = 0; col < ncols; col++) {
	    int i, count;
	    CELL value;

	    x = G_col_to_easting(col + 0.5, &region);

	    box.E = x + radius;
	    box.W = x - radius;

	    Vect_select_lines_by_box(&In, &box, GV_POINTS, List);
	    G_debug(3, "  %d lines in box", List->n_values);

	    count = 0;

	    for (i = 0; i < List->n_values; i++) {
		double distance;

		Vect_read_line(&In, Points, Cats, List->value[i]);
		distance =
		    Vect_points_distance(x, y, 0.0, Points->x[0],
					 Points->y[0], 0.0, 0);

		if (distance <= radius) {
		    count++;
		}
	    }

	    if (count > 0) {
		value = count;
		G_set_raster_value_d(rp, value, CELL_TYPE);
	    }
	    rp = G_incr_void_ptr(rp, G_raster_size(CELL_TYPE));
	}

	G_put_raster_row(out_fd, result, CELL_TYPE);
    }
    G_percent(row, nrows, 1);

    Vect_close(&In);
    G_close_cell(out_fd);

    exit(EXIT_SUCCESS);
}
