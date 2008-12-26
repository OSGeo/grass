/* gasdev.c                                                             */
#include <math.h>

#include "ransurf.h"
#include "local_proto.h"

/* GasDev() returns a random double with a mean of 0.0 and a standard   */
/*      deviation of 1.0.                                               */
double GasDev(void)
{
    double fac, r, v1, v2;

    do {
	v1 = 2.0 * ran1() - 1.0;
	v2 = 2.0 * ran1() - 1.0;
	r = v1 * v1 + v2 * v2;
    } while (r >= 1.0);

    fac = sqrt(-2.0 * log(r) / r);
    G_debug(3, "(v2 * fac):%.12lf", v2 * fac);

    return (v2 * fac);
}
