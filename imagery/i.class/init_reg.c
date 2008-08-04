#include <grass/gis.h>
#include "globals.h"


int init_region(struct region Region)
{
    Region.area.define = 0;
    Region.area.completed = 0;
    Region.area.filled = 0;
    Region.area.saved = 0;

    Region.npoints = 0;
    Region.view = NULL;

    Region.saved_npoints = 0;
    Region.saved_view = NULL;

    Region.vertex_npoints = 0;
    Region.perimeter_npoints = 0;


    return 0;
}
