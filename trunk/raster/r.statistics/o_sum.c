#include <stdlib.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include "method.h"

/* function prototypes */
static void sum_out(FILE *, long, double);

int o_sum(const char *basemap, const char *covermap, const char *outputmap,
	  int usecats, struct Categories *cats)
{
    long catb, basecat, covercat;
    double x, area, sum1;
    int stat;
    struct Popen stats_child, reclass_child;
    FILE *stats, *reclass;

    stats = run_stats(&stats_child, basemap, covermap, "-cn");
    reclass = run_reclass(&reclass_child, basemap, outputmap);

    sum_out(reclass, 0L, 0.0);	/* force at least one reclass rule */

    catb = 0;
    sum1 = 0.0;

    while (fscanf(stats, "%ld %ld %lf", &basecat, &covercat, &area) == 3) {
	if (catb != basecat) {
	    sum_out(reclass, catb, sum1);
	    sum1 = 0.0;
	    catb = basecat;
	}
	if (usecats)
	    sscanf(Rast_get_c_cat((CELL *) &covercat, cats), "%lf", &x);
	else
	    x = covercat;
	sum1 += x * area;
	/*        fprintf(stderr,"sum: %d\n",(int)sum1); */

    }
    sum_out(reclass, basecat, sum1);

    G_popen_close(&stats_child);
    G_popen_close(&reclass_child);

    return stat;
}

static void sum_out(FILE *fp, long cat, double sum1)
{
    char buf[64];

    if (cat == 0)
	*buf = 0;
    else {
	sprintf(buf, "%.10lf", sum1);
	G_trim_decimal(buf);
    }

    fprintf(fp, "%ld = %ld %s\n", cat, cat, buf);
}

