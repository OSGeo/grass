#include "driver.h"
#include "driverlib.h"

int COM_Has_work(void)
{
    return driver->Do_work ? 1 : 0;
}

int COM_Work_stream(void)
{
    return driver->Work_stream ? (*driver->Work_stream) ()
	: -1;
}

void COM_Do_work(int opened)
{
    if (driver->Do_work)
	(*driver->Do_work) (opened);
}
