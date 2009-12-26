#include <stdlib.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include "method.h"

/* function prototypes */
static int out(FILE *, long, double, double);

int
o_average(const char *basemap, const char *covermap, const char *outputmap, int usecats,
	  struct Categories *cats)
{
    char *me = "o_average";
    long catb, basecat, covercat;
    double x, area, sum1, sum2;
    int stat;
    char *tempfile1, *tempfile2;
    FILE *fd1, *fd2;

    tempfile1 = G_tempfile();
    tempfile2 = G_tempfile();

    run_stats(basemap, covermap, "-a", tempfile1);

    fd1 = fopen(tempfile1, "r");
    fd2 = fopen(tempfile2, "w");
    if (fd1 == NULL || fd2 == NULL) {
	unlink(tempfile1);
	unlink(tempfile2);
	G_fatal_error(_("%s: unable to open temporary file"), me);
    }
    out(fd2, 0L, 0.0, 1.0);	/* force at least one reclass rule */

    catb = 0;
    sum1 = 0.0;
    sum2 = 0.0;
    while (fscanf(fd1, "%ld %ld %lf", &basecat, &covercat, &area) == 3) {
	if (catb != basecat) {
	    out(fd2, catb, sum1, sum2);
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
    out(fd2, basecat, sum1, sum2);
    fclose(fd1);
    fclose(fd2);
    stat = run_reclass(basemap, outputmap, tempfile2);
    unlink(tempfile1);
    unlink(tempfile2);

    return (stat);
}


static int out(FILE * fd, long cat, double sum1, double sum2)
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
    fprintf(fd, "%ld = %ld %s\n", cat, cat, buf);

    return 0;
}
