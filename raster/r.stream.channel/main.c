
/****************************************************************************
 *
 * MODULE:       r.stream.channel
 * AUTHOR(S):    Jarek Jasiewicz jarekj amu.edu.pl
 *               
 * PURPOSE:      Program calculate some channel properties:
 * 			local elevation change, curvature along stream,
 * 			distance to channel init/join, elevation below channel init, 
 * 			optionally distance to outlet, elevation above outlet;
        
 * COPYRIGHT:    (C) 2002,2010-2014 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *   	    	 License (>=v2). Read the file COPYING that comes with GRASS
 *   	    	 for details.
 *
 *****************************************************************************/
#define MAIN
#include <grass/glocale.h>
#include "local_proto.h"

int nextr[9] = { 0, -1, -1, -1, 0, 1, 1, 1, 0 };
int nextc[9] = { 0, 1, 0, -1, -1, -1, 0, 1, 1 };

int main(int argc, char *argv[])
{
    /*
       IO output[] = {
       {"local_diff",NO,"Local elevation difference"},
       {"out_dist",NO,"Upstream distance form init"},
     */

    struct GModule *module;
    struct Option *in_dir_opt,	/* options */
     *in_stm_opt,
	*in_elev_opt,
	*out_identifier_opt,
	*out_distance_opt,
	*out_difference_opt,
	*out_gradient_opt, *out_curvature_opt, *opt_swapsize;

    struct Flag *flag_segmentation,
	*flag_local, *flag_cells, *flag_downstream;

    char *method_name[] = { "UPSTREAM", "DOWNSTREAM" };
    int number_of_segs;
    int number_of_streams;
    int segmentation, downstream, local, cells;	/*flags */

    /* initialize GIS environment */
    G_gisinit(argv[0]);

    /* initialize module */
    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("hydrology"));
    G_add_keyword(_("stream network"));
    module->description =
	_("Calculates local parameters for individual streams.");

    in_stm_opt = G_define_standard_option(G_OPT_R_INPUT);
    in_stm_opt->key = "stream_rast";
    in_stm_opt->description = _("Name for input raster map with stream network");

    in_dir_opt = G_define_standard_option(G_OPT_R_INPUT);
    in_dir_opt->key = "direction";
    in_dir_opt->description = _("Name for input raster map with flow direction");

    in_elev_opt = G_define_standard_option(G_OPT_R_ELEV);

    out_identifier_opt = G_define_standard_option(G_OPT_R_OUTPUT);
    out_identifier_opt->key = "identifier";
    out_identifier_opt->required = NO;
    out_identifier_opt->description = _("Name for output unique stream identifier raster map");
    out_identifier_opt->guisection = _("Output maps");

    out_distance_opt = G_define_standard_option(G_OPT_R_OUTPUT);
    out_distance_opt->key = "distance";
    out_distance_opt->required = NO;
    out_distance_opt->description = _("Name for output init/join/outlet distance raster map");
    out_distance_opt->guisection = _("Output maps");

    out_difference_opt = G_define_standard_option(G_OPT_R_OUTPUT);
    out_difference_opt->key = "difference";
    out_difference_opt->required = NO;
    out_difference_opt->description =
        _("Name for output elevation init/join/outlet difference raster map");
    out_difference_opt->guisection = _("Output maps");

    out_gradient_opt = G_define_standard_option(G_OPT_R_OUTPUT);
    out_gradient_opt->key = "gradient";
    out_gradient_opt->required = NO;
    out_gradient_opt->description =
	_("Name for output mean init/join/outlet gradient of stream raster map");
    out_gradient_opt->guisection = _("Output maps");

    out_curvature_opt = G_define_standard_option(G_OPT_R_OUTPUT);
    out_curvature_opt->key = "curvature";
    out_curvature_opt->required = NO;
    out_curvature_opt->description = _("Name for output local stream curvature raster map");
    out_curvature_opt->guisection = _("Output maps");

    opt_swapsize = G_define_option();
    opt_swapsize->key = "memory";
    opt_swapsize->type = TYPE_INTEGER;
    opt_swapsize->answer = "300";
    opt_swapsize->description = _("Max memory used in memory swap mode (MB)");
    opt_swapsize->guisection = _("Memory settings");

    flag_downstream = G_define_flag();
    flag_downstream->key = 'd';
    flag_downstream->description =
	_("Calculate parameters from outlet (downstream values)");

    flag_local = G_define_flag();
    flag_local->key = 'l';
    flag_local->description = _("Calculate local values (for current cell)");

    flag_cells = G_define_flag();
    flag_cells->key = 'c';
    flag_cells->description = _("Calculate distance in cell count (ignored local)");

    flag_segmentation = G_define_flag();
    flag_segmentation->key = 'm';
    flag_segmentation->description = _("Use memory swap (operation is slow)");
    flag_segmentation->guisection = _("Memory settings");

    if (G_parser(argc, argv))	/* parser */
	exit(EXIT_FAILURE);

    segmentation = (flag_segmentation->answer != 0);
    downstream = (flag_downstream->answer != 0);

    if (!out_identifier_opt->answer &&
	!out_distance_opt->answer &&
	!out_difference_opt->answer &&
	!out_gradient_opt->answer && !out_curvature_opt->answer)
	G_fatal_error(_("You must select at least one output raster maps"));

    local = (flag_local->answer != 0);
    cells = (flag_cells->answer != 0);
    nrows = Rast_window_rows();
    ncols = Rast_window_cols();
    G_get_window(&window);
    G_begin_distance_calculations();

    if (!segmentation) {
	MAP map_dirs, map_streams, map_elevation, map_output, map_identifier;
	CELL **streams, **dirs, **identifier = NULL;
	FCELL **elevation;
	DCELL **output;

	G_message(_("All in RAM calculation - direction <%s>..."),
		  method_name[downstream]);
	ram_create_map(&map_streams, CELL_TYPE);
	ram_read_map(&map_streams, in_stm_opt->answer, 1, CELL_TYPE);
	ram_create_map(&map_dirs, CELL_TYPE);
	ram_read_map(&map_dirs, in_dir_opt->answer, 1, CELL_TYPE);
	ram_create_map(&map_elevation, FCELL_TYPE);
	ram_read_map(&map_elevation, in_elev_opt->answer, 0, -1);

	streams = (CELL **) map_streams.map;
	dirs = (CELL **) map_dirs.map;
	elevation = (FCELL **) map_elevation.map;

	number_of_streams = ram_number_of_streams(streams, dirs) + 1;
	ram_build_streamlines(streams, dirs, elevation, number_of_streams);
	ram_release_map(&map_streams);
	ram_release_map(&map_dirs);
	ram_create_map(&map_output, DCELL_TYPE);
	output = (DCELL **) map_output.map;	/* one output for all maps */

	if (out_identifier_opt->answer) {
	    ram_create_map(&map_identifier, CELL_TYPE);
	    identifier = (CELL **) map_identifier.map;
	    ram_calculate_identifiers(identifier, number_of_streams,
				      downstream);
	    ram_write_map(&map_identifier, out_identifier_opt->answer,
			  CELL_TYPE, 1, 0);
	    ram_release_map(&map_identifier);
	}

	if (out_difference_opt->answer) {
	    ram_set_null_output(output);
	    if (local)
		ram_calculate_difference(output, number_of_streams,
					 downstream);
	    else
		ram_calculate_drop(output, number_of_streams, downstream);
	    ram_write_map(&map_output, out_difference_opt->answer, DCELL_TYPE,
			  0, 0);
	}

	if (out_distance_opt->answer) {
	    ram_set_null_output(output);
	    if (local && !cells)
		ram_calculate_local_distance(output, number_of_streams,
					     downstream);
	    if (!local && !cells)
		ram_calculate_distance(output, number_of_streams, downstream);
	    if (cells)
		ram_calculate_cell(output, number_of_streams, downstream);
	    ram_write_map(&map_output, out_distance_opt->answer, DCELL_TYPE,
			  0, 0);
	}

	if (out_gradient_opt->answer) {
	    ram_set_null_output(output);
	    if (local)
		ram_calculate_local_gradient(output, number_of_streams,
					     downstream);
	    else
		ram_calculate_gradient(output, number_of_streams, downstream);
	    ram_write_map(&map_output, out_gradient_opt->answer, DCELL_TYPE,
			  0, 0);
	}

	if (out_curvature_opt->answer) {
	    ram_set_null_output(output);
	    ram_calculate_curvature(output, number_of_streams, downstream);
	    ram_write_map(&map_output, out_curvature_opt->answer, DCELL_TYPE,
			  0, 0);
	}

	ram_release_map(&map_output);
    }


    if (segmentation) {
	SEG map_dirs, map_streams, map_elevation, map_output, map_identifier;
	SEGMENT *streams, *dirs, *elevation, *output, *identifier;

	G_message(_("Calculating segments in direction <%s> (may take some time)..."),
		  method_name[downstream]);

	number_of_segs = (int)atof(opt_swapsize->answer);
	number_of_segs = number_of_segs < 32 ? (int)(32 / 0.18) : number_of_segs / 0.18;

	seg_create_map(&map_streams, SROWS, SCOLS, number_of_segs, CELL_TYPE);
	seg_read_map(&map_streams, in_stm_opt->answer, 1, CELL_TYPE);
	seg_create_map(&map_dirs, SROWS, SCOLS, number_of_segs, CELL_TYPE);
	seg_read_map(&map_dirs, in_dir_opt->answer, 1, CELL_TYPE);
	seg_create_map(&map_elevation, SROWS, SCOLS, number_of_segs,
		       FCELL_TYPE);
	seg_read_map(&map_elevation, in_elev_opt->answer, 0, -1);

	streams = &map_streams.seg;
	dirs = &map_dirs.seg;
	elevation = &map_elevation.seg;

	number_of_streams = seg_number_of_streams(streams, dirs) + 1;
	seg_build_streamlines(streams, dirs, elevation, number_of_streams);
	seg_release_map(&map_streams);
	seg_release_map(&map_dirs);
	seg_create_map(&map_output, SROWS, SCOLS, number_of_segs, DCELL_TYPE);
	output = &map_output.seg;	/* one output for all maps */

	if (out_identifier_opt->answer) {
	    seg_create_map(&map_identifier, SROWS, SCOLS, number_of_segs,
			   CELL_TYPE);
	    identifier = &map_identifier.seg;
	    seg_calculate_identifiers(identifier, number_of_streams,
				      downstream);
	    seg_write_map(&map_identifier, out_identifier_opt->answer,
			  CELL_TYPE, 1, 0);
	    seg_release_map(&map_identifier);
	}

	if (out_difference_opt->answer) {
	    seg_set_null_output(output);
	    if (local)
		seg_calculate_difference(output, number_of_streams,
					 downstream);
	    else
		seg_calculate_drop(output, number_of_streams, downstream);
	    seg_write_map(&map_output, out_difference_opt->answer, DCELL_TYPE,
			  0, 0);
	}

	if (out_distance_opt->answer) {
	    seg_set_null_output(output);
	    if (local && !cells)
		seg_calculate_local_distance(output, number_of_streams,
					     downstream);
	    if (!local && !cells)
		seg_calculate_distance(output, number_of_streams, downstream);
	    if (cells)
		seg_calculate_cell(output, number_of_streams, downstream);
	    seg_write_map(&map_output, out_distance_opt->answer, DCELL_TYPE,
			  0, 0);
	}

	if (out_gradient_opt->answer) {
	    seg_set_null_output(output);
	    if (local)
		seg_calculate_local_gradient(output, number_of_streams,
					     downstream);
	    else
		seg_calculate_gradient(output, number_of_streams, downstream);
	    seg_write_map(&map_output, out_gradient_opt->answer, DCELL_TYPE,
			  0, 0);
	}

	if (out_curvature_opt->answer) {
	    seg_set_null_output(output);
	    seg_calculate_curvature(output, number_of_streams, downstream);
	    seg_write_map(&map_output, out_curvature_opt->answer, DCELL_TYPE,
			  0, 0);
	}

	seg_release_map(&map_output);
    }
    free_attributes(number_of_streams);
    exit(EXIT_SUCCESS);
}
