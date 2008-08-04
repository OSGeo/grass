#include "driver.h"
#include "driverlib.h"

void COM_Line_width(int width)
{
    if (driver->Line_width)
	(*driver->Line_width) (width);
}
