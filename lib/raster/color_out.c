/*!
  \file lib/raster/color_out.c
  
  \brief Raster Library - Print color table
  
  (C) 2010-2011 by the GRASS Development Team
  
  This program is free software under the GNU General Public
  License (>=v2). Read the file COPYING that comes with GRASS
  for details.
  
  \author Glynn Clements
*/

#include <grass/raster.h>

static void write_rule(DCELL *val, DCELL *min, DCELL *max, int r, int g, int b,
		       FILE *fp, int perc)
{
    static DCELL v0;
    static int r0 = -1, g0 = -1, b0 = -1;

    if (v0 == *val && r0 == r && g0 == g && b0 == b)
	return;
    v0 = *val, r0 = r, g0 = g, b0 = b;

    if (perc)
	fprintf(fp, "%g%% %d:%d:%d\n", 100 * (*val - *min) / (*max - *min), r, g, b);
    else
	fprintf(fp, "%g %d:%d:%d\n", *val, r, g, b);
}

/*!
  \brief Print color table

  \param colors pointer to Colors structure
  \param min,max minimum and maximum value for percentage output (used only when \p perc is non-zero)
  \param fp file where to print color table rules
  \param perc TRUE for percentage output
*/
void Rast_print_colors(struct Colors *colors, DCELL min, DCELL max, FILE *fp, 
		       int perc)
{
    int i, count;
    
    count = 0;
    if (colors->version < 0) {
	/* 3.0 format */
	CELL lo, hi;

	Rast_get_c_color_range(&lo, &hi, colors);

	for (i = lo; i <= hi; i++) {
	    unsigned char r, g, b, set;
	    DCELL val = (DCELL) i;
	    Rast_lookup_c_colors(&i, &r, &g, &b, &set, 1, colors);
	    write_rule(&val, &min, &max, r, g, b, fp, perc);
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

	    write_rule(&val1, &min, &max, r1, g1, b1, fp, perc);
	    write_rule(&val2, &min, &max, r2, g2, b2, fp, perc);
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
