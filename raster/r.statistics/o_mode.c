#include <stdlib.h>
#include <unistd.h>
#include <grass/gis.h>
#include "method.h"

int
o_mode(char *basemap, char *covermap, char *outputmap, int usecats,
       struct Categories *cats)
{
    char command[1024];
    FILE *stats, *reclass;
    int first;
    long basecat, covercat, catb, catc;
    double value, max;

    sprintf(command, "r.stats -an input=\"%s,%s\" fs=space", basemap,
	    covermap);
    stats = popen(command, "r");

    sprintf(command, "r.reclass i=\"%s\" o=\"%s\"", basemap, outputmap);
    reclass = popen(command, "w");

    first = 1;

    while (read_stats(stats, &basecat, &covercat, &value)) {
	if (first) {
	    first = 0;
	    catb = basecat;
	    catc = covercat;
	    max = value;
	}

	if (basecat != catb) {
	    write_reclass(reclass, catb, catc, G_get_cat(catc, cats),
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

    write_reclass(reclass, catb, catc, G_get_cat(catc, cats), usecats);

    pclose(stats);
    pclose(reclass);

    return (0);
}
