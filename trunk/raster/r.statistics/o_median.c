#include <stdlib.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include "method.h"

/* function prototypes */
static long median(struct stats *);


int o_median(const char *basemap, const char *covermap, const char *outputmap,
	     int usecats, struct Categories *cats)
{
    struct Popen stats_child, reclass_child;
    FILE *stats_fp, *reclass_fp;
    int first;
    long basecat, covercat, catb, catc;
    double area;
    struct stats stats;

    stats_fp = run_stats(&stats_child, basemap, covermap, "-an");
    reclass_fp = run_reclass(&reclass_child, basemap, outputmap);

    first = 1;
    while (read_stats(stats_fp, &basecat, &covercat, &area)) {
	if (first) {
	    stats.n = 0;
	    stats.nalloc = 16;
	    stats.cat = (long *)
		G_calloc(stats.nalloc, sizeof(long));
	    stats.area = (double *)
		G_calloc(stats.nalloc, sizeof(double));
	    first = 0;
	    catb = basecat;
	}
	if (basecat != catb) {
	    catc = median(&stats);
	    write_reclass(reclass_fp, catb, catc, Rast_get_c_cat((CELL *) &catc, cats),
			  usecats);
	    catb = basecat;
	    stats.n = 0;
	}
	stats.n++;
	if (stats.n > stats.nalloc) {
	    stats.nalloc *= 2;
	    stats.cat = (long *)
		G_realloc(stats.cat, stats.nalloc * sizeof(long));
	    stats.area = (double *)
		G_realloc(stats.area, stats.nalloc * sizeof(double));
	}
	stats.cat[stats.n - 1] = covercat;
	stats.area[stats.n - 1] = area;
    }
    if (!first) {
	catc = median(&stats);
	write_reclass(reclass_fp, catb, catc, Rast_get_c_cat((CELL *) &catc, cats), usecats);
    }

    G_popen_close(&stats_child);
    G_popen_close(&reclass_child);

    return 0;
}


static long median(struct stats *stats)
{
    double total, sum;
    int i;

    total = 0;
    for (i = 0; i < stats->n; i++)
	total += stats->area[i];

    total /= 2.0;

    sum = 0;
    for (i = 0; i < stats->n; i++) {
	sum += stats->area[i];
	if (sum > total)
	    break;
    }
    if (i == stats->n)
	i--;
    if (i < 0)
	return ((long)0);

    return (stats->cat[i]);
}
