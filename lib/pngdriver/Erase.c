#include "pngdriver.h"

void PNG_Erase(void)
{
    int n = width * height;
    int i;

    for (i = 0; i < n; i++)
	grid[i] = background;

    modified = 1;
}
