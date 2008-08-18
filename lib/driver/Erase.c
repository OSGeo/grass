#include "driver.h"
#include "driverlib.h"

void COM_Erase(void)
{
    double top, bot, left, rite;

    if (driver->Erase) {
	(*driver->Erase) ();
	return;
    }

    COM_Get_window(&top, &bot, &left, &rite);

    COM_Box_abs(left, top, rite, bot);
}
