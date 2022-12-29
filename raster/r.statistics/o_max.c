#include <stdlib.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include "method.h"

int o_max(const char *basemap, const char *covermap, const char *outputmap,
	  int usecats, struct Categories *cats)
{
    struct Popen stats_child, reclass_child;
    FILE *stats, *reclass;
    int first;
    long basecat, covercat, catb, catc;

    stats = run_stats(&stats_child, basemap, covermap, "-n");
    reclass = run_reclass(&reclass_child, basemap, outputmap);

    first = 1;

    while (fscanf(stats, "%ld %ld", &basecat, &covercat) == 2) {
	if (first) {
	    first = 0;
	    catb = basecat;
	    catc = covercat;
	}

	if (basecat != catb) {
	  write_reclass(reclass, catb, catc, Rast_get_c_cat((CELL *) &catc, cats),
			  usecats);
	    catb = basecat;
	    catc = covercat;
	}

	if (covercat > catc)
	    catc = covercat;
    }

    if (first) {
	catb = catc = 0;
    }

    write_reclass(reclass, catb, catc, Rast_get_c_cat((CELL *) &catc, cats), usecats);

    G_popen_close(&stats_child);
    G_popen_close(&reclass_child);

    return 0;
}

