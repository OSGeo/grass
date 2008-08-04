#include "cairodriver.h"

void write_ppm(void)
{
    char *mask_name = G_store(file_name);
    FILE *output, *mask;
    int x, y;

    output = fopen(file_name, "wb");
    if (!output)
	G_fatal_error("cairo: couldn't open output file %s", file_name);

    mask_name[strlen(mask_name) - 2] = 'g';

    mask = fopen(mask_name, "wb");
    if (!mask)
	G_fatal_error("cairo: couldn't open mask file %s", mask_name);

    G_free(mask_name);

    fprintf(output, "P6\n%d %d\n255\n", width, height);
    fprintf(mask, "P5\n%d %d\n255\n", width, height);

    for (y = 0; y < height; y++) {
	const unsigned int *row = (const unsigned int *)(grid + y * stride);

	for (x = 0; x < width; x++) {
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
