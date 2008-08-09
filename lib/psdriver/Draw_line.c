
#include "psdriver.h"

void PS_draw_line(double x1, double y1, double x2, double y2)
{
    output("%f %f %f %f LINE\n", x1, y1, x2, y2);
}
