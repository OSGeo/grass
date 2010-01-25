#include <stdlib.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include "method.h"

int
o_mode(const char *basemap, const char *covermap, const char *outputmap, int usecats,
       struct Categories *cats)
{
    struct Popen stats_child, reclass_child;
    FILE *stats, *reclass;
    int first;
    long basecat, covercat, catb, catc;
    double value, max;

    stats = run_stats(&stats_child, basemap, covermap, "-an");
    reclass = run_reclass(&reclass_child, basemap, outputmap);

    first = 1;

    while (read_stats(stats, &basecat, &covercat, &value)) {
	if (first) {
	    first = 0;
	    catb = basecat;
	    catc = covercat;
	    max = value;
	}

	if (basecat != catb) {
	  write_reclass(reclass, catb, catc, Rast_get_c_cat((CELL *) &catc, cats),
			  usecats);
	    catb = basecat;
	    catc = covercat;
	    max = value;
	}

	if (value > max) {
	    catc = covercat;
	    max = value;
	}
    }

    if (first) {
	catb = catc = 0;
    }

    write_reclass(reclass, catb, catc, Rast_get_c_cat((CELL *) &catc, cats), usecats);

    G_popen_close(&stats_child);
    G_popen_close(&reclass_child);

    return 0;
}

