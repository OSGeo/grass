#include "driver.h"
#include "driverlib.h"

void COM_Move_abs(int x, int y)
{
    cur_x = x;
    cur_y = y;
}

void COM_Move_rel(int x, int y)
{
    cur_x += x;
    cur_y += y;
}
