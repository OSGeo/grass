#include <grass/gis.h>
void Rast_shift_colors(int shift, struct Colors *colors)
{
    colors->shift += (DCELL) shift;
}

void Rast_shift_d_colors(DCELL shift, struct Colors *colors)
{
    colors->shift += shift;
}
