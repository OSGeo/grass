#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/imagery.h>
#include <grass/cluster.h>


#define FMT1 "%g/%d=%.1f"
#define FMT2 "%g/%d=?"


int print_centroids(FILE * fd, struct Cluster *C)
{
    int band, cat;
    char buf[40];

    fprintf(fd, _("class centroids (sum/count=mean)%s"), HOST_NEWLINE);
    for (band = 0; band < C->nbands; band++) {
	fprintf(fd, _("band %d"), band + 1);
	for (cat = 0; cat < C->nclasses; cat++) {
	    if (C->count[cat])
		sprintf(buf, FMT1, C->sum[band][cat], C->count[cat],
			(double)C->sum[band][cat] / (double)C->count[cat]);
	    else
		sprintf(buf, FMT2, C->sum[band][cat], C->count[cat]);
	    fprintf(fd, "%s %-18s", cat % 4 ? "" : HOST_NEWLINE, buf);
	}
	fprintf(fd, "%s", HOST_NEWLINE);
    }

    return 0;
}
