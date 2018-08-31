#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <grass/raster3d.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/vector.h>
#include <grass/glocale.h>

#include "r3flow_structs.h"
#include "flowline.h"
#include "interpolate.h"

static void test_interpolation(RASTER3D_Region * region,
			       RASTER3D_Map ** input_maps, double north,
			       double east, double top)
{
    double interpolated[3];

    if (interpolate_velocity(region, input_maps, north, east, top,
			     &interpolated[0], &interpolated[1],
			     &interpolated[2]) < 0) {
	fprintf(stdout, "return=-1\n");
    }
    else
	fprintf(stdout, "return=0\nvalues=%.10f,%.10f,%.10f\n",
		interpolated[0], interpolated[1], interpolated[2]);

}

int main(int argc, char *argv[])
{
    int i;
    struct GModule *module;
    struct Option *test_opt, *coordinates_opt, *input_opt;
    RASTER3D_Region region;
    RASTER3D_Map *input_3drasters[3];
    double coordinates[3];

    G_gisinit(argv[0]);
    module = G_define_module();
    G_add_keyword(_("raster3d"));
    G_add_keyword(_("unit test"));
    module->description = _("Testing flow lines.");

    test_opt = G_define_option();
    test_opt->key = "test";
    test_opt->required = YES;
    test_opt->type = TYPE_STRING;
    test_opt->options = "interpolation,gradient";
    test_opt->description = "Select what is tested";

    coordinates_opt = G_define_option();
    coordinates_opt->key = "coordinates";
    coordinates_opt->required = NO;
    coordinates_opt->type = TYPE_DOUBLE;
    coordinates_opt->multiple = YES;
    coordinates_opt->description = "x,y,z coordinates";

    input_opt = G_define_standard_option(G_OPT_R3_INPUTS);
    input_opt->required = NO;

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    Rast3d_init_defaults();
    Rast3d_get_window(&region);

    if (strcmp(test_opt->answer, "interpolation") == 0) {


	if (input_opt->answers) {
	    for (i = 0; i < 3; i++) {
		input_3drasters[i] =
		    Rast3d_open_cell_old(input_opt->answers[i],
					 G_find_raster3d(input_opt->
							 answers[i], ""),
					 &region, RASTER3D_TILE_SAME_AS_FILE,
					 RASTER3D_USE_CACHE_DEFAULT);
		if (input_3drasters[i] == NULL)
		    Rast3d_fatal_error(_("Unable to open 3D raster map <%s>"),
				       input_opt->answers[i]);
	    }
	}
	else
	    G_fatal_error("No input map for interpolation test");
	if (coordinates_opt->answers) {
	    for (i = 0; i < 3; i++) {
		if (coordinates_opt->answers[i]) {
		    coordinates[i] = atof(coordinates_opt->answers[i]);
		}
		else
		    G_fatal_error("Provide 3 coordinates");
	    }
	}
	else
	    G_fatal_error("No coordinates for interpolation test");
	test_interpolation(&region, input_3drasters, coordinates[1],
			   coordinates[0], coordinates[2]);
    }

    return EXIT_SUCCESS;
}
