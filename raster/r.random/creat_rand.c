#include <grass/gis.h>


long make_rand(void)
{
    return G_lrand48();
}

void init_rand(void)
{
    /* FIXME - allow seed to be specified for repeatability */
    G_srand48_auto();
}

