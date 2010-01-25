#include <stdlib.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include "method.h"

int o_divr(const char *basemap, const char *covermap, const char *outputmap,
	   int usecats, struct Categories *cats)
{
    struct Popen stats_child, reclass_child;
    FILE *stats_fp, *reclass_fp;
    int first;
    long basecat, covercat, catb, catc;
    double area;

    stats_fp = run_stats(&stats_child, basemap, covermap, "-an");
    reclass_fp = run_reclass(&reclass_child, basemap, outputmap);

    first = 1;
    while (read_stats(stats_fp, &basecat, &covercat, &area)) {
	if (first) {
	    first = 0;
	    catb = basecat;
	    catc = 0;
	}
	if (basecat != catb) {
	  write_reclass(reclass_fp, catb, catc, Rast_get_c_cat((CELL *) &catc, cats),
			  usecats);
	    catb = basecat;
	    catc = 0;
	}
	catc++;
    }
    if (!first)
      write_reclass(reclass_fp, catb, catc, Rast_get_c_cat((CELL *) &catc, cats), usecats);

    G_popen_close(&stats_child);
    G_popen_close(&reclass_child);

    return 0;
}

