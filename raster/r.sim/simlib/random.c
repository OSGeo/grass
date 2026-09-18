/* random.c (simlib), 20.nov.2002, JH */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/bitmap.h>
#include <grass/linkm.h>

#include <grass/simlib.h>

double simwe_rand(struct G_rand48_state *state)
{
    return G_drand48_r(state);
} /* ulec */

/* Two independent standard normal deviates, by the polar form of the
 * Box-Muller transform. The method yields them in pairs, so both are
 * returned rather than one being cached between calls: a cache would be
 * state shared behind the caller's back, which is what the caller-owned
 * generator exists to avoid. */
void gasdev(struct G_rand48_state *state, double *x, double *y)
{
    double r = 0.0, vv1 = 0.0, vv2 = 0.0, fac = 0.0;

    while (r >= 1. || r == 0.) {
        vv1 = simwe_rand(state) * 2. - 1.;
        vv2 = simwe_rand(state) * 2. - 1.;
        r = vv1 * vv1 + vv2 * vv2;
    }
    fac = sqrt(log(r) * -2. / r);
    (*y) = vv1 * fac;
    (*x) = vv2 * fac;
}
