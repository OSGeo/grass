/*!
  \file lib/cairodriver/write_ppm.c

  \brief GRASS cairo display driver - write PPM image (lower level functions)

  (C) 2007-2008 by Lars Ahlzen and the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Lars Ahlzen <lars ahlzen.com> (original contibutor)
  \author Glynn Clements  
*/

#include <grass/glocale.h>

#include "cairodriver.h"

void cairo_write_ppm(void)
{
    char *mask_name = G_store(ca.file_name);
    FILE *output, *mask;
    int x, y;

    output = fopen(ca.file_name, "wb");
    if (!output)
	G_fatal_error(_("Cairo: unable to open output file <%s>"),
			ca.file_name);

    mask_name[strlen(mask_name) - 2] = 'g';

    mask = fopen(mask_name, "wb");
    if (!mask)
	G_fatal_error(_("Cairo: unable to open mask file <%s>"),
		      mask_name);

    G_free(mask_name);

    fprintf(output, "P6\n%d %d\n255\n", ca.width, ca.height);
    fprintf(mask, "P5\n%d %d\n255\n", ca.width, ca.height);

    for (y = 0; y < ca.height; y++) {
	const unsigned int *row = (const unsigned int *)(ca.grid + y * ca.stride);

	for (x = 0; x < ca.width; x++) {
	    unsigned int c = row[x];
	    int a = (c >> 24) & 0xFF;
	    int r = (c >> 16) & 0xFF;
	    int g = (c >> 8) & 0xFF;
	    int b = (c >> 0) & 0xFF;

	    if (a > 0 && a < 0xFF) {
		r = r * 0xFF / a;
		g = g * 0xFF / a;
		b = b * 0xFF / a;
	    }

	    fputc((unsigned char)r, output);
	    fputc((unsigned char)g, output);
	    fputc((unsigned char)b, output);
	    fputc((unsigned char)a, mask);
	}
    }

    fclose(output);
    fclose(mask);
}
