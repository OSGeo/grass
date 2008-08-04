#include <stdlib.h>
#include <grass/gis.h>
#include "watershed.h"

int free_input(INPUT * input)
{
    if (input->com_line_ram)
	G_free(input->com_line_ram);
    if (input->com_line_seg)
	G_free(input->com_line_seg);
    G_free(input->haf_name);
    G_free(input->ar_file_name);
    G_free(input->accum_name);

    return 0;
}

int free_output(OUTPUT * output)
{
    int c;

    G_free(output->basin_facts);
    G_free(output->file_name);
    for (c = output->num_maps - 1; c >= 0; c--) {
	G_free(output->maps[c].name);
    }
    G_free(output->maps);

    return 0;
}
