#include "driver.h"
#include "driverlib.h"

void COM_Text_size(double x, double y)
{
    text_size_x = x / 25.0;
    text_size_y = y / 25.0;
}

void COM_Text_rotation(double val)
{
    text_rotation = val;
}
