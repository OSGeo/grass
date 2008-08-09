#include "driver.h"
#include "driverlib.h"

void COM_Erase(void)
{
    double top, bot, left, rite;

    if (driver->Erase) {
	(*driver->Erase) ();
	return;
    }

    top  = COM_Screen_top();
    bot  = COM_Screen_bot();
    rite = COM_Screen_rite();
    left = COM_Screen_left();

    COM_Box_abs(left, top, rite, bot);
}
