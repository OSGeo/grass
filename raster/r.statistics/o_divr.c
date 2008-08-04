#include <stdlib.h>
#include <unistd.h>
#include <grass/gis.h>
#include "method.h"

int
o_divr(char *basemap, char *covermap, char *outputmap, int usecats,
       struct Categories *cats)
{
    char command[1024];
    FILE *stats_fd, *reclass_fd;
    int first;
    long basecat, covercat, catb, catc;
    double area;


    sprintf(command, "r.stats -an input=\"%s,%s\" fs=space", basemap,
	    covermap);
    stats_fd = popen(command, "r");


    sprintf(command, "r.reclass i=\"%s\" o=\"%s\"", basemap, outputmap);
    reclass_fd = popen(command, "w");

    first = 1;
    while (read_stats(stats_fd, &basecat, &covercat, &area)) {
	if (first) {
	    first = 0;
	    catb = basecat;
	    catc = 0;
	}
	if (basecat != catb) {
	    write_reclass(reclass_fd, catb, catc, G_get_cat(catc, cats),
			  usecats);
	    catb = basecat;
	    catc = 0;
	}
	catc++;
    }
    if (!first)
	write_reclass(reclass_fd, catb, catc, G_get_cat(catc, cats), usecats);

    pclose(stats_fd);
    pclose(reclass_fd);

    exit(EXIT_SUCCESS);
}
