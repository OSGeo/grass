
#include <stdio.h>
#include <stdlib.h>

#include <grass/gis.h>
#include <grass/colors.h>
#include "psdriver.h"

void init_color_table(void)
{
    NCOLORS = true_color ? (1 << 24) : (1 << 8);
}

static int get_color_rgb(int r, int g, int b)
{
    return (r << 16) + (g << 8) + b;
}

static int get_color_gray(int r, int g, int b)
{
    return (int)(r * 0.299 + g * 0.587 + b * 0.114);
}

int PS_lookup_color(int r, int g, int b)
{
    return true_color ? get_color_rgb(r, g, b)
	: get_color_gray(r, g, b);
}
