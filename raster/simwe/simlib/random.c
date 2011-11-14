/* random.c (simlib), 20.nov.2002, JH */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/bitmap.h>
#include <grass/linkm.h>

#include <grass/waterglobs.h>

int seeds(long int irand1, long int irand2)
{
    seed.is1 = irand1;
    seed.is2 = irand2;
    return 0;
}


int seedg(long int irand1, long int irand2)
{
    irand1 = seed.is1;
    irand2 = seed.is2;
    return 0;
}


double ulec(void)
{
    /* System generated locals */
    double ret_val;

    /* Local variables */
    long int k, iz;


    /*      uniform random number generator (combined type) */
    /*      P. L'Ecuyer, Commun. ACM, 31(1988)742 */
    /*      portable (32 bits arithmetics) */


    k = seed.is1 / 53668;
    seed.is1 -= k * 53668;
    seed.is1 = seed.is1 * 40014 - k * 12211;
    /*      is1=40014*(is1-k*53668)-k*12211 */
    if (seed.is1 < 0) {
	seed.is1 += 2147483563;
    }

    k = seed.is2 / 52774;
    seed.is2 -= k * 52774;
    seed.is2 = seed.is2 * 40692 - k * 3791;
    /*      is2=40692*(is2-k*52774)-k*3791 */
    if (seed.is2 < 0) {
	seed.is2 += 2147483399;
    }

    iz = seed.is1 - seed.is2;
    if (iz < 0) {
	iz += 2147483562;
    }
    ret_val = (double)iz *4.656613e-10;

    return ret_val;
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
	    vv1 = ulec() * 2. - 1.;
	    vv2 = ulec() * 2. - 1.;
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
