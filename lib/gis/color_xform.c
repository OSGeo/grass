
/****************************************************************************
 *
 * MODULE:       gis library
 * AUTHOR(S):    Glynn Clements <glynn@gclements.plus.com>
 * COPYRIGHT:    (C) 2007 Glynn Clements
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *****************************************************************************/

/**********************************************************************
 *
 *  G_histogram_eq_colors (dst, src, statf)
 *
 *   struct Colors *dst         struct to hold new colors
 *   struct Colors *src         struct containing original colors
 *   struct Cell_stats *statf   cell stats info
 *
 *  Generates histogram equalized version of an existing color table from
 *  cell stats structure info.
 *
 **********************************************************************
 *
 *  G_log_colors (dst, src, samples)
 *
 *   struct Colors *dst         struct to hold new colors
 *   struct Colors *src         struct containing original colors
 *   int samples                number of samples
 *
 *  Generates logarithmically-scaled version of an existing color table.
 *
 **********************************************************************/
#include <grass/gis.h>
#include <math.h>

/*!
 * \brief make histogram-stretched version of existing color table
 *
 * Generates a histogram
 * contrast-stretched color table that goes from the histogram
 * information in the Cell_stats structure <b>statf.</b>  (See
 * Raster_Histograms).
 *
 *  \param dst
 *  \param src
 *  \param statf
 *  \return int
 */

int G_histogram_eq_colors(struct Colors *dst,
			  struct Colors *src, struct Cell_stats *statf)
{
    DCELL min, max;
    int red, grn, blu;
    long count, total, sum;
    CELL cat, prev;
    int first;

    G_init_colors(dst);

    G_get_d_color_range(&min, &max, src);

    G_get_default_color(&red, &grn, &blu, src);
    G_set_default_color(red, grn, blu, dst);

    G_get_null_value_color(&red, &grn, &blu, src);
    G_set_null_value_color(red, grn, blu, dst);

    total = 0;

    G_rewind_cell_stats(statf);
    while (G_next_cell_stat(&cat, &count, statf))
	if (count > 0)
	    total += count;

    if (total <= 0)
	return 0;

    sum = 0;
    prev = 0;
    first = 1;

    G_rewind_cell_stats(statf);
    while (G_next_cell_stat(&cat, &count, statf)) {
	int red2, grn2, blu2;
	DCELL x;

	if (count <= 0)
	    continue;

	x = min + (max - min) * (sum + count / 2.0) / total;
	G_get_d_raster_color(&x, &red2, &grn2, &blu2, src);

	if (!first)
	    G_add_color_rule(prev, red, grn, blu, cat, red2, grn2, blu2, dst);

	sum += count;
	first = 0;

	prev = cat;
	red = red2;
	grn = grn2;
	blu = blu2;
    }

    return 0;
}

/*!
 * \brief make histogram-stretched version of existing color table (FP version)
 *
 * Generates a histogram
 * contrast-stretched color table that goes from the histogram
 * information in the FP_stats structure <b>statf.</b>  (See
 * Raster_Histograms).
 *
 *  \param dst
 *  \param src
 *  \param statf
 *  \return void
 */

void G_histogram_eq_colors_fp(struct Colors *dst,
			      struct Colors *src, struct FP_stats *statf)
{
    DCELL min, max;
    int red, grn, blu;
    unsigned long sum;
    DCELL val;
    int first;
    int i;

    G_init_colors(dst);

    G_get_d_color_range(&min, &max, src);

    G_get_default_color(&red, &grn, &blu, src);
    G_set_default_color(red, grn, blu, dst);

    G_get_null_value_color(&red, &grn, &blu, src);
    G_set_null_value_color(red, grn, blu, dst);

    if (!statf->total)
	return;

    sum = 0;
    first = 1;

    for (i = 0; i <= statf->count; i++) {
	int red2, grn2, blu2;
	DCELL val2, x;

	val2 = statf->min + (statf->max - statf->min) * i / statf->count;
	if (statf->geometric)
	    val2 = exp(val2);
	if (statf->geom_abs)
	    val2 = exp(val2) - 1;
	if (statf->flip)
	    val2 = -val2;
	x = min + (max - min) * sum / statf->total;
	G_get_d_raster_color(&x, &red2, &grn2, &blu2, src);

	if (!first)
	    G_add_d_raster_color_rule(&val, red, grn, blu, &val2, red2, grn2, blu2, dst);
	first = 0;

	if (i == statf->count)
	    break;

	sum += statf->stats[i];

	val = val2;
	red = red2;
	grn = grn2;
	blu = blu2;
    }
}

/*!
 * \brief make logarithmically-scaled version of an existing color table
 *
 *  \param dst
 *  \param src
 *  \param samples
 *  \return int
 */

int G_log_colors(struct Colors *dst, struct Colors *src, int samples)
{
    DCELL min, max;
    double lmin, lmax;
    int red, grn, blu;
    DCELL prev;
    int i;

    G_init_colors(dst);

    G_get_d_color_range(&min, &max, src);

    lmin = log(min);
    lmax = log(max);

    G_get_default_color(&red, &grn, &blu, src);
    G_set_default_color(red, grn, blu, dst);

    G_get_null_value_color(&red, &grn, &blu, src);
    G_set_null_value_color(red, grn, blu, dst);

    for (i = 0; i <= samples; i++) {
	int red2, grn2, blu2;
	double lx;
	DCELL x, y;

	y = min + (max - min) * i / samples;
	G_get_d_raster_color(&y, &red2, &grn2, &blu2, src);

	if (i == 0)
	    x = min;
	else if (i == samples)
	    x = max;
	else {
	    lx = lmin + (lmax - lmin) * i / samples;
	    x = exp(lx);
	}

	if (i > 0)
	    G_add_d_raster_color_rule(&prev, red, grn, blu,
				      &x, red2, grn2, blu2,
				      dst);

	prev = x;

	red = red2;
	grn = grn2;
	blu = blu2;
    }

    return 0;
}

/*!
 * \brief make logarithmically-scaled version of an existing color table, allowing for signed values
 *
 *  \param dst
 *  \param src
 *  \param samples
 *  \return int
 */

int G_abs_log_colors(struct Colors *dst, struct Colors *src, int samples)
{
    DCELL min, max;
    double lmin, lmax;
    DCELL amax, lamax;
    int red, grn, blu;
    DCELL prev;
    int i;

    G_init_colors(dst);

    G_get_d_color_range(&min, &max, src);

    lmin = log(fabs(min) + 1.0);
    lmax = log(fabs(max) + 1.0);

    amax = fabs(min) > fabs(max) ? fabs(min) : fabs(max);
    lamax = lmin > lmax ? lmin : lmax;

    G_get_default_color(&red, &grn, &blu, src);
    G_set_default_color(red, grn, blu, dst);

    G_get_null_value_color(&red, &grn, &blu, src);
    G_set_null_value_color(red, grn, blu, dst);

    for (i = 0; i <= samples; i++) {
	int red2, grn2, blu2;
	double lx;
	DCELL x, y;

	y = min + (max - min) * i / samples;
	G_get_d_raster_color(&y, &red2, &grn2, &blu2, src);

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
	    G_add_d_raster_color_rule(&x0, red, grn, blu,
				      &x1, red2, grn2, blu2,
				      dst);
	    x0 = -x0;
	    x1 = -x1;
	    G_add_d_raster_color_rule(&x0, red, grn, blu,
				      &x1, red2, grn2, blu2,
				      dst);
	}

	prev = x;

	red = red2;
	grn = grn2;
	blu = blu2;
    }

    return 0;
}

