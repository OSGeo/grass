#include "driver.h"
#include "driverlib.h"

void COM_Set_window(int t, int b, int l, int r)
{
    if (driver->Set_window)
	(*driver->Set_window) (t, b, l, r);
}
