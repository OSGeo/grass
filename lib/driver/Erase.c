#include "driver.h"
#include "driverlib.h"

void COM_Erase(void)
{
    int top, bot, left, rite;

    if (driver->Erase) {
	(*driver->Erase) ();
	return;
    }

    COM_Screen_top(&top);
    COM_Screen_bot(&bot);
    COM_Screen_rite(&rite);
    COM_Screen_left(&left);

    COM_Box_abs(left, top, rite, bot);
}
