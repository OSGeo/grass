#include "pngdriver.h"

void PNG_draw_point(int x, int y)
{
    if (x < clip_left || x >= clip_rite || y < clip_top || y >= clip_bot)
	return;

    grid[y * width + x] = currentColor;

    modified = 1;
}
