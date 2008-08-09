#include "driver.h"
#include "driverlib.h"

void COM_Set_window(double t, double b, double l, double r)
{
    if (driver->Set_window)
	(*driver->Set_window) (t, b, l, r);
}
