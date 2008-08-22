#include "pngdriver.h"

void PNG_Erase(void)
{
    int n = png.width * png.height;
    int i;

    for (i = 0; i < n; i++)
	png.grid[i] = png.background;

    png.modified = 1;
}
