#include <grass/gis.h>

#include <grass/raster.h>
#include <grass/graphics.h>
#include "transport.h"

void R_get_location_with_box(int cx, int cy, int *wx, int *wy, int *button)
{
    trans->get_location_with_box(cx, cy, wx, wy, button);
}

void R_get_location_with_line(int cx, int cy, int *wx, int *wy, int *button)
{
    trans->get_location_with_line(cx, cy, wx, wy, button);
}

void R_get_location_with_pointer(int *wx, int *wy, int *button)
{
    trans->get_location_with_pointer(wx, wy, button);
}
