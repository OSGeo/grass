#include "driver.h"
#include "driverlib.h"

void COM_Erase(void)
{
    if (driver->Erase)
	(*driver->Erase)();
}
