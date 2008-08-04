
#include "driver.h"
#include "driverlib.h"

void COM_Respond(void)
{
    if (driver->Respond)
	(*driver->Respond) ();
}
