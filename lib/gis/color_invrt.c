#include <grass/gis.h>

void G_invert_colors(struct Colors *colors)
{
    colors->invert = !colors->invert;
}
