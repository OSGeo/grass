#include "driver.h"
#include "driverlib.h"

void COM_Cont_abs(double x, double y)
{
    DRV_draw_line(cur_x, cur_y, x, y);
    cur_x = x;
    cur_y = y;
}

void COM_Cont_rel(double x, double y)
{
    COM_Cont_abs(cur_x + x, cur_y + y);
}
