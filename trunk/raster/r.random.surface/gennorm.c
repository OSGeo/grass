/* gennorm.c                                                    */
#include <math.h>
#include <grass/gis.h>
#include "ransurf.h"

void GenNorm(void)
{
    double t, b, c, sqr;
    int i;

    G_debug(2, "GenNorm()");

    Norm = (double *)G_malloc(SIZE_OF_DISTRIBUTION * sizeof(double));
    sqr = 1 / sqrt(2 * PI);
    c = 0.0;
    for (i = 0; i < SIZE_OF_DISTRIBUTION; i++) {
	t = ((double)(i - SIZE_OF_DISTRIBUTION / 2)) * DELTA_T;
	b = exp(-t * t / 2.0) * sqr * DELTA_T;
	c = c + b;
	G_debug(3, "(c):%.12lf", c);
	Norm[i] = c;
    }

    return;
}
