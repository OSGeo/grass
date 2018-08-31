
/***************************************************************
 *
 * MODULE:       v.neighbors
 * 
 * AUTHOR(S):    Radim Blazek, original code taken from r.neighbors/main.c
 *               OGR support by Martin Landa <landa.martin gmail.com> (2009)
 *               
 * PURPOSE:      Category manipulations
 *               
 * COPYRIGHT:    (C) 2001-2014 by the GRASS Development Team
 *
 *               This program is free software under the GNU General
 *               Public License (>=v2).  Read the file COPYING that
 *               comes with GRASS for details.
 *
 **************************************************************/

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/vector.h>
#include <grass/glocale.h>

int main(int argc, char *argv[])
{
    int out_fd;
    CELL *result, *rp;
    int nrows, ncols;
    int row, col, count_sum;
    int field;
    struct GModule *module;
    struct Option *in_opt, *out_opt, *field_opt;
    struct Option *method_opt, *size_opt;
    struct Map_info In;
    double radius, dia;
    struct boxlist *List;
    struct Cell_head region;
    struct bound_box box;
    struct line_pnts *Points;
    struct line_cats *Cats;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("algebra"));
    G_add_keyword(_("statistics"));
    G_add_keyword(_("raster"));
    G_add_keyword(_("aggregation"));
    module->label = _("Neighborhood analysis tool for vector point maps.");
    module->description = _("Makes each cell value a "
			    "function of the attribute values assigned to the vector points or centroids "
			    "around it, and stores new cell values in an output raster map.");

    in_opt = G_define_standard_option(G_OPT_V_INPUT);

    field_opt = G_define_standard_option(G_OPT_V_FIELD_ALL);

    out_opt = G_define_standard_option(G_OPT_R_OUTPUT);

    method_opt = G_define_option();
    method_opt->key = "method";
    method_opt->type = TYPE_STRING;
    method_opt->required = YES;
    method_opt->options = "count";
    method_opt->answer = "count";
    method_opt->description = _("Neighborhood operation");

    size_opt = G_define_option();
    size_opt->key = "size";
    size_opt->type = TYPE_DOUBLE;
    size_opt->required = YES;
    size_opt->description = _("Neighborhood diameter in map units");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    radius = atof(size_opt->answer) / 2;

    /* open input vector */
    Vect_set_open_level(2);
    if (Vect_open_old2(&In, in_opt->answer, "", field_opt->answer) < 0)
	G_fatal_error(_("Unable to open vector map <%s>"), in_opt->answer);

    field = Vect_get_field_number(&In, field_opt->answer);

    G_get_set_window(&region);
    Vect_get_map_box(&In, &box);
    
    if (box.N > region.north + radius || box.S < region.south - radius ||
        box.E > region.east + radius || box.W < region.west - radius) {
	Vect_close(&In);
	G_warning(_("Input vector and computational region do not overlap"));
    }

    dia = sqrt(region.ns_res * region.ns_res + region.ew_res * region.ew_res);
    if (radius * 2.0 < dia) {
	G_warning(_("The search diameter %g is smaller than cell diagonal %g: some points could not be detected"),
	          radius * 2, dia);
    }

    nrows = Rast_window_rows();
    ncols = Rast_window_cols();

    result = Rast_allocate_buf(CELL_TYPE);
    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();
    List = Vect_new_boxlist(0);

    /*open the new cellfile */
    out_fd = Rast_open_new(out_opt->answer, CELL_TYPE);

    box.T = PORT_DOUBLE_MAX;
    box.B = -PORT_DOUBLE_MAX;

    count_sum = 0;
    for (row = 0; row < nrows; row++) {
	double x, y;

	G_percent(row, nrows, 2);

	y = Rast_row_to_northing(row + 0.5, &region);
	box.N = y + radius;
	box.S = y - radius;

	Rast_set_null_value(result, ncols, CELL_TYPE);
	rp = result;

	for (col = 0; col < ncols; col++) {
	    int i, count;
	    CELL value;

	    x = Rast_col_to_easting(col + 0.5, &region);

	    box.E = x + radius;
	    box.W = x - radius;

	    Vect_select_lines_by_box(&In, &box, GV_POINTS, List);
	    G_debug(3, "  %d lines in box", List->n_values);

	    count = 0;

	    for (i = 0; i < List->n_values; i++) {
		Vect_read_line(&In, Points, Cats, List->id[i]);

		if (field != -1 && Vect_cat_get(Cats, field, NULL) == 0)
		    continue;
		
		if (Vect_points_distance(x, y, 0.0, Points->x[0],
					 Points->y[0], 0.0, 0) <= radius)
		    count++;
	    }

	    if (count > 0) {
		value = count;
		Rast_set_d_value(rp, value, CELL_TYPE);
	    }
	    rp = G_incr_void_ptr(rp, Rast_cell_size(CELL_TYPE));
	    count_sum += count;
	}

	Rast_put_row(out_fd, result, CELL_TYPE);
    }
    G_percent(1, 1, 1);

    Vect_close(&In);
    Rast_close(out_fd);

    if (count_sum < 1) 
	G_warning(_("No points found")); 

    exit(EXIT_SUCCESS);
}
