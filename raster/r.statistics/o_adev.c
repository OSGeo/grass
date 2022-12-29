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
static int a_dev(double *, int, double *);


int o_adev(const char *basemap, const char *covermap, const char *outputmap,
	   int usecats, struct Categories *cats)
{
    struct Popen stats_child, reclass_child;
    FILE *stats, *reclass;
    int first, mem, i, count;
    long basecat, covercat, catb, catc;
    double value, adev, x;
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
	    a_dev(tab, count, &adev);
	    fprintf(reclass, "%ld = %ld %f\n", catb, catb, adev);
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
		/* fprintf(stderr,"MALLOC: %d KB needed\n",(int)(mem/1024));  */
	    }
	    tab[count++] = x;
	}

    }
    if (first) {
	catb = catc = 0;
    }
    else {
	a_dev(tab, count, &adev);
	fprintf(reclass, "%ld = %ld %f\n", catb, catb, adev);
    }

    G_popen_close(&stats_child);
    G_popen_close(&reclass_child);

    return 0;
}


/***********************************************************************
*
*  Given an array of data[1...n], this routine returns its average
*  deviation adev.
*
************************************************************************/

static int a_dev(double *data, int n, double *adev)
{
    double ave, s;
    int i;

    if (n < 1) {
	G_warning(_("o_adev: No data in array"));
	return (1);
    }

    *adev = 0.0;
    s = 0.0;

    for (i = 0; i < n; i++)	/* First pass to get the mean     */
	s += data[i];
    ave = s / n;

    for (i = 0; i < n; i++) {
	*adev += fabs(data[i] - ave);	/* deviation from the mean             */
    }

    *adev /= n;

    return (0);
}
