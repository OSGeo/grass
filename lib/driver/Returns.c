#include "driver.h"
#include "driverlib.h"

double COM_Screen_left(void)
{
    return screen_left;
}

double COM_Screen_rite(void)
{
    return screen_right;
}

double COM_Screen_bot(void)
{
    return screen_bottom;
}

double COM_Screen_top(void)
{
    return screen_top;
}

int COM_Number_of_colors(void)
{
    return NCOLORS;
}
