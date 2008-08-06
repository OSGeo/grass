#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "watershed.h"


int basin_maps(INPUT * input, OUTPUT * output)
{
    char *mapset, map_layer[48];
    int i;

    G_message(_("\n\nPlease indicate which map layers you wish to use in the lumped"));
    G_message(_("parameter hydrologic/soil erosion model.  Continue inputing cell map"));
    G_message(_("layers, one at a time, until all desired map layers are in."));
    G_message(_("You can have %s include a list of categories in each."),
	      G_program_name());
    G_message(_("\nHit <return> at the map prompt to continue with %s"),
	      G_program_name());

    mapset = G_ask_old("", map_layer, "cell", "cell");
    while (mapset != NULL) {
	output->num_maps++;
	if (output->num_maps == 1)
	    output->maps = (MAP *) G_malloc(sizeof(MAP));
	else
	    output->maps =
		(MAP *) G_realloc(output->maps,
				  output->num_maps * sizeof(MAP));
	output->maps[output->num_maps - 1].mapset = mapset;
	output->maps[output->num_maps - 1].name = G_store(map_layer);
	output->maps[output->num_maps - 1].do_cats =
	    G_yes("Complete list of categories?", 1);
	mapset = G_ask_old("", map_layer, "cell", "cell");
    }

    G_message(_("\nThe output from %s will be divided into watershed"),
	      G_program_name());
    G_message(_("basins.  There are two possible methods of tabulating the information:"));
    G_message(_("1) by only including data pertaining to the basin itself, or 2) using"));
    G_message(_("data from the basin, and all basins upstream of it."));

    do {
	G_message(_("\nWould you like the data organized:"));
	G_message(_("1) Basin only\n2) Upstream only\n3) Both\nOR 0) to cancel program"));
	fprintf(stderr, _("\nYour choice: "));
	G_gets(map_layer);
	sscanf(map_layer, "%d", &i);
    } while (i > 3 || i < 0);

    switch (i) {
    case 0:
	exit(EXIT_SUCCESS);
	break;
    case 1:
	output->do_basin = 1;
	output->do_accum = 0;
	break;
    case 2:
	output->do_basin = 0;
	output->do_accum = 1;
	break;
    case 3:
	output->do_basin = 1;
	output->do_accum = 1;
	break;
    }

    if (input->fast) {
	G_message(_("\nOK, %s should start running now using the "
		    "following form:\n%s"), RAM_NAME, input->com_line_ram);
    }
    else {
	G_message(_("\nOK, %s should start running now using the "
		    "following form:\n%s"), SEG_NAME, input->com_line_seg);
    }

    return 0;
}
