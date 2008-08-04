
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

    output = fopen(file_name, "wb");
    if (!output)
	G_fatal_error("PNG: couldn't open output file %s", file_name);

    fprintf(output, "P6\n%d %d\n255\n", width, height);

    for (y = 0, p = grid; y < height; y++) {
	for (x = 0; x < width; x++, p++) {
	    unsigned int c = *p;
	    int r, g, b, a;

	    get_pixel(c, &r, &g, &b, &a);

	    fputc((unsigned char)r, output);
	    fputc((unsigned char)g, output);
	    fputc((unsigned char)b, output);
	}
    }

    fclose(output);
}

void write_pgm(void)
{
    char *mask_name = G_store(file_name);
    FILE *output;
    int x, y;
    unsigned int *p;

    mask_name[strlen(mask_name) - 2] = 'g';

    output = fopen(mask_name, "wb");
    if (!output)
	G_fatal_error("PNG: couldn't open mask file %s", mask_name);

    G_free(mask_name);

    fprintf(output, "P5\n%d %d\n255\n", width, height);

    for (y = 0, p = grid; y < height; y++) {
	for (x = 0; x < width; x++, p++) {
	    unsigned int c = *p;
	    int r, g, b, a;

	    get_pixel(c, &r, &g, &b, &a);

	    fputc((unsigned char)(255 - a), output);
	}
    }

    fclose(output);
}
