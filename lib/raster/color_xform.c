/*!
 * \file lib/raster/color_xform.c
 *
 * \brief Raster Library - Colors management
 *
 * (C) 2001-2009 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License 
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Original author CERL
 */

#include <math.h>

#include <grass/gis.h>
#include <grass/raster.h>

/*!
 * \brief Make histogram-stretched version of existing color table
 *
 * Generates a histogram contrast-stretched color table that goes from
 * the histogram information in the Cell_stats structure <i>statf</i>.
 * (See \ref Raster_Histograms).
 *
 * \param[out] dst struct to hold new colors
 * \param src struct containing original colors
 * \param statf cell stats info
 */
void Rast_histogram_eq_colors(struct Colors *dst,
			      struct Colors *src, struct Cell_stats *statf)
{
    DCELL min, max;
    int red, grn, blu;
    int red2, grn2, blu2;
    long count, total, sum;
    CELL cat, prev;
    int first;

    Rast_init_colors(dst);

    Rast_get_d_color_range(&min, &max, src);

    Rast_get_default_color(&red, &grn, &blu, src);
    Rast_set_default_color(red, grn, blu, dst);

    Rast_get_null_value_color(&red, &grn, &blu, src);
    Rast_set_null_value_color(red, grn, blu, dst);

    total = 0;

    Rast_rewind_cell_stats(statf);
    while (Rast_next_cell_stat(&cat, &count, statf))
	if (count > 0)
	    total += count;

    if (total <= 0)
	return;

    sum = 0;
    prev = 0;
    first = 1;

    Rast_rewind_cell_stats(statf);
    while (Rast_next_cell_stat(&cat, &count, statf)) {
	DCELL x;

	if (count <= 0)
	    continue;

	x = min + (max - min) * (sum + count / 2.0) / total;
	Rast_get_d_color(&x, &red2, &grn2, &blu2, src);

	sum += count;

	if (!first && red2 == red && blu2 == blu && grn2 == grn)
	    continue;

	if (!first)
	    Rast_add_c_color_rule(&prev, red, grn, blu,
				  &cat, red2, grn2, blu2,
				  dst);

	first = 0;

	prev = cat;
	red = red2;
	grn = grn2;
	blu = blu2;
    }

    if (!first && cat > prev)
	Rast_add_c_color_rule(&prev, red, grn, blu,
			      &cat, red2, grn2, blu2,
			      dst);
}

/*!
 * \brief Make histogram-stretched version of existing color table (FP version)
 *
 * Generates a histogram contrast-stretched color table that goes from
 * the histogram information in the FP_stats structure <b>statf.</b>
 * (See \ref Raster_Histograms).
 *
 * \param[out] dst struct to hold new colors
 * \param src struct containing original colors
 * \param statf cell stats info
 */
void Rast_histogram_eq_fp_colors(struct Colors *dst,
				 struct Colors *src, struct FP_stats *statf)
{
    DCELL min, max;
    int red, grn, blu;
    int red2, grn2, blu2;
    unsigned long sum;
    DCELL val, val2;
    int first;
    int i;

    Rast_init_colors(dst);

    Rast_get_d_color_range(&min, &max, src);

    Rast_get_default_color(&red, &grn, &blu, src);
    Rast_set_default_color(red, grn, blu, dst);

    Rast_get_null_value_color(&red, &grn, &blu, src);
    Rast_set_null_value_color(red, grn, blu, dst);

    if (!statf->total)
	return;

    sum = 0;
    first = 1;

    for (i = 0; i <= statf->count; i++) {
	DCELL x;

	val2 = statf->min + (statf->max - statf->min) * i / statf->count;
	if (statf->geometric)
	    val2 = exp(val2);
	if (statf->geom_abs)
	    val2 = exp(val2) - 1;
	if (statf->flip)
	    val2 = -val2;
	x = min + (max - min) * sum / statf->total;
	Rast_get_d_color(&x, &red2, &grn2, &blu2, src);

	if (i < statf->count)
	    sum += statf->stats[i];

	if (!first && red2 == red && blu2 == blu && grn2 == grn)
	    continue;

	if (!first)
	    Rast_add_d_color_rule(&val, red, grn, blu,
				  &val2, red2, grn2, blu2,
				  dst);

	first = 0;

	if (i == statf->count)
	    break;

	val = val2;
	red = red2;
	grn = grn2;
	blu = blu2;
    }

