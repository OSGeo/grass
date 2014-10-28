
#include <stdio.h>
#include <stdlib.h>

#include <grass/gis.h>
#include <grass/colors.h>
#include "pngdriver.h"

static int r_shift, g_shift, b_shift, a_shift;
static int Red[256], Grn[256], Blu[256];

static void set_color(int i, int red, int grn, int blu)
{
    png.palette[i][0] = red;
    png.palette[i][1] = grn;
    png.palette[i][2] = blu;
    png.palette[i][3] = 0;
}

static void init_colors_rgb(void)
{
    if (G_is_little_endian()) {
	b_shift = 0;
	g_shift = 8;
	r_shift = 16;
	a_shift = 24;
    }
    else {
	b_shift = 24;
	g_shift = 16;
	r_shift = 8;
	a_shift = 0;
    }
}

static void init_colors_indexed(void)
{
    int n_pixels;
    int r, g, b;
    int i;

    n_pixels = 0;

    if (png.has_alpha)
	/* transparent color should be the first!
	 * Its RGB value doesn't matter since we fake RGB-to-index. */
	set_color(n_pixels++, 0, 0, 0);

    for (r = 0; r < 6; r++) {
	for (g = 0; g < 6; g++) {
	    for (b = 0; b < 6; b++) {
		int red = r * 0xFF / 5;
		int grn = g * 0xFF / 5;
		int blu = b * 0xFF / 5;

		set_color(n_pixels++, red, grn, blu);
	    }
	}
    }

    while (n_pixels < 256)
	set_color(n_pixels++, 0, 0, 0);

    for (i = 0; i < 256; i++) {
	int k = i * 6 / 256;

	Red[i] = k * 6 * 6;
	Grn[i] = k * 6;
	Blu[i] = k;
    }
}

void png_init_color_table(void)
{
    if (png.true_color)
	init_colors_rgb();
    else
	init_colors_indexed();
}

static int get_color_rgb(int r, int g, int b, int a)
{
    return (r << r_shift) + (g << g_shift) + (b << b_shift) + (a << a_shift);
}

static int get_color_indexed(int r, int g, int b, int a)
{
    if (png.has_alpha && a >= 128)
	return 0;

    return Red[r] + Grn[g] + Blu[b] + png.has_alpha;
}

static void get_pixel_rgb(unsigned int pixel, int *r, int *g, int *b, int *a)
{
    *r = (pixel >> r_shift) & 0xFF;
    *g = (pixel >> g_shift) & 0xFF;
    *b = (pixel >> b_shift) & 0xFF;
    *a = (pixel >> a_shift) & 0xFF;
}

static void get_pixel_indexed(unsigned int pixel, int *r, int *g, int *b,
			      int *a)
{
    *r = png.palette[pixel][0];
    *g = png.palette[pixel][1];
    *b = png.palette[pixel][2];
    *a = png.palette[pixel][3];
}


void png_get_pixel(unsigned int pixel, int *r, int *g, int *b, int *a)
{
    if (png.true_color)
	get_pixel_rgb(pixel, r, g, b, a);
    else
	get_pixel_indexed(pixel, r, g, b, a);
}

unsigned int png_get_color(int r, int g, int b, int a)
{
    return png.true_color
	? get_color_rgb(r, g, b, a)
	: get_color_indexed(r, g, b, a);
}

