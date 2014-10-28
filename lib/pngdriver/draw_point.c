#include <math.h>
#include "pngdriver.h"

void PNG_draw_point(double fx, double fy)
{
    int x = (int) floor(fx + 0.5);
    int y = (int) floor(fy + 0.5);

    if (x < png.clip_left || x >= png.clip_rite || y < png.clip_top || y >= png.clip_bot)
	return;

    png.grid[y * png.width + x] = png.current_color;

    png.modified = 1;
}
