/* random.c (simlib), 20.nov.2002, JH */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/bitmap.h>
#include <grass/linkm.h>

#include <grass/waterglobs.h>

double simwe_rand(void)
{
    return G_drand48();
}				/* ulec */


double gasdev(void)
{
    /* Initialized data */

    static int iset = 0;
    static double gset = .1;

    /* System generated locals */
    double ret_val;

    /* Local variables */
    double r = 0., vv1, vv2, fac;

    if (iset == 0) {
	while (r >= 1. || r == 0.) {
	    vv1 = simwe_rand() * 2. - 1.;
	    vv2 = simwe_rand() * 2. - 1.;
	    r = vv1 * vv1 + vv2 * vv2;
	}
	fac = sqrt(log(r) * -2. / r);
	gset = vv1 * fac;
	ret_val = vv2 * fac;
	iset = 1;
    }
    else {
	ret_val = gset;
	iset = 0;
    }
    return ret_val;
}				/* gasdev */

void gasdev_for_paralel(double *x, double *y)
{
    double r = 0., vv1, vv2, fac;

    while (r >= 1. || r == 0.) {
        vv1 = simwe_rand() * 2. - 1.;
        vv2 = simwe_rand() * 2. - 1.;
        r = vv1 * vv1 + vv2 * vv2;
    }
    fac = sqrt(log(r) * -2. / r);
    (*y) = vv1 * fac;
    (*x) = vv2 * fac;
}
