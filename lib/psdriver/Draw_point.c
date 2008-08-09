#include "psdriver.h"

void PS_draw_point(double x, double y)
{
    output("%f %f POINT\n", x, y);
}
