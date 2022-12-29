#include <stdlib.h>
#include <stdio.h>
#include <grass/gis.h>
#include <grass/glocale.h>


void usage(char *me)
{
    G_fatal_error(_("USAGE for basin delineation:\n%s -4 elevation=elevation_map "
		    "threshold=swale_threshold [flow=overland_flow_map] "
		    "[drainage=drain_direction_map] [depression=depression_map] "
		    "[accumulation=accumulation_map] [basin=watershed_basin_map] "
		    "[retention=retention_map] [stream=stream_segment_map]\n\n"
		    "USAGE for slope length determination:\n%s [-4] "
		    "elevation=elevation_map threshold=swale_threshold "
		    "[drainage=drain_direction_map] [depression=depression_map] "
		    "[accumulation=accumulation_map] [retention=retention_map] "
		    "[max_slope_length=max_slope_length] "
		    "[blocking=overland_blocking_map] [slope_steepness=slope_steepness_map] "
		    "length_slope=length_slope_map [disturbed_land=rill_erosion_map] "
		    "[slope_deposition=slope_deposition value or map]"
		    "USAGE for ARMSED FILE creation:\n%s [-4] elevation=elevation_map "
		    "threshold=swale_threshold [flow=overland_flow_map] "
		    "[drainage=drain_direction_map] [depression=depression_map] "
		    "[accumulation=accumulation_map] [basin=watershed_basin_map] "
		    "[stream=stream_segment_map] [half_basin=half_basin_map] "
		    "[retention=retention_map] ar=ARMSED_file_name\n\n"), me, me, me);
}
