#include <stdlib.h>
#include <stdio.h>
#include <grass/gis.h>
#include <grass/glocale.h>


void usage(void)
{
    G_fatal_error(_("USAGE for basin delineation:\n\n%s -4 elevation_input=elevation_map "
		    "threshold=swale_threshold [flow_input=overland_flow_map] "
		    "[drainage_output=drain_direction_map] [delevation_input=depression_map] "
		    "[accumulation_output=accumulation_map] [basin_output=watershed_basin_map] "
		    "[stream_output=stream_segment_map]\n\n"
		    "USAGE for slope length determination:\n\n%s [-4] "
		    "elevation_input=elevation_map threshold=swale_threshold "
		    "[drainage_output=drain_direction_map] [delevation_input=depression_map] "
		    "[accumulation_output=accumulation_map] [slope_length_output=max_slope_length] "
		    "[block_input=overland_blocking_map] [slope_steep_output=slope_steepness_map] "
		    "length_slope=length_slope_map [land_input=rill_erosion_map] "
		    "[slope_deposition=slope_deposition value or map]\n\n"
		    "USAGE for ARMSED FILE creation:\n\n%s [-4] elevation_input=elevation_map "
		    "threshold=swale_threshold [flow_input=overland_flow_map] "
		    "[drainage_output=drain_direction_map] [delevation_input=depression_map] "
		    "[accumulation_output=accumulation_map] [basin_output=watershed_basin_map] "
		    "[stream_output=stream_segment_map] [half_basin_output=half_basin_map] "
		    "ar=ARMSED_file_name\n\n"), G_program_name(), G_program_name(), G_program_name());
}
