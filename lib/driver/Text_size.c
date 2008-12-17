#include <math.h>
#include "driver.h"
#include "driverlib.h"

void COM_Text_size(double x, double y)
{
    text_size_x = x;
    text_size_y = y;
    matrix_valid = 0;
}

void COM_Text_rotation(double val)
{
    text_rotation = val;
    text_sinrot = sin(M_PI * text_rotation / 180.0);
    text_cosrot = cos(M_PI * text_rotation / 180.0);
    matrix_valid = 0;
}
