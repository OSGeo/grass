#include "driver.h"
#include "driverlib.h"

void COM_Screen_left(int *index)
{
    *index = screen_left;
}

void COM_Screen_rite(int *index)
{
    *index = screen_right;
}

void COM_Screen_bot(int *index)
{
    *index = screen_bottom;
}

void COM_Screen_top(int *index)
{
    *index = screen_top;
}

void COM_Number_of_colors(int *ncolors)
{
    *ncolors = NCOLORS;
}
