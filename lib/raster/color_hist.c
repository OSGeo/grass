/*!
 * \file lib/raster/color_hist.c
 *
 * \brief Raster Library - histogram grey scale colors
 *
 * (C) 2007-2009 Glynn Clements and the GRASS Development Team
 *
 * This program is free software under the GNU General Public License 
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Glynn Clements <glynn@gclements.plus.com>
 */

#include <math.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/colors.h>

/*!
 * \brief Make histogram-stretched grey colors
 *
 * Generates a histogram contrast-stretched grey scale color table
 * that goes from the, histogram information in the Cell_stats
 * structure (see \ref Raster_Histograms).
 *
 * Color range is 0-255.
 *
 * \param colors pointer to Colors structure which holds color info
 * \param statf pointer to Cell_stats structure which holds cell stats info
 */
void Rast_make_histogram_eq_colors(struct Colors *colors,
				   struct Cell_stats *statf)
{
    long count, total;
    CELL prev = 0, cat, val2;
    double span, sum;
    int first;
    int x, grey;
    int R, G, B;

    Rast_init_colors(colors);

    G_str_to_color(DEFAULT_BG_COLOR, &R, &G, &B);
    Rast_set_null_value_color(R, G, B, colors);

    total = 0;

    Rast_rewind_cell_stats(statf);
    while (Rast_next_cell_stat(&cat, &count, statf))
	if (count > 0)
	    total += count;
    if (total <= 0)
	return;

    span = total / 256.0;
    first = 1;
    grey = 0;
    sum = 0.0;

    Rast_rewind_cell_stats(statf);
    while (Rast_next_cell_stat(&cat, &count, statf)) {
	if (count <= 0)
	    continue;
	x = (sum + (count / 2.0)) / span;
	if (x < 0)
	    x = 0;
	else if (x > 255)
	    x = 255;
	sum += count;
	if (first) {
	    prev = cat;
	    grey = x;
	    first = 0;
	}
	else if (grey != x) {
	    val2 = cat - 1;
	    Rast_add_c_color_rule(&prev, grey, grey, grey, &val2, grey, grey,
				  grey, colors);
	    grey = x;
	    prev = cat;
	}
    }
    if (!first) {
	Rast_add_c_color_rule(&prev, grey, grey, grey, &cat, grey, grey, grey,
			      colors);
    }
}

/*!
   \brief Generates histogram with normalized log transformed grey scale.

   Generates histogram with normalized log transformed grey scale from
   cell stats structure info. Color range is 0-255.

   \param colors pointer to Colors structure which holds color info
   \param statf pointer to Cell_stats structure which holds cell stats info
   \param min minimum value
   \param max maximum value
 */
void Rast_make_histogram_log_colors(struct Colors *colors,
				    struct Cell_stats *statf, int min,
				    int max)
{
    long count, total;
    double lmin, lmax;
    CELL prev = 0, cat, val2;
    int first;
    int x, grey;
    int R, G, B;

    Rast_init_colors(colors);

    G_str_to_color(DEFAULT_BG_COLOR, &R, &G, &B);
    Rast_set_null_value_color(R, G, B, colors);

    total = 0;

    Rast_rewind_cell_stats(statf);
    while (Rast_next_cell_stat(&cat, &count, statf))
	if (count > 0)
	    total += count;
    if (total <= 0)
	return;

    first = 1;
    grey = 0;

    lmin = log(min);
    lmax = log(max);

    Rast_rewind_cell_stats(statf);
    while (Rast_next_cell_stat(&cat, &count, statf)) {
	if (count <= 0)
	    continue;

	/* log transform normalized */
	x = (int)(255 * (log(cat) - lmin) / (lmax - lmin));

	if (x < 0)
	    x = 0;
	else if (x > 255)
	    x = 255;
	if (first) {
	    prev = cat;
	    grey = x;
	    first = 0;
	}
	else if (grey != x) {
	    val2 = cat - 1;
	    Rast_add_c_color_rule(&prev, grey, grey, grey, &val2, grey, grey,
				  grey, colors);
	    grey = x;
	    prev = cat;
	}
    }
    if (!first) {
	Rast_add_c_color_rule(&prev, grey, grey, grey, &cat, grey, grey, grey,
			      colors);
    }
}
