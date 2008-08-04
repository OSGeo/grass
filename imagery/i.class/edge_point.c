#include "globals.h"

int edge_point(register int x, register int y)
{
    register int n;

    n = Region.perimeter_npoints++;
    Region.perimeter[n].x = x;
    Region.perimeter[n].y = y;

    return 0;
}
