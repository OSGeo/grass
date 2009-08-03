#include <grass/gis.h>
#include <grass/raster.h>
#include "options.h"

void fancy(struct Cell_head *window, struct Categories *cats, FILE * fp)
{
    char xname[GNAME_MAX], xmapset[GMAPSET_MAX];

    G_unqualified_name(map_name, G_mapset(), xname, xmapset);

    fprintf(fp, ".C %s\n", "green");
    fprintf(fp, ".S %f\n", size + 1.0);
    fprintf(fp, "LOCATION: %s\n", G_location());
    fprintf(fp, ".C %s\n", color);
    fprintf(fp, ".S %f\n", size);
    fprintf(fp, "%s in %s\n", xname, xmapset);
    fprintf(fp, "%s\n", cats->title);
    fprintf(fp, "North: %10.2f  South: %10.2f\n",
	    window->north, window->south);
    fprintf(fp, "West:  %10.2f  East:  %10.2f\n", window->west, window->east);
    fprintf(fp, "Resolution: n-s: %7.2f  e-w: %7.2f\n",
	    window->ns_res, window->ns_res);
}
