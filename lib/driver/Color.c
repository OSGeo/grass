#include <grass/colors.h>
#include "driver.h"
#include "driverlib.h"

int DRV_lookup_color(int r, int g, int b)
{
    if (driver->lookup_color)
	return (*driver->lookup_color) (r, g, b);
    return 0;
}

void DRV_color(int number)
{
    if (driver->color)
	(*driver->color) (number);
}


void COM_Color_RGB(unsigned char r, unsigned char g, unsigned char b)
{
    DRV_color(DRV_lookup_color(r, g, b));
}

void COM_Standard_color(int number)
{
    struct color_rgb rgb;

    if (number < 0 || number >= G_num_standard_colors())
	return;

    rgb = G_standard_color_rgb(number);
    COM_Color_RGB(rgb.r, rgb.g, rgb.b);
}
