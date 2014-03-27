
/****************************************************************************
 *
 * MODULE: r.stream.segment
 * AUTHOR(S): Jarek Jasiewicz jarekj amu.edu.pl
 *							 
 * PURPOSE:	 Calculate geometrical attributes for segments of current order, 
 * 			 divide segments on near straight line portions and 
 * 			 segment orientation and angles between streams and its
 *           tributaries. For stream direction it use algorithm to divide
 *           particular streams of the same order into near-straight line
 *           portions.
 * 				
 *							
 *
 * COPYRIGHT:		 (C) 2002,2010-2014 by the GRASS Development Team
 *
 *			 This program is free software under the GNU General Public
 *			 License (>=v2). Read the file COPYING that comes with GRASS
 *			 for details.
 *
 *****************************************************************************/
#define MAIN
#include <grass/glocale.h>
#include "local_proto.h"

int nextr[9] = { 0, -1, -1, -1, 0, 1, 1, 1, 0 };
int nextc[9] = { 0, 1, 0, -1, -1, -1, 0, 1, 1 };


int main(int argc, char *argv[])
{

    struct GModule *module;	/* GRASS module for parsing arguments */
    struct Option *in_dir_opt,	/* options */
     *in_stm_opt,
	*in_elev_opt,
	*out_segment_opt,
	*out_sector_opt,
	*opt_length, *opt_skip, *opt_threshold, *opt_swapsize;

    struct Flag *flag_radians, *flag_segmentation;	/* segmentation library */

    int i;
    int seg_length, seg_skip;
    int radians, segmentation;	/* flags */
    double seg_treshold;
    int number_of_streams, ordered;

    /* initialize GIS environment */
    G_gisinit(argv[0]);		/* reads grass env, stores program name to G_program_name() */

    /* initialize module */
    module = G_define_module();
    module->description =
	_("Divides network into near straight-line segments and calculate its order.");
    G_add_keyword(_("raster"));
    G_add_keyword(_("hydrology"));
    G_add_keyword(_("stream network"));
    G_add_keyword(_("stream divide"));

    in_stm_opt = G_define_standard_option(G_OPT_R_INPUT);
    in_stm_opt->key = "stream_raster";
    in_stm_opt->description = _("Name of input streams mask raster map");

    in_dir_opt = G_define_standard_option(G_OPT_R_INPUT);
    in_dir_opt->key = "direction";
    in_dir_opt->description = _("Name for input raster map with flow direction");

    in_elev_opt = G_define_standard_option(G_OPT_R_ELEV);

    out_segment_opt = G_define_standard_option(G_OPT_V_OUTPUT);
    out_segment_opt->key = "segments";
    out_segment_opt->description =
	_("Name for output vector map to write segment attributes");

    out_sector_opt = G_define_standard_option(G_OPT_V_OUTPUT);
    out_sector_opt->key = "sectors";
    out_sector_opt->description =
	_("Name for output vector map to write segment attributes");

    opt_length = G_define_option();
    opt_length->key = "length";
    opt_length->label = _("Search length to calculate direction");
    opt_length->description = _("Must be > 0");
    opt_length->answer = "15";
    opt_length->type = TYPE_INTEGER;

    opt_skip = G_define_option();
    opt_skip->key = "skip";
    opt_skip->label = _("Skip segments shorter than");
    opt_skip->description = _("Must be >= 0");
    opt_skip->answer = "5";
    opt_skip->type = TYPE_INTEGER;

    opt_threshold = G_define_option();
    opt_threshold->key = "threshold";
    opt_threshold->label = _("Max angle (degrees) between stream segments");
    opt_threshold->description = _("Must be > 0");
    opt_threshold->answer = "160";
    opt_threshold->type = TYPE_DOUBLE;

    opt_swapsize = G_define_option();
    opt_swapsize->key = "memory";
    opt_swapsize->type = TYPE_INTEGER;
    opt_swapsize->answer = "300";
    opt_swapsize->description = _("Max memory used in memory swap mode (MB)");
    opt_swapsize->guisection = _("Memory setings");

    flag_radians = G_define_flag();
    flag_radians->key = 'r';
    flag_radians->description =
	_("Output angles in radians (default: degrees)");

    flag_segmentation = G_define_flag();
    flag_segmentation->key = 'm';
    flag_segmentation->description = _("Use memory swap (operation is slow)");
    flag_segmentation->guisection = _("Memory setings");

    if (G_parser(argc, argv))	/* parser */
	exit(EXIT_FAILURE);

    seg_length = atoi(opt_length->answer);
    seg_treshold = atof(opt_threshold->answer);
    seg_skip = atoi(opt_skip->answer);
    radians = (flag_radians->answer != 0);
    segmentation = (flag_segmentation->answer != 0);

    if (seg_length <= 0)
        G_fatal_error(_("Search's length must be > 0"));
    if (seg_treshold < 0 || seg_treshold > 180)
	G_fatal_error(_("Threshold must be between 0 and 180"));
    if (seg_skip < 0)
	G_fatal_error(_("Segment's length must be >= 0"));

    seg_treshold = DEG2RAD(seg_treshold);
    nrows = Rast_window_rows();
    ncols = Rast_window_cols();
    Rast_get_window(&window);
    G_begin_distance_calculations();



    if (!segmentation) {
	MAP map_dirs, map_streams, map_elevation, map_unique_streams;
	CELL **streams, **dirs, **unique_streams = NULL;
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

	number_of_streams =
	    ram_number_of_streams(streams, dirs, &ordered) + 1;
	ram_build_streamlines(streams, dirs, elevation, number_of_streams);

	if (ordered) {
	    ram_create_map(&map_unique_streams, CELL_TYPE);
	    unique_streams = (CELL **) map_unique_streams.map;
	    ram_fill_streams(unique_streams, number_of_streams);
	    ram_identify_next_stream(unique_streams, number_of_streams);
	    ram_release_map(&map_unique_streams);
	}
	else
	    ram_identify_next_stream(streams, number_of_streams);

	ram_release_map(&map_streams);
	ram_release_map(&map_dirs);
	ram_release_map(&map_elevation);
    }


    if (segmentation) {
	SEG map_dirs, map_streams, map_elevation, map_unique_streams;
	SEGMENT *streams, *dirs, *unique_streams = NULL;
	SEGMENT *elevation;
	int number_of_segs;

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

	number_of_streams =
	    seg_number_of_streams(streams, dirs, &ordered) + 1;
	seg_build_streamlines(streams, dirs, elevation, number_of_streams);

	if (ordered) {
	    seg_create_map(&map_unique_streams, SROWS, SCOLS, number_of_segs,
			   CELL_TYPE);
	    unique_streams = &map_unique_streams.seg;
	    seg_fill_streams(unique_streams, number_of_streams);
	    seg_identify_next_stream(unique_streams, number_of_streams);
	    seg_release_map(&map_unique_streams);
	}
	else
	    seg_identify_next_stream(streams, number_of_streams);

	seg_release_map(&map_streams);
	seg_release_map(&map_dirs);
	seg_release_map(&map_elevation);
    }


    for (i = 1; i < number_of_streams; ++i)
	G_message("%d %d %d", stream_attributes[i].stream,
		  stream_attributes[i].next_stream,
		  stream_attributes[i].last_cell_dir);


    /*
       for(i=1;i<number_of_streams;++i)
       printf("STREAM %d NEXT_STREAM %d POINT %d \n",
       stream_attributes[i].stream,
       stream_attributes[i].next_stream,
       stream_attributes[i].outlet);

     */
    G_message(_("Creating sectors and calculating attributes..."));

    for (i = 1; i < number_of_streams; ++i) {
	create_sectors(&stream_attributes[i], seg_length, seg_skip,
		       seg_treshold);
	calc_tangents(&stream_attributes[i], seg_length, seg_skip,
		      number_of_streams);
    }


    /*


       for(j=1;j<number_of_streams;++j)
       G_message("STREAM %d   ncells %d len %f str %f sin %f",
       stream_attributes[j].stream,
       stream_attributes[j].number_of_cells,
       stream_attributes[j].length,
       stream_attributes[j].stright,
       stream_attributes[j].length/stream_attributes[j].stright);
       G_message("%d",j);


       for(j=1;j<number_of_streams;++j)
       printf("STREAM %d   max %f min %f drop %f\n",
       stream_attributes[j].stream,
       stream_attributes[j].elevation[1],
       stream_attributes[j].elevation[stream_attributes[j].number_of_cells-1],
       stream_attributes[j].drop);

       for(j=1;j<number_of_streams;++j)
       for (i = 0; i < stream_attributes[j].number_of_sectors; ++i)
       printf("%d  cat %d |BRAEK %d | dir %.2f | len %.2f | drop %.3f  \n" ,j,
       stream_attributes[j].sector_cats[i],
       stream_attributes[j].sector_breakpoints[i],
       stream_attributes[j].sector_directions[i],
       stream_attributes[j].sector_lengths[i],
       stream_attributes[j].sector_drops[i]);

       for(j=1;j<number_of_streams;++j) {
       printf("        %d %d \n" ,j,stream_attributes[j].number_of_cells);
       for (i = 0; i <= stream_attributes[j].number_of_cells; ++i)
       printf("%d %f \n" ,i,stream_attributes[j].elevation[i]);
       }
     */

    create_segment_vector(out_segment_opt->answer, number_of_streams,
			  radians);
    create_sector_vector(out_sector_opt->answer, number_of_streams, radians);

    free_attributes(number_of_streams);
    G_message("Done");
    exit(EXIT_SUCCESS);
}
