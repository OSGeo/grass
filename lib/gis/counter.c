#include <grass/gis.h>

void G_init_counter(struct Counter *c, int v)
{
    c->value = v;
}

int G_counter_next(struct Counter *c)
{
    return c->value++;
}

