
#include <stdio.h>
#include <stdlib.h>
#include <png.h>

#include <grass/gis.h>
#include "pngdriver.h"

static void read_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
  png_size_t check;
  FILE *fp;

  if (png_ptr == NULL )
    return;

  fp = (FILE *) png_get_io_ptr(png_ptr);

  if ( fp == NULL )
    return;

  /* fread() returns 0 on error, so it is OK to store this in a png_size_t
   * instead of an int, which is what fread() actually returns.
   */
  check = fread(data, 1, length, fp);

  if (check != length)
    G_fatal_error("PNG: Read Error");
}

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

    input = fopen(png.file_name, "rb");
    if (!input)
	G_fatal_error("PNG: couldn't open output file %s", png.file_name);

    png_set_read_fn(png_ptr, input, read_data);

    png_read_info(png_ptr, info_ptr);

    png_get_IHDR(png_ptr, info_ptr, &i_width, &i_height,
		 &depth, &color_type, NULL, NULL, NULL);

    if (depth != 8)
	G_fatal_error("PNG: input file is not 8-bit");

    if (i_width != png.width || i_height != png.height)
	G_fatal_error
	    ("PNG: input file has incorrect dimensions: expected: %dx%d got: %lux%lu",
	     png.width, png.height, (unsigned long) i_width, (unsigned long) i_height);

    if (png.true_color) {
	if (color_type != PNG_COLOR_TYPE_RGB_ALPHA)
	    G_fatal_error("PNG: input file is not RGBA");
    }
    else {
	if (color_type != PNG_COLOR_TYPE_PALETTE)
	    G_fatal_error("PNG: input file is not indexed color");
    }

    if (!png.true_color && png.has_alpha) {
	png_bytep trans;
	int num_trans;

	png_get_tRNS(png_ptr, info_ptr, &trans, &num_trans, NULL);

	if (num_trans != 1 || trans[0] != 0)
	    G_fatal_error("PNG: input file has invalid palette");
    }

    if (png.true_color)
	png_set_invert_alpha(png_ptr);
    else {
	png_colorp png_pal;
	int num_palette;
	int i;

	png_get_PLTE(png_ptr, info_ptr, &png_pal, &num_palette);

	if (num_palette > 256)
	    num_palette = 256;

	for (i = 0; i < num_palette; i++) {
	    png.palette[i][0] = png_pal[i].red;
	    png.palette[i][1] = png_pal[i].green;
	    png.palette[i][2] = png_pal[i].blue;
	}
    }

    line = G_malloc(png.width * 4);

    for (y = 0, p = png.grid; y < png.height; y++) {
	png_bytep q = line;

	png_read_row(png_ptr, q, NULL);

	if (png.true_color)
	    for (x = 0; x < png.width; x++, p++) {
		int r = *q++;
		int g = *q++;
		int b = *q++;
		int a = *q++;
		unsigned int c = png_get_color(r, g, b, a);

		*p = c;
	    }
	else
	    for (x = 0; x < png.width; x++, p++, q++)
		*p = (png_byte) * q;
    }

    G_free(line);

    png_read_end(png_ptr, NULL);

    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

    fclose(input);
}