    if (!first && val2 > val)
	Rast_add_d_color_rule(&val, red, grn, blu,
			      &val2, red2, grn2, blu2,
			      dst);
}

/*!
 * \brief Make logarithmically-scaled version of an existing color table
 *
 * \param[out] dst struct to hold new colors
 * \param src struct containing original colors
 * \param samples number of samples
 */

void Rast_log_colors(struct Colors *dst, struct Colors *src, int samples)
{
    DCELL min, max;
    double lmin, lmax;
    int red, grn, blu;
    DCELL prev;
    int i;

    Rast_init_colors(dst);

    Rast_get_d_color_range(&min, &max, src);

    lmin = log(min);
    lmax = log(max);

    Rast_get_default_color(&red, &grn, &blu, src);
    Rast_set_default_color(red, grn, blu, dst);

    Rast_get_null_value_color(&red, &grn, &blu, src);
    Rast_set_null_value_color(red, grn, blu, dst);

    for (i = 0; i <= samples; i++) {
	int red2, grn2, blu2;
	double lx;
	DCELL x, y;

	y = min + (max - min) * i / samples;
	Rast_get_d_color(&y, &red2, &grn2, &blu2, src);

	if (i == 0)
	    x = min;
	else if (i == samples)
	    x = max;
	else {
	    lx = lmin + (lmax - lmin) * i / samples;
	    x = exp(lx);
	}

	if (i > 0)
	    Rast_add_d_color_rule(&prev, red, grn, blu,
				  &x, red2, grn2, blu2, dst);

	prev = x;

	red = red2;
	grn = grn2;
	blu = blu2;
    }
}

/*!
 * \brief Make logarithmically-scaled version of an existing color
 * table, allowing for signed values
 *
 * \param[out] dst struct to hold new colors
 * \param src struct containing original colors
 * \param samples number of samples
 */
void Rast_abs_log_colors(struct Colors *dst, struct Colors *src, int samples)
{
    DCELL min, max;
    double lmin, lmax;
    DCELL amax, lamax;
    int red, grn, blu;
    DCELL prev;
    int i;

    Rast_init_colors(dst);

    Rast_get_d_color_range(&min, &max, src);

    lmin = log(fabs(min) + 1.0);
    lmax = log(fabs(max) + 1.0);

    amax = fabs(min) > fabs(max) ? fabs(min) : fabs(max);
    lamax = lmin > lmax ? lmin : lmax;

    Rast_get_default_color(&red, &grn, &blu, src);
    Rast_set_default_color(red, grn, blu, dst);

    Rast_get_null_value_color(&red, &grn, &blu, src);
    Rast_set_null_value_color(red, grn, blu, dst);

    for (i = 0; i <= samples; i++) {
	int red2, grn2, blu2;
	double lx;
	DCELL x, y;

	y = min + (max - min) * i / samples;
	Rast_get_d_color(&y, &red2, &grn2, &blu2, src);

	if (i == 0)
	    x = 1;
	else if (i == samples)
	    x = amax;
	else {
	    lx = 0 + lamax * i / samples;
	    x = exp(lx);
	}

	if (i > 0) {
	    DCELL x0 = prev, x1 = x;

	    Rast_add_d_color_rule(&x0, red, grn, blu,
				  &x1, red2, grn2, blu2, dst);
	    x0 = -x0;
	    x1 = -x1;
	    Rast_add_d_color_rule(&x0, red, grn, blu,
				  &x1, red2, grn2, blu2, dst);
	}

	prev = x;

	red = red2;
	grn = grn2;
	blu = blu2;
    }
}
