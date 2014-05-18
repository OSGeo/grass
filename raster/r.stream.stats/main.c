
/****************************************************************************
 *
 * MODULE:       r.stream.stats
 * AUTHOR(S):    Jarek Jasiewicz jarekj amu.edu.pl
 *               
 * PURPOSE:      Calculate Horton's statistics according stream network and elevation map.
 *               Program calculates: Bifurcation ratio, length ratio, area ratio, 
 *               slope ratio and drainage density.
 *               It uses r.stream.order stream map, r.watershed direction map and DEM
 *               Stream input map shall contain streams ordered according Strahler's or
 *               Horton's orders. It also can calculate Hack's laws as an option.
 *               If input stream comes from r.stream.extract direction map 
 *               from r.stream.extract dir map must be patched with that of r.watershed.
 *
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

    struct GModule *module;
    struct Option *in_dir_opt,	/* options */
     *in_stm_opt, *in_elev_opt, *opt_swapsize, *opt_output;

    struct Flag *flag_segmentation,
	*flag_catchment_total, *flag_orders_summary;

    char *filename;
    int number_of_segs;
    int order_max;
    int segmentation, catchment_total, orders_summary;	/*flags */

    /* initialize GIS environment */
    G_gisinit(argv[0]);

    /* initialize module */
    module = G_define_module();
    module->description =
	_("Calculates Horton's statistics for Strahler and Horton ordered networks created with r.stream.order.");
    G_add_keyword(_("raster"));
    G_add_keyword(_("hydrology"));
    G_add_keyword(_("stream network"));
    G_add_keyword(_("Horton's statistics"));

    in_stm_opt = G_define_standard_option(G_OPT_R_INPUT);
    in_stm_opt->key = "stream_rast";
    in_stm_opt->description = _("Name of input raster map with stream network");

    in_dir_opt = G_define_standard_option(G_OPT_R_INPUT);
    in_dir_opt->key = "direction";
    in_dir_opt->description = _("Name of input flow direction raster map");

    in_elev_opt = G_define_standard_option(G_OPT_R_ELEV);

    opt_swapsize = G_define_option();
    opt_swapsize->key = "memory";
    opt_swapsize->type = TYPE_INTEGER;
    opt_swapsize->answer = "300";
    opt_swapsize->description = _("Max memory used in memory swap mode (MB)");
    opt_swapsize->guisection = _("Memory settings");
    
    opt_output = G_define_standard_option(G_OPT_F_OUTPUT);
    opt_output->required = NO;
    opt_output->description =
	_("Name for output file (if omitted output to stdout)");

    flag_segmentation = G_define_flag();
    flag_segmentation->key = 'm';
    flag_segmentation->description = _("Use memory swap (operation is slow)");
    flag_segmentation->guisection = _("Memory settings");

    flag_catchment_total = G_define_flag();
    flag_catchment_total->key = 'c';
    flag_catchment_total->description =
	_("Print only catchment's statistics");
    flag_catchment_total->guisection = _("Print");

    flag_orders_summary = G_define_flag();
    flag_orders_summary->key = 'o';
    flag_orders_summary->description = _("Print only orders' statistics");
    flag_orders_summary->guisection = _("Print");

    if (G_parser(argc, argv))	/* parser */
	exit(EXIT_FAILURE);

    segmentation = (flag_segmentation->answer != 0);
    catchment_total = (flag_catchment_total->answer != 0);
    orders_summary = (flag_orders_summary->answer != 0);

    filename = opt_output->answer;
    if (filename != NULL)
	if (NULL == freopen(filename, "w", stdout))
	    G_fatal_error(_("Unable to open file '%s' for writing"),
			  filename);

    nrows = Rast_window_rows();
    ncols = Rast_window_cols();

    if (!segmentation) {
	MAP map_dirs, map_streams, map_elevation;
	CELL **streams, **dirs;
	FCELL **elevation;

	G_message(_("All in RAM calculation..."));

	ram_create_map(&map_streams, CELL_TYPE);
	ram_read_map(&map_streams, in_stm_opt->answer, 1, CELL_TYPE);
	ram_create_map(&map_dirs, CELL_TYPE);
	ram_read_map(&map_dirs, in_dir_opt->answer, 1, CELL_TYPE);
	ram_create_map(&map_elevation, FCELL_TYPE);
	ram_read_map(&map_elevation, in_elev_opt->answer, 0, -1);

	streams = (CELL **) map_streams.map;
	dirs = (CELL **) map_dirs.map;
	elevation = (FCELL **) map_elevation.map;
	order_max = (int)map_streams.max;

	ram_init_streams(streams, dirs, elevation);
	ram_calculate_streams(streams, dirs, elevation);
	ram_calculate_basins(dirs);

	stats(order_max);
	if (!catchment_total && !orders_summary)
	    print_stats(order_max);
	if (catchment_total)
	    print_stats_total();
	if (orders_summary)
	    print_stats_orders(order_max);

	G_free(stat_streams);
	G_free(ord_stats);

	ram_release_map(&map_streams);
	ram_release_map(&map_dirs);
	ram_release_map(&map_elevation);
    }

    if (segmentation) {
	SEG map_dirs, map_streams, map_elevation;
	SEGMENT *streams, *dirs, *elevation;

        G_message(_("Memory swap calculation (may take some time)..."));

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
	order_max = (int)map_streams.max;

	seg_init_streams(streams, dirs, elevation);
	seg_calculate_streams(streams, dirs, elevation);
	seg_calculate_basins(dirs);

	stats(order_max);

	if (!catchment_total && !orders_summary)
	    print_stats(order_max);
	if (catchment_total)
	    print_stats_total();
	if (orders_summary)
	    print_stats_orders(order_max);

	G_free(stat_streams);
	G_free(ord_stats);

	seg_release_map(&map_streams);
	seg_release_map(&map_dirs);
	seg_release_map(&map_elevation);

    }
    exit(EXIT_SUCCESS);
}
