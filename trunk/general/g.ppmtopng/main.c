/*
 * MODULE:       g.ppmtopng
 * AUTHOR(S):    Glynn Clements
 * PURPOSE:      g.ppmtopng isn't meant for end users. It's an internal tool for use by
 *               the script to generate thumbnails for the r.colors manual page.
 * COPYRIGHT:    (C) 2009 by Glynn Clements and the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <png.h>
#include <grass/gis.h>
#include <grass/glocale.h>

static int width, height;
static unsigned char *buf;

static void read_ppm(const char *filename)
{
    FILE *input;
    int x, y;
    int maxval;
    unsigned char *p;

    input = fopen(filename, "rb");
    if (!input)
	G_fatal_error(_("Unable to open input file %s"), filename);

    if (fscanf(input, "P6 %d %d %d", &width, &height, &maxval) != 3)
	G_fatal_error(_("Invalid input file %s"), filename);

    fgetc(input);

    buf = G_malloc(width * height * 3);

    p = buf;
    for (y = 0; y < height; y++) {
	for (x = 0; x < width; x++) {
	    int r = fgetc(input);
	    int g = fgetc(input);
	    int b = fgetc(input);

	    *p++ = (unsigned char) (r * 255 / maxval);
	    *p++ = (unsigned char) (g * 255 / maxval);
	    *p++ = (unsigned char) (b * 255 / maxval);
	}
    }

    fclose(input);
}

static void write_png(const char *filename)
{
    static jmp_buf jbuf;
    static png_struct *png_ptr;
    static png_info *info_ptr;
    FILE *output;
    int y;
    unsigned char *p;

    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, &jbuf, NULL, NULL);
    if (!png_ptr)
	G_fatal_error(_("Unable to allocate PNG structure"));

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)
	G_fatal_error(_("Unable to allocate PNG structure"));

    if (setjmp(png_jmpbuf(png_ptr)))
	G_fatal_error(_("Error writing PNG file"));

    output = fopen(filename, "wb");
    if (!output)
	G_fatal_error(_("Unable to open output file %s"), filename);

    png_init_io(png_ptr, output);

    png_set_IHDR(png_ptr, info_ptr,
		 width, height,
		 8, PNG_COLOR_TYPE_RGB,
		 PNG_INTERLACE_NONE,
		 PNG_COMPRESSION_TYPE_DEFAULT,
		 PNG_FILTER_TYPE_DEFAULT);

    png_set_invert_alpha(png_ptr);

    png_write_info(png_ptr, info_ptr);

    for (y = 0, p = buf; y < height; y++, p += 3 * width)
	png_write_row(png_ptr, p);

    png_write_end(png_ptr, info_ptr);

    png_destroy_write_struct(&png_ptr, &info_ptr);

    fclose(output);
}

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct
    {
	struct Option *in, *out;
    } opt;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("general"));
    G_add_keyword(_("display"));
    module->description = _("Converts between PPM/PGM and PNG image formats.");

    opt.in = G_define_standard_option(G_OPT_F_INPUT);

    opt.out = G_define_standard_option(G_OPT_F_OUTPUT);

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    read_ppm(opt.in->answer);
    write_png(opt.out->answer);

    return 0;
}
