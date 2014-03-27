
/****************************************************************************
 *
 * MODULE:       r.stream.distance
 * AUTHOR(S):    Jarek Jasiewicz jarekj amu.edu.pl
 *               
 * PURPOSE:      Calculate distance and elevation over the streams and outlets 
 *               according user's input data. The elevation and distance is calculated
 *               along watercourses
 *               It uses any stream map  and r.watershed or .stream.extreact direction map.
 *               Stram input map shall contains streams or points outlets with or
 *               without unique categories
 *
 * COPYRIGHT:    (C) 2002,2009-2014 by the GRASS Development Team
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

int process_cat(char **cat_list);
int main(int argc, char *argv[])
{

    struct GModule *module;
    struct Option *in_dir_opt,
	*in_stm_opt,
	*in_elev_opt,
	*in_method_opt, *opt_swapsize, *out_dist_opt, *out_diff_opt;
    struct Flag *flag_outs, *flag_sub, *flag_near, *flag_segmentation;
    char *method_name[] = { "UPSTREAM", "DOWNSTREAM" };
    int method;
    int number_of_segs;
    int outlets_num;
    int number_of_streams;
    int outs, subs, near, segmentation;	/*flags */
    int j;

    G_gisinit(argv[0]);

    module = G_define_module();
    module->label = _("Calculates distance to and elevation above streams and outlets.");
    module->description =
      _("The module can work in stream mode where target are streams and "
        "outlets mode where targets are outlets.");
    G_add_keyword(_("raster"));
    G_add_keyword(_("hydrology"));
    G_add_keyword(_("stream network"));
    G_add_keyword(_("watercourse distance"));

    in_stm_opt = G_define_standard_option(G_OPT_R_INPUT);
    in_stm_opt->key = "stream_rast";
    in_stm_opt->description = _("Name of input streams (outlets) mask raster map");

    in_dir_opt = G_define_standard_option(G_OPT_R_INPUT);
    in_dir_opt->key = "direction";
    in_dir_opt->description = _("Name for input raster map with flow direction");

    in_elev_opt = G_define_standard_option(G_OPT_R_ELEV);
    in_elev_opt->required = NO;
    in_elev_opt->guisection = _("Input maps");

    in_method_opt = G_define_option();
    in_method_opt->key = "method";
    in_method_opt->description = _("Calculation method");
    in_method_opt->type = TYPE_STRING;
    in_method_opt->required = YES;
    in_method_opt->options = "upstream,downstream";
    in_method_opt->answer = "downstream";

    out_dist_opt = G_define_standard_option(G_OPT_R_OUTPUT);
    out_dist_opt->key = "distance";
    out_dist_opt->required = NO;
    out_dist_opt->description = _("Name for output distance/accumulation raster map");
    out_dist_opt->guisection = _("Output maps");

    out_diff_opt = G_define_standard_option(G_OPT_R_OUTPUT);
    out_diff_opt->key = "difference";
    out_diff_opt->required = NO;
    out_diff_opt->description = _("Name for output elevation difference raster map");
    out_diff_opt->guisection = _("Output maps");

    opt_swapsize = G_define_option();
    opt_swapsize->key = "memory";
    opt_swapsize->type = TYPE_INTEGER;
    opt_swapsize->answer = "300";
    opt_swapsize->description = _("Max memory used in memory swap mode (MB)");
    opt_swapsize->guisection = _("Memory settings");

    flag_outs = G_define_flag();
    flag_outs->key = 'o';
    flag_outs->description =
	_("Calculate parameters for outlets (outlet mode) instead of (default) streams");

    flag_sub = G_define_flag();
    flag_sub->key = 's';
    flag_sub->description =
	_("Calculate parameters for subbasins (ignored in stream mode)");

    flag_near = G_define_flag();
    flag_near->key = 'n';
    flag_near->description =
	_("Calculate nearest local maximum (ignored in downstream calculation)");

    flag_segmentation = G_define_flag();
    flag_segmentation->key = 'm';
    flag_segmentation->description = _("Use memory swap (operation is slow)");
    flag_segmentation->guisection = _("Memory settings");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    if (!out_diff_opt->answer && !out_dist_opt->answer)
	G_fatal_error(_("You must select at least one output raster maps"));
    if (out_diff_opt->answer && !in_elev_opt->answer)
	G_fatal_error(_("Output elevation difference raster map requires "
                        "input elevation raster map to be specified"));
    
    if (!strcmp(in_method_opt->answer, "upstream"))
	method = UPSTREAM;
    else if (!strcmp(in_method_opt->answer, "downstream"))
	method = DOWNSTREAM;

    outs = (flag_outs->answer != 0);
    subs = (flag_sub->answer != 0);
    near = (flag_near->answer != 0);
    segmentation = (flag_segmentation->answer != 0);

    nrows = Rast_window_rows();
    ncols = Rast_window_cols();
    G_begin_distance_calculations();

    fifo_max = 4 * nrows * ncols;
    fifo_points = (POINT *) G_malloc((fifo_max + 1) * sizeof(POINT));

    if (!segmentation) {
	G_message(_("All in RAM calculation - method <%s>..."),
		  method_name[method]);
	MAP map_dirs, map_streams, map_distance, map_elevation,
	    map_tmp_elevation;
	CELL **streams, **dirs;
	FCELL **distance;
	FCELL **elevation = NULL;
	FCELL **tmp_elevation = NULL;

	ram_create_map(&map_streams, CELL_TYPE);
	ram_read_map(&map_streams, in_stm_opt->answer, 1, CELL_TYPE);
	ram_create_map(&map_dirs, CELL_TYPE);
	ram_read_map(&map_dirs, in_dir_opt->answer, 1, CELL_TYPE);
	ram_create_map(&map_distance, FCELL_TYPE);

	streams = (CELL **) map_streams.map;
	dirs = (CELL **) map_dirs.map;
	distance = (FCELL **) map_distance.map;
	number_of_streams = (int)map_streams.max + 1;

	outlets_num =
	    ram_find_outlets(streams, number_of_streams, dirs, subs, outs);
	ram_init_distance(streams, distance, outlets_num, outs);
	ram_release_map(&map_streams);

	if (in_elev_opt->answer) {
	    ram_create_map(&map_elevation, FCELL_TYPE);
	    ram_read_map(&map_elevation, in_elev_opt->answer, 0, -1);
	    elevation = (FCELL **) map_elevation.map;
	}			/* map elevation will be replaced by elevation difference map */


	if (method == DOWNSTREAM) {
	    G_message(_("Calculate downstream parameters..."));
	    for (j = 0; j < outlets_num; ++j) {
		G_percent(j, outlets_num, 1);
		ram_calculate_downstream(dirs, distance, elevation,
					 outlets[j], outs);
	    }
	    G_percent(j, outlets_num, 1);

	}
	else if (method == UPSTREAM) {

	    if (out_diff_opt->answer) {
		ram_create_map(&map_tmp_elevation, FCELL_TYPE);
		tmp_elevation = (FCELL **) map_tmp_elevation.map;
	    }

	    for (j = 0; j < outlets_num; ++j)
		ram_fill_basins(outlets[j], distance, dirs);
	    ram_calculate_upstream(distance, dirs, elevation, tmp_elevation,
				   near);

	}
	else {
	    G_fatal_error(_("Unrecognised method of processing"));
	}			/* end methods */

	if (out_diff_opt->answer) {
	    ram_prep_null_elevation(distance, elevation);
	    ram_write_map(&map_elevation, out_diff_opt->answer, FCELL_TYPE, 1,
			  -1);
	}

	if (out_dist_opt->answer)
	    ram_write_map(&map_distance, out_dist_opt->answer, FCELL_TYPE, 1,
			  -1);

	ram_release_map(&map_dirs);
	ram_release_map(&map_distance);

	if (in_elev_opt->answer)
	    ram_release_map(&map_elevation);
	if (in_elev_opt->answer && method == UPSTREAM)
	    ram_release_map(&map_tmp_elevation);
    }

    if (segmentation) {
	G_message(_("Calculating segments in direction <%s> (may take some time)..."),
		  method_name[method]);
	SEG map_dirs, map_streams, map_distance, map_elevation,
	    map_tmp_elevation;
	SEGMENT *streams, *dirs, *distance;
	SEGMENT *elevation = NULL;
	SEGMENT *tmp_elevation = NULL;

	number_of_segs = (int)atof(opt_swapsize->answer);
	if (method == DOWNSTREAM)
	    number_of_segs = number_of_segs < 32 ? (int)(32 / 0.18) : number_of_segs / 0.18;
	else			/* two elevation maps */
	    number_of_segs = number_of_segs < 32 ? (int)(32 / 0.24) : number_of_segs / 0.24;

	seg_create_map(&map_streams, SROWS, SCOLS, number_of_segs, CELL_TYPE);
	seg_read_map(&map_streams, in_stm_opt->answer, 1, CELL_TYPE);
	seg_create_map(&map_dirs, SROWS, SCOLS, number_of_segs, CELL_TYPE);
	seg_read_map(&map_dirs, in_dir_opt->answer, 1, CELL_TYPE);
	seg_create_map(&map_distance, SROWS, SCOLS, number_of_segs,
		       FCELL_TYPE);

	streams = &map_streams.seg;
	dirs = &map_dirs.seg;
	distance = &map_distance.seg;
	number_of_streams = (int)map_streams.max + 1;

	outlets_num =
	    seg_find_outlets(streams, number_of_streams, dirs, subs, outs);
	seg_init_distance(streams, distance, outlets_num, outs);
	seg_release_map(&map_streams);

	if (in_elev_opt->answer) {
	    seg_create_map(&map_elevation, SROWS, SCOLS, number_of_segs,
			   FCELL_TYPE);
	    seg_read_map(&map_elevation, in_elev_opt->answer, 0, -1);
	    elevation = &map_elevation.seg;
	}			/* map elevation will be replaced by elevation difference map */

	if (method == DOWNSTREAM) {
	    G_message(_("Calculate downstream parameters..."));
	    for (j = 0; j < outlets_num; ++j) {
		G_percent(j, outlets_num, 1);
		seg_calculate_downstream(dirs, distance, elevation,
					 outlets[j], outs);
	    }
	    G_percent(j, outlets_num, 1);

	}
	else if (method == UPSTREAM) {

	    if (out_diff_opt->answer) {
		seg_create_map(&map_tmp_elevation, SROWS, SCOLS,
			       number_of_segs, FCELL_TYPE);
		tmp_elevation = &map_tmp_elevation.seg;
	    }

	    for (j = 0; j < outlets_num; ++j)
		seg_fill_basins(outlets[j], distance, dirs);
	    seg_calculate_upstream(distance, dirs, elevation, tmp_elevation,
				   near);

	}
	else {
	    G_fatal_error(_("Unrecognised method of processing"));
	}			/* end methods */

	if (out_dist_opt->answer)
	    seg_write_map(&map_distance, out_dist_opt->answer, FCELL_TYPE, 1,
			  -1);

	if (out_diff_opt->answer) {
	    seg_prep_null_elevation(distance, elevation);
	    seg_write_map(&map_elevation, out_diff_opt->answer, FCELL_TYPE, 1,
			  -1);
	}

	seg_release_map(&map_dirs);
	seg_release_map(&map_distance);

	if (in_elev_opt->answer)
	    seg_release_map(&map_elevation);
	if (in_elev_opt->answer && method == UPSTREAM)
	    seg_release_map(&map_tmp_elevation);
    }

    G_free(fifo_points);
    exit(EXIT_SUCCESS);
}
