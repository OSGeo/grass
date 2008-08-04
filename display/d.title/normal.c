#include <grass/gis.h>
#include "options.h"

void normal(char *mapset, struct Cell_head *window, struct Categories *cats,
	    int simple, FILE * fp)
{

    fprintf(fp, ".C %s\n", color);
    fprintf(fp, ".S %f\n", size);

    if (simple) {
	fprintf(fp, "%s\n", map_name);
    }
    else {
	fprintf(fp, "LOCATION: %s\n", G_location());
	fprintf(fp, "%s in %s\n", map_name, mapset);
	fprintf(fp, "%s\n", cats->title);
	fprintf(fp, "North: %10.2f  South: %10.2f\n",
		window->north, window->south);
	fprintf(fp, "West:  %10.2f  East:  %10.2f\n",
		window->west, window->east);
	fprintf(fp, "Resolution: n-s: %7.2f  e-w: %7.2f\n",
		window->ns_res, window->ns_res);
    }
}
