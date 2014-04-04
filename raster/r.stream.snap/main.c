
/****************************************************************************
 *
 * MODULE:       r.stream.snap
 * AUTHOR(S):    Jarek Jasiewicz jarekj amu.edu.pl
 *               
 * PURPOSE:      Snap points features to nearest pour points. Useful both to
 *               snap outlets as well as sources. It uses two parameters: 
 *               maximum snap distance and minimum accumulation value to snap
 *               
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
    struct Option *in_points_opt,
	*out_points_opt,
	*in_stream_opt,
	*in_accum_opt,
	*opt_accum_treshold, *opt_distance_treshold, *opt_swapsize;

    int i;
    SEG map_streams, map_accum;
    SEGMENT *streams = NULL, *accum = NULL;
    int number_of_segs;
    int number_of_points;
    int radius;
    double accum_treshold;

    G_gisinit(argv[0]);

    module = G_define_module();
    module->label = _("Snap point to modelled stream network.");
    module->description = _("Input can be stream network, point vector map with outlets or outlet coordinates.");
    G_add_keyword(_("raster"));
    G_add_keyword(_("hydrology"));
    G_add_keyword(_("stream network"));
    G_add_keyword(_("basins creation"));

    in_points_opt = G_define_standard_option(G_OPT_V_INPUT);
    in_points_opt->description = _("Name of input vector points map");

    out_points_opt = G_define_standard_option(G_OPT_V_OUTPUT);
    out_points_opt->description = _("Name of output vector points map");

    in_stream_opt = G_define_standard_option(G_OPT_R_INPUT);
    in_stream_opt->key = "stream_rast";
    in_stream_opt->required = NO;
    in_stream_opt->description = _("Name for input raster map with stream network");
    in_stream_opt->guisection = _("Input maps");

    in_accum_opt = G_define_standard_option(G_OPT_R_INPUT);
    in_accum_opt->key = "accumulation";
    in_accum_opt->required = NO;
    in_accum_opt->description = _("Name of input accumulation raster map");
    in_accum_opt->guisection = _("Input maps");
 
    opt_accum_treshold = G_define_option();
    opt_accum_treshold->key = "threshold";
    opt_accum_treshold->type = TYPE_DOUBLE;
    opt_accum_treshold->answer = "-1";
    opt_accum_treshold->description =
	_("Minimum accumulation threshold to snap");

    opt_distance_treshold = G_define_option();
    opt_distance_treshold->key = "radius";
    opt_distance_treshold->answer = "1";
    opt_distance_treshold->type = TYPE_INTEGER;
    opt_distance_treshold->description =
	_("Maximum distance to snap (in cells)");

    opt_swapsize = G_define_option();
    opt_swapsize->key = "memory";
    opt_swapsize->type = TYPE_INTEGER;
    opt_swapsize->answer = "300";
    opt_swapsize->required = NO;
    opt_swapsize->description = _("Max memory used (MB)");
    opt_swapsize->guisection = _("Memory settings");
    
    if (G_parser(argc, argv))	/* parser */
	exit(EXIT_FAILURE);

    number_of_segs = (int)atof(opt_swapsize->answer);
    number_of_segs = number_of_segs < 32 ? (int)(32 / 0.12) : number_of_segs / 0.12;

    radius = atoi(opt_distance_treshold->answer);
    accum_treshold = atof(opt_accum_treshold->answer);

    nrows = Rast_window_rows();
    ncols = Rast_window_cols();

    if (G_legal_filename(out_points_opt->answer) < 0)
	G_fatal_error(_("<%s> is an illegal basin name"),
		      out_points_opt->answer);

    if (!in_stream_opt->answer && !in_accum_opt->answer)
	G_fatal_error(_("At least one of accumulation or streams raster maps is required"));

    if (!in_accum_opt->answer)
	accum_treshold = -1;


    /* SEGMENT VERSION ONLY */

    if (in_stream_opt->answer) {
	seg_create_map(&map_streams, SROWS, SCOLS, number_of_segs, CELL_TYPE);
	seg_read_map(&map_streams, in_stream_opt->answer, 1, CELL_TYPE);
	streams = &map_streams.seg;
    }

    if (in_accum_opt->answer) {
	seg_create_map(&map_accum, SROWS, SCOLS, number_of_segs, DCELL_TYPE);
	seg_read_map(&map_accum, in_accum_opt->answer, 0, -1);
	accum = &map_accum.seg;
    }

    create_distance_mask(radius);
    number_of_points = read_points(in_points_opt->answer, streams, accum);

    for (i = 0; i < number_of_points; ++i)
	snap_point(&points[i], radius, streams, accum, accum_treshold);

    write_points(out_points_opt->answer, number_of_points);


    /*
    for (i = 0; i < number_of_points; ++i)
	G_message("AFTER %d %d %d %d",
		  points[i].r, points[i].c, points[i].di, points[i].dj);
    */

    if (in_stream_opt->answer)
	seg_release_map(&map_streams);
    if (in_accum_opt->answer)
	seg_release_map(&map_accum);

    exit(EXIT_SUCCESS);
}
