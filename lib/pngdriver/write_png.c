#include <stdio.h>
#include <stdlib.h>
#include <png.h>

#include <grass/gis.h>
#include "pngdriver.h"


static void write_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
    png_size_t check;
    FILE *fp;

    if (png_ptr == NULL )
	return;

    fp = (FILE *) png_get_io_ptr(png_ptr);
    if ( fp == NULL )
        return;

    check = fwrite(data, 1, length, fp);

    if (check != length)
	G_fatal_error("PNG: Write Error");
}

static void output_flush(png_structp png_ptr)
{
    FILE *fp;

    if (png_ptr == NULL )
	return;

    fp = (FILE *) png_get_io_ptr(png_ptr);
    if ( fp == NULL )
	return;

    fflush( fp );
}

void write_png(void)
{
    static jmp_buf jbuf;
    static png_struct *png_ptr;
    static png_info *info_ptr;
    FILE *output;
    int x, y;
    unsigned int *p;
    png_bytep line;
    const char *str;
    int compress;

    png_ptr =
	png_create_write_struct(PNG_LIBPNG_VER_STRING, &jbuf, NULL, NULL);
    if (!png_ptr)
	G_fatal_error("PNG: couldn't allocate PNG structure");

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)
	G_fatal_error("PNG: couldn't allocate PNG structure");

    if (setjmp(png_jmpbuf(png_ptr)))
	G_fatal_error("error writing PNG file");

    output = fopen(png.file_name, "wb");
    if (!output)
	G_fatal_error("PNG: couldn't open output file %s", png.file_name);

    png_set_write_fn(png_ptr, output, write_data, output_flush);

    png_set_IHDR(png_ptr, info_ptr,
		 png.width, png.height, 8,
		 png.true_color ? PNG_COLOR_TYPE_RGB_ALPHA :
		 PNG_COLOR_TYPE_PALETTE, PNG_INTERLACE_NONE,
		 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    if (png.true_color)
	png_set_invert_alpha(png_ptr);
    else {
	png_color png_pal[256];
	int i;

	for (i = 0; i < 256; i++) {
	    png_pal[i].red   = png.palette[i][0];
	    png_pal[i].green = png.palette[i][1];
	    png_pal[i].blue  = png.palette[i][2];
	}

	png_set_PLTE(png_ptr, info_ptr, png_pal, 256);

	if (png.has_alpha) {
	    png_byte trans = (png_byte) 0;

	    png_set_tRNS(png_ptr, info_ptr, &trans, 1, NULL);
	}
    }

    str = getenv("GRASS_PNG_COMPRESSION");
    if (str && sscanf(str, "%d", &compress) == 1)
	png_set_compression_level(png_ptr, compress);

    png_write_info(png_ptr, info_ptr);

    line = G_malloc(png.width * 4);

    for (y = 0, p = png.grid; y < png.height; y++) {
	png_bytep q = line;

	if (png.true_color)
	    for (x = 0; x < png.width; x++, p++) {
		unsigned int c = *p;
		int r, g, b, a;

		png_get_pixel(c, &r, &g, &b, &a);
		*q++ = (png_byte) r;
		*q++ = (png_byte) g;
		*q++ = (png_byte) b;
		*q++ = (png_byte) a;
	    }
	else
	    for (x = 0; x < png.width; x++, p++, q++)
		*q = (png_byte) * p;

	png_write_row(png_ptr, line);
    }

    G_free(line);

    png_write_end(png_ptr, info_ptr);

    png_destroy_write_struct(&png_ptr, &info_ptr);

    fclose(output);
}
