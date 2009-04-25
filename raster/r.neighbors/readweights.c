#include <stdio.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "ncb.h"
#include "local_proto.h"

void read_weights(const char *filename)
{
    FILE *fp = fopen(filename, "r");
    int i, j;

    ncb.weights = G_malloc(ncb.nsize * sizeof(DCELL *));
    for (i = 0; i < ncb.nsize; i++)
	ncb.weights[i] = G_malloc(ncb.nsize * sizeof(DCELL));

    if (!fp)
	G_fatal_error(_("Unable to open weights file %s"), filename);

    for (i = 0; i < ncb.nsize; i++)
	for (j = 0; j < ncb.nsize; j++)
	    if (fscanf(fp, "%lf", &ncb.weights[i][j]) != 1)
		G_fatal_error(_("Error reading weights file %s"), filename);

    fclose(fp);
}

void gaussian_weights(double sigma)
{
    double sigma2 = sigma * sigma;
    int i, j;

    ncb.weights = G_malloc(ncb.nsize * sizeof(DCELL *));
    for (i = 0; i < ncb.nsize; i++)
	ncb.weights[i] = G_malloc(ncb.nsize * sizeof(DCELL));

    for (i = 0; i < ncb.nsize; i++) {
	double y = i - ncb.dist;
	for (j = 0; j < ncb.nsize; j++) {
	    double x = j - ncb.dist;
	    ncb.weights[i][j] = exp(-(x*x+y*y)/(2*sigma2))/(2*M_PI*sigma2);
	}
    }
}
