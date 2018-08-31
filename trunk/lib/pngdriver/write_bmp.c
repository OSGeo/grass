/*!
  \file lib/pngdriver/write_bmp.c

  \brief GRASS png display driver - write bitmap (lower level functions)

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

static unsigned char *put_2(unsigned char *p, unsigned int n)
{
    *p++ = n & 0xFF;
    n >>= 8;
    *p++ = n & 0xFF;
    return p;
}

static unsigned char *put_4(unsigned char *p, unsigned int n)
{
    *p++ = n & 0xFF;
    n >>= 8;
    *p++ = n & 0xFF;
    n >>= 8;
    *p++ = n & 0xFF;
    n >>= 8;
    *p++ = n & 0xFF;
    return p;
}

static void make_bmp_header(unsigned char *p)
{
    *p++ = 'B';
    *p++ = 'M';

    p = put_4(p, HEADER_SIZE + png.width * png.height * 4);
    p = put_4(p, 0);
    p = put_4(p, HEADER_SIZE);

    p = put_4(p, 40);
    p = put_4(p, png.width);
    p = put_4(p, -png.height);
    p = put_2(p, 1);
    p = put_2(p, 32);
    p = put_4(p, 0);
    p = put_4(p, png.width * png.height * 4);
    p = put_4(p, 0);
    p = put_4(p, 0);
    p = put_4(p, 0);
    p = put_4(p, 0);
}

void write_bmp(void)
{
    char header[HEADER_SIZE];
    FILE *output;
    int x, y;
    unsigned int *p;

    output = fopen(png.file_name, "wb");
    if (!output)
	G_fatal_error("PNG: couldn't open output file %s", png.file_name);

    memset(header, 0, sizeof(header));
    make_bmp_header(header);
    fwrite(header, sizeof(header), 1, output);

    for (y = 0, p = png.grid; y < png.height; y++) {
	for (x = 0; x < png.width; x++, p++) {
	    unsigned int c = *p;
	    int r, g, b, a;

	    png_get_pixel(c, &r, &g, &b, &a);

	    fputc((unsigned char)b, output);
	    fputc((unsigned char)g, output);
	    fputc((unsigned char)r, output);
	    fputc((unsigned char)a, output);
	}
    }

    fclose(output);
}
