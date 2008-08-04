#include <grass/gis.h>
int G_shift_colors(int shift, struct Colors *colors)
{
    colors->shift += (DCELL) shift;

    return 0;
}

int G_shift_d_colors(DCELL shift, struct Colors *colors)
{
    colors->shift += shift;

    return 0;
}
