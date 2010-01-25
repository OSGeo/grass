#include <stdlib.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include "method.h"

/* function prototypes */
static int out(FILE *, long, double, double);

int o_average(const char *basemap, const char *covermap, const char *outputmap,
	      int usecats, struct Categories *cats)
{
    struct Popen stats_child, reclass_child;
    FILE *stats, *reclass;
    long catb, basecat, covercat;
    double x, area, sum1, sum2;

    stats = run_stats(&stats_child, basemap, covermap, "-an");
    reclass = run_reclass(&reclass_child, basemap, outputmap);

    out(reclass, 0L, 0.0, 1.0);	/* force at least one reclass rule */

    catb = 0;
    sum1 = 0.0;
    sum2 = 0.0;
    while (fscanf(stats, "%ld %ld %lf", &basecat, &covercat, &area) == 3) {
	if (catb != basecat) {
	    out(reclass, catb, sum1, sum2);
	    sum1 = 0.0;
	    sum2 = 0.0;
	    catb = basecat;
	}
	if (usecats)
	    sscanf(Rast_get_c_cat((CELL *) &covercat, cats), "%lf", &x);
	else
	    x = covercat;
	sum1 += x * area;
	sum2 += area;
    }

    out(reclass, basecat, sum1, sum2);

    G_popen_close(&stats_child);
    G_popen_close(&reclass_child);

    return 0;
}


static int out(FILE *fp, long cat, double sum1, double sum2)
{
    char buf[80];

    if (sum2 == 0)
	return -1;
    if (cat == 0)
	*buf = 0;
    else {
	sprintf(buf, "%.10f", sum1 / sum2);
	G_trim_decimal(buf);
    }
    fprintf(fp, "%ld = %ld %s\n", cat, cat, buf);

    return 0;
}
