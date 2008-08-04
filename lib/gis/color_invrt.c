#include <grass/gis.h>

int G_invert_colors(struct Colors *colors)
{
    colors->invert = !colors->invert;

    return 0;
}
