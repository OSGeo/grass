
#include <math.h>
#include "pngdriver.h"

void PNG_Box(double fx1, double fy1, double fx2, double fy2)
{
    int x1 = (int) floor(fx1 + 0.5);
    int y1 = (int) floor(fy1 + 0.5);
    int x2 = (int) floor(fx2 + 0.5);
    int y2 = (int) floor(fy2 + 0.5);
    int tmp;
    int x, y;

    if (x1 > x2)
	tmp = x1, x1 = x2, x2 = tmp;

    if (y1 > y2)
	tmp = y1, y1 = y2, y2 = tmp;

    if (x2 < 0 || x1 > png.width)
	return;

    if (y2 < 0 || y1 > png.height)
	return;

    if (x1 < png.clip_left)
	x1 = png.clip_left;

    if (x2 > png.clip_rite)
	x2 = png.clip_rite;

    if (y1 < png.clip_top)
	y1 = png.clip_top;

    if (y2 > png.clip_bot)
	y2 = png.clip_bot;

    for (y = y1; y < y2; y++) {
	unsigned int *p = &png.grid[y * png.width + x1];

	for (x = x1; x < x2; x++)
	    *p++ = png.current_color;
    }

    png.modified = 1;
}
