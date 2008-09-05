#include "driver.h"
#include "driverlib.h"

void COM_Line_abs(double x1, double y1, double x2, double y2)
{
    DRV_draw_line(x1, y1, x2, y2);
}

