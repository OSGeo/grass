#include "driver.h"
#include "driverlib.h"

void COM_Box_abs(double x1, double y1, double x2, double y2)
{
    if (driver->Box)
	(*driver->Box)(x1, y1, x2, y2);
}

void COM_Box_rel(double x, double y)
{
    COM_Box_abs(cur_x, cur_y, cur_x + x, cur_y + y);
}
