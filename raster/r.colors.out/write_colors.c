
/****************************************************************************
 *
 * MODULE:       r.colors.out
 *
 * AUTHOR(S):    Glynn Clements
 *
 * PURPOSE:      Allows export of the color table for a raster map layer.
 *
 * COPYRIGHT:    (C) 2008, 2010 Glynn Clements and the GRASS Development Team
 *
 *               This program is free software under the GNU General
 *               Public License (>=v2). Read the file COPYING that
 *               comes with GRASS for details.
 *
 ***************************************************************************/

#include "local_proto.h"

static FILE *fp;
static DCELL min, max;

static void write_rule(DCELL *val, int r, int g, int b, int perc)
{
    static DCELL v0;
    static int r0 = -1, g0 = -1, b0 = -1;

    if (v0 == *val && r0 == r && g0 == g && b0 == b)
	return;
    v0 = *val, r0 = r, g0 = g, b0 = b;

    if (perc)
	fprintf(fp, "%g%% %d:%d:%d\n", 100 * (*val - min) / (max - min), r, g, b);
    else
	fprintf(fp, "%g %d:%d:%d\n", *val, r, g, b);
}

void write_colors(struct Colors *colors, struct FPRange*range, const char *file, 
                  int perc)
{
    int i, count = 0;;

    Rast_get_fp_range_min_max(range, &min, &max);

    if (!file || strcmp(file, "-") == 0)
	fp = stdout;
    else {
	fp = fopen(file, "w");
	if (!fp)
	    G_fatal_error(_("Unable to open output file <%s>"), file);
    }

    if (colors->version < 0) {
	/* 3.0 format */
	CELL lo, hi;

	Rast_get_c_color_range(&lo, &hi, colors);

	for (i = lo; i <= hi; i++) {
	    unsigned char r, g, b, set;
	    DCELL val = (DCELL) i;
	    Rast_lookup_c_colors(&i, &r, &g, &b, &set, 1, colors);
	    write_rule(&val, r, g, b, perc);
	}
    }
    else {
	count = Rast_colors_count(colors);

	for (i = 0; i < count; i++) {
	    DCELL val1, val2;
	    unsigned char r1, g1, b1, r2, g2, b2;

	    Rast_get_fp_color_rule(
		&val1, &r1, &g1, &b1,
		&val2, &r2, &g2, &b2,
		colors, count - 1 - i);

	    write_rule(&val1, r1, g1, b1, perc);
	    write_rule(&val2, r2, g2, b2, perc);
	}
    }

    {
	int r, g, b;
	Rast_get_null_value_color(&r, &g, &b, colors);
	fprintf(fp, "nv %d:%d:%d\n", r, g, b);
	Rast_get_default_color(&r, &g, &b, colors);
	fprintf(fp, "default %d:%d:%d\n", r, g, b);
    }

    if (fp != stdout)
	fclose(fp);
}
