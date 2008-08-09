#include "driver.h"
#include "driverlib.h"

void COM_Move_abs(double x, double y)
{
    cur_x = x;
    cur_y = y;
}

void COM_Move_rel(double x, double y)
{
    cur_x += x;
    cur_y += y;
}
