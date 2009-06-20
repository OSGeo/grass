#include <grass/gis.h>

void Rast_invert_colors(struct Colors *colors)
{
    colors->invert = !colors->invert;
}
