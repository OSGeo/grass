#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include "method.h"

#define MEM  1024

/* function prototypes */
static int kurt(double *, int, double *);


int o_kurt(const char *basemap, const char *covermap, const char *outputmap,
	   int usecats, struct Categories *cats)
{
    struct Popen stats_child, reclass_child;
    FILE *stats, *reclass;
    int first, mem, i, count;
    long basecat, covercat, catb, catc;
    double value, var, x;
    double *tab;

    mem = MEM * sizeof(double);
    tab = (double *)G_malloc(mem);

    stats = run_stats(&stats_child, basemap, covermap, "-cn");
    reclass = run_reclass(&reclass_child, basemap, outputmap);

    first = 1;
    while (read_stats(stats, &basecat, &covercat, &value)) {
	if (first) {
	    first = 0;
	    catb = basecat;
	    catc = covercat;
	    i = 0;
	    count = 0;
	}

	if (basecat != catb) {
	    kurt(tab, count, &var);
	    fprintf(reclass, "%ld = %ld %f\n", catb, catb, var);
	    catb = basecat;
	    catc = covercat;
	    count = 0;
	}

	if (usecats)
	    sscanf(Rast_get_c_cat((CELL *) &covercat, cats), "%lf", &x);
	else
	    x = covercat;

	for (i = 0; i < value; i++) {
	    if (count * sizeof(double) >= mem) {
		mem += MEM * sizeof(double);
		tab = (double *)G_realloc(tab, mem);
		/* fprintf(stderr,"MALLOC: %d KB needed\n",(int)(mem/1024)); */
	    }
	    tab[count++] = x;
	}

    }
    if (first) {
	catb = catc = 0;
    }

    kurt(tab, count, &var);
    fprintf(reclass, "%ld = %ld %f\n", catb, catb, var);

    G_popen_close(&stats_child);
    G_popen_close(&reclass_child);

    return 0;
}


/***********************************************************************
*
*  Given an array of data[1...n], this routine returns its kurtosis
*
************************************************************************/

static int kurt(double *data, int n, double *kurto)
{
    double ave, ep, var, s;
    int i;

    if (n < 1) {
	G_warning(_("o_kurto: No data in array"));
	return (1);
    }

    *kurto = 0.0;
    var = 0.0;
    ep = 0.0;
    s = 0.0;


    for (i = 0; i < n; i++)	/* First pass to get the mean     */
	s += data[i];
    ave = s / n;

    for (i = 0; i < n; i++) {
	s = data[i] - ave;
	var += s * s;
	ep += s;
    }

    var = (var - ep * ep / n) / (n - 1);

    for (i = 0; i < n; i++) {
	s = (data[i] - ave) / sqrt(var);
	*kurto += s * s * s * s;
    }
    *kurto = (*kurto / n) - 3;

    return (0);
}
