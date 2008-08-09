#include "driver.h"
#include "driverlib.h"

void COM_Box_abs(double x1, double y1, double x2, double y2)
{
    double x[4], y[4];

    if (driver->Box) {
	(*driver->Box) (x1, y1, x2, y2);
	return;
    }

    x[0] = x1;
    y[0] = y1;
    x[1] = x1;
    y[1] = y2;
    x[2] = x2;
    y[2] = y2;
    x[3] = x2;
    y[3] = y1;

    COM_Polygon_abs(x, y, 4);
}

void COM_Box_rel(double x, double y)
{
    COM_Box_abs(cur_x, cur_y, cur_x + x, cur_y + y);
}
