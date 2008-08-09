#include <math.h>
#include "pngdriver.h"

void PNG_draw_point(double fx, double fy)
{
    int x = (int) floor(fx + 0.5);
    int y = (int) floor(fy + 0.5);

    if (x < clip_left || x >= clip_rite || y < clip_top || y >= clip_bot)
	return;

    grid[y * width + x] = currentColor;

    modified = 1;
}
