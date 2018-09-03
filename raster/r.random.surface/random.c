/* random.c                                                             */
#include <grass/gis.h>
#include "ransurf.h"

/* ran1() returns a double with a value between 0.0 and 1.0             */
double ran1(void)
{
    G_debug(2, "ran1()");

    return G_drand48();
}
