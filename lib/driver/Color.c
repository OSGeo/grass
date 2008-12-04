#include <grass/colors.h>
#include "driver.h"
#include "driverlib.h"

void COM_Color_RGB(unsigned char r, unsigned char g, unsigned char b)
{
    if (driver->Color)
	(*driver->Color)(r, g, b);
}

void COM_Standard_color(int number)
{
    struct color_rgb rgb;

    if (number < 0 || number >= G_num_standard_colors())
	return;

    rgb = G_standard_color_rgb(number);
    COM_Color_RGB(rgb.r, rgb.g, rgb.b);
}
