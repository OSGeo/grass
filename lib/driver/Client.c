#include "driver.h"
#include "driverlib.h"

void COM_Client_Open(void)
{
    if (driver->Client_Open)
	(*driver->Client_Open) ();
}

void COM_Client_Close(void)
{
    if (driver->Client_Close)
	(*driver->Client_Close) ();
}
