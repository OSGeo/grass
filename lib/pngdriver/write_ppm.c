/*!
  \file lib/pngdriver/write_ppm.c

  \brief GRASS png display driver - write PPM image (lower level functions)

  (C) 2007-2014 by Glynn Clements and the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Glynn Clements
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <grass/gis.h>
#include "pngdriver.h"

void write_ppm(void)
{
    FILE *output;
    int x, y;
    unsigned int *p;

    output = fopen(png.file_name, "wb");
    if (!output)
	G_fatal_error("PNG: couldn't open output file %s", png.file_name);

    fprintf(output, "P6\n%d %d\n255\n", png.width, png.height);

    for (y = 0, p = png.grid; y < png.height; y++) {
	for (x = 0; x < png.width; x++, p++) {
	    unsigned int c = *p;
	    int r, g, b, a;

	    png_get_pixel(c, &r, &g, &b, &a);

	    fputc((unsigned char)r, output);
	    fputc((unsigned char)g, output);
	    fputc((unsigned char)b, output);
	}
    }

    fclose(output);
}

void write_pgm(void)
{
    char *mask_name = G_store(png.file_name);
    FILE *output;
    int x, y;
    unsigned int *p;

    mask_name[strlen(mask_name) - 2] = 'g';

    output = fopen(mask_name, "wb");
    if (!output)
	G_fatal_error("PNG: couldn't open mask file %s", mask_name);

    G_free(mask_name);

    fprintf(output, "P5\n%d %d\n255\n", png.width, png.height);

    for (y = 0, p = png.grid; y < png.height; y++) {
	for (x = 0; x < png.width; x++, p++) {
	    unsigned int c = *p;
	    int r, g, b, a;

	    png_get_pixel(c, &r, &g, &b, &a);

	    fputc((unsigned char)(255 - a), output);
	}
    }

    fclose(output);
}
