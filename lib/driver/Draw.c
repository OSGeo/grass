#include "driver.h"
#include "driverlib.h"


void DRV_draw_bitmap(int ncols, int nrows, int threshold,
		     const unsigned char *buf)
{
    if (driver->draw_bitmap)
	(*driver->draw_bitmap) (ncols, nrows, threshold, buf);
}

void DRV_draw_line(int x0, int y0, int x1, int y1)
{
    if (driver->draw_line)
	(*driver->draw_line) (x0, y0, x1, y1);
}

void DRV_draw_point(int x, int y)
{
    if (driver->draw_point)
	(*driver->draw_point) (x, y);
}
