#include "driver.h"
#include "driverlib.h"

void COM_Box_abs(double x1, double y1, double x2, double y2)
{
    if (driver->Box)
	(*driver->Box)(x1, y1, x2, y2);
}

