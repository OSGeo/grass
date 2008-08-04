
#include <stdio.h>
#include <stdlib.h>
#include <png.h>

#include <grass/gis.h>
#include "pngdriver.h"

void read_png(void)
{
    static jmp_buf jbuf;
    static png_struct *png_ptr;
    static png_info *info_ptr;
    FILE *input;
    int x, y;
    unsigned int *p;
    png_bytep line;
    png_uint_32 i_width, i_height;
    int depth, color_type;

    png_ptr =
	png_create_read_struct(PNG_LIBPNG_VER_STRING, &jbuf, NULL, NULL);
    if (!png_ptr)
	G_fatal_error("PNG: couldn't allocate PNG structure");

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)
	G_fatal_error("PNG: couldn't allocate PNG structure");

    if (setjmp(png_jmpbuf(png_ptr)))
	G_fatal_error("error reading PNG file");

    input = fopen(file_name, "rb");
    if (!input)
	G_fatal_error("PNG: couldn't open output file %s", file_name);

    png_init_io(png_ptr, input);

    png_read_info(png_ptr, info_ptr);

    png_get_IHDR(png_ptr, info_ptr, &i_width, &i_height,
		 &depth, &color_type, NULL, NULL, NULL);

    if (depth != 8)
	G_fatal_error("PNG: input file is not 8-bit");

    if (i_width != width || i_height != height)
	G_fatal_error
	    ("PNG: input file has incorrect dimensions: expected: %dx%d got: %lux%lu",
	     width, height, i_width, i_height);

    if (true_color) {
	if (color_type != PNG_COLOR_TYPE_RGB_ALPHA)
	    G_fatal_error("PNG: input file is not RGBA");
    }
    else {
	if (color_type != PNG_COLOR_TYPE_PALETTE)
	    G_fatal_error("PNG: input file is not indexed color");
    }

    if (!true_color && has_alpha) {
	png_bytep trans;
	int num_trans;

	png_get_tRNS(png_ptr, info_ptr, &trans, &num_trans, NULL);

	if (num_trans != 1 || trans[0] != 0)
	    G_fatal_error("PNG: input file has invalid palette");
    }

    if (true_color)
	png_set_invert_alpha(png_ptr);
    else {
	png_colorp png_pal;
	int num_palette;
	int i;

	png_get_PLTE(png_ptr, info_ptr, &png_pal, &num_palette);

	if (num_palette > 256)
	    num_palette = 256;

	for (i = 0; i < num_palette; i++) {
	    png_palette[i][0] = png_pal[i].red;
	    png_palette[i][1] = png_pal[i].green;
	    png_palette[i][2] = png_pal[i].blue;
	}
    }

    line = G_malloc(width * 4);

    for (y = 0, p = grid; y < height; y++) {
	png_bytep q = line;

	png_read_row(png_ptr, q, NULL);

	if (true_color)
	    for (x = 0; x < width; x++, p++) {
		int r = *q++;
		int g = *q++;
		int b = *q++;
		int a = *q++;
		unsigned int c = get_color(r, g, b, a);

		*p = c;
	    }
	else
	    for (x = 0; x < width; x++, p++, q++)
		*p = (png_byte) * q;
    }

    G_free(line);

    png_read_end(png_ptr, NULL);

    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

    fclose(input);
}
