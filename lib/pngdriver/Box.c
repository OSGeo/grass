
#include "pngdriver.h"

void PNG_Box_abs(int x1, int y1, int x2, int y2)
{
    int tmp;
    int x, y;

    if (x1 > x2)
	tmp = x1, x1 = x2, x2 = tmp;

    if (y1 > y2)
	tmp = y1, y1 = y2, y2 = tmp;

    if (x2 < 0 || x1 > width)
	return;

    if (y2 < 0 || y1 > height)
	return;

    if (x1 < clip_left)
	x1 = clip_left;

    if (x2 > clip_rite)
	x2 = clip_rite;

    if (y1 < clip_top)
	y1 = clip_top;

    if (y2 > clip_bot)
	y2 = clip_bot;

    for (y = y1; y < y2; y++) {
	unsigned int *p = &grid[y * width + x1];

	for (x = x1; x < x2; x++)
	    *p++ = currentColor;
    }

    modified = 1;
}
