/*!
   \file lib/imagery/iclass_statistics.c

   \brief Imagery library - functions for wx.iclass

   Computation based on training areas for supervised classification.
   Based on i.class module (GRASS 6).

   Computing statistical values (mean, min, max, ...) from given area
   perimeters for each band.

   Copyright (C) 1999-2007, 2011 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2).  Read the file COPYING that comes with GRASS for details.

   \author David Satnik, Central Washington University (original author)
   \author Markus Neteler <neteler itc.it> (i.class module)
   \author Bernhard Reiter <bernhard intevation.de> (i.class module)
   \author Brad Douglas <rez touchofmadness.com>(i.class module)
   \author Glynn Clements <glynn gclements.plus.com> (i.class module)
   \author Hamish Bowman <hamish_b yahoo.com> (i.class module)
   \author Jan-Oliver Wagner <jan intevation.de> (i.class module)
   \author Anna Kratochvilova <kratochanna gmail.com> (rewriting for wx.iclass)
   \author Vaclav Petras <wenzeslaus gmail.com> (rewriting for wx.iclass)
 */

#include <math.h>

#include <grass/imagery.h>
#include <grass/glocale.h>
#include <grass/colors.h>

#include "iclass_local_proto.h"

/*!
   \brief Initialize statistics.

   \param[out] statistics pointer to statistics structure
   \param category category (class)
   \param name class name
   \param color class color
   \param nstd standard deviation
 */
void I_iclass_init_statistics(IClass_statistics * statistics, int category,
			      const char *name, const char *color, float nstd)
{
    G_debug(4, "init_statistics() category=%d, name=%s, color=%s, nstd=%f",
	    category, name, color, nstd);

    statistics->cat = category;
    statistics->name = G_store(name);
    statistics->color = G_store(color);
    statistics->nstd = nstd;

    statistics->ncells = 0;
    statistics->nbands = 0;

    statistics->band_min = NULL;
    statistics->band_max = NULL;
    statistics->band_sum = NULL;
    statistics->band_mean = NULL;
    statistics->band_stddev = NULL;
    statistics->band_product = NULL;
    statistics->band_histo = NULL;
    statistics->band_range_min = NULL;
    statistics->band_range_max = NULL;
}

/*!
   \brief Allocate space for statistics.

   \param statistics pointer to statistics structure
   \param nbands number of band files
 */
void alloc_statistics(IClass_statistics * statistics, int nbands)
{
    int i;

    G_debug(4, "alloc_statistics()");

    statistics->nbands = nbands;

    statistics->band_min = (int *)G_calloc(nbands, sizeof(int));
    statistics->band_max = (int *)G_calloc(nbands, sizeof(int));
    statistics->band_sum = (float *)G_calloc(nbands, sizeof(float));
    statistics->band_mean = (float *)G_calloc(nbands, sizeof(float));
    statistics->band_stddev = (float *)G_calloc(nbands, sizeof(float));
    statistics->band_product = (float **)G_calloc(nbands, sizeof(float *));
    statistics->band_histo = (int **)G_calloc(nbands, sizeof(int *));
    statistics->band_range_min = (int *)G_calloc(nbands, sizeof(int));
    statistics->band_range_max = (int *)G_calloc(nbands, sizeof(int));

    for (i = 0; i < nbands; i++) {
	statistics->band_product[i] =
	    (float *)G_calloc(nbands, sizeof(float));
	statistics->band_histo[i] = (int *)G_calloc(MAX_CATS, sizeof(int));
    }
}

/*!
   \brief Free space allocated for statistics attributes.

   Frees all allocated arrays in statistics structure.

   \param statistics pointer to statistics structure
 */
void I_iclass_free_statistics(IClass_statistics * statistics)
{
    int i;

    G_debug(4, "free_statistics()");

    G_free((char *) statistics->name);
    G_free((char *) statistics->color);
    G_free(statistics->band_min);
    G_free(statistics->band_max);
    G_free(statistics->band_sum);
    G_free(statistics->band_mean);
    G_free(statistics->band_stddev);
    G_free(statistics->band_range_max);
    G_free(statistics->band_range_min);


    for (i = 0; i < statistics->nbands; i++) {
	G_free(statistics->band_histo[i]);
	G_free(statistics->band_product[i]);
    }
    G_free(statistics->band_histo);
    G_free(statistics->band_product);
}

/*!
   \brief Calculate statistics for all training areas.

   \param statistics pointer to statistics structure
   \param perimeters list of all area perimeters
   \param band_buffer buffer to read band rows into
   \param band_fd band files descriptors

   \return 1 on succes
   \return 0 on failure
 */
int make_all_statistics(IClass_statistics * statistics,
			IClass_perimeter_list * perimeters,
			CELL ** band_buffer, int *band_fd)
{
    int i, b, b2, nbands;

    float mean_value, stddev_value;

    G_debug(5, "make_all_statistics()");

    nbands = statistics->nbands;
    for (b = 0; b < nbands; b++) {
	statistics->band_sum[b] = 0.0;
	statistics->band_min[b] = MAX_CATS;
	statistics->band_max[b] = 0;
	for (b2 = 0; b2 < nbands; b2++)
	    statistics->band_product[b][b2] = 0.0;
	for (b2 = 0; b2 < MAX_CATS; b2++)
	    statistics->band_histo[b][b2] = 0;
    }

    for (i = 0; i < perimeters->nperimeters; i++) {
	if (!make_statistics
	    (statistics, &perimeters->perimeters[i], band_buffer, band_fd)) {
	    return 0;
	}
    }
    for (b = 0; b < statistics->nbands; b++) {
	mean_value = mean(statistics, b);
	stddev_value = stddev(statistics, b);

	statistics->band_stddev[b] = stddev_value;
	statistics->band_mean[b] = mean_value;

	band_range(statistics, b);
    }

    return 1;
}

/*!
   \brief Calculate statistics for one training area.

   \param[out] statistics pointer to statistics structure
   \param perimeter area perimeter
   \param band_buffer buffer to read band rows into
   \param band_fd band files descriptors

   \return 1 on succes
   \return 0 on failure
 */
int make_statistics(IClass_statistics * statistics,
		    IClass_perimeter * perimeter, CELL ** band_buffer,
		    int *band_fd)
{
    int b, b2;

    int value;

    int i;

    int x0, x1;

    int x, y;

    int ncells;

    int nbands;

    G_debug(5, "make_statistics()");

    nbands = statistics->nbands;

    if (perimeter->npoints % 2) {
	G_warning(_("prepare_signature: outline has odd number of points."));
	return 0;
    }

    ncells = 0;

    for (i = 1; i < perimeter->npoints; i += 2) {
	y = perimeter->points[i].y;
	if (y != perimeter->points[i - 1].y) {
	    G_warning(_("prepare_signature: scan line %d has odd number of points."),
		      (i + 1) / 2);
	    return 0;
	}
	read_band_row(band_buffer, band_fd, nbands, y);

	x0 = perimeter->points[i - 1].x - 1;
	x1 = perimeter->points[i].x - 1;

	if (x0 > x1) {
	    G_warning(_("signature: perimeter points out of order."));
	    return 0;
	}

	for (x = x0; x <= x1; x++) {
	    ncells++;		/* count interior points */
	    for (b = 0; b < nbands; b++) {
		value = band_buffer[b][x];
		G_debug(5, "make_statistics() band: %d, read value: %d (max: %d)",
                        b, value, MAX_CATS);
		if (value < 0 || value > MAX_CATS - 1) {
                    G_warning(_("Data error preparing signatures: value (%d) > num of cats (%d)"),
                              value, MAX_CATS);
		    return 0;
		}
		statistics->band_sum[b] += value;	/* sum for means */
		statistics->band_histo[b][value]++;	/* histogram */
		if (statistics->band_min[b] > value)
		    statistics->band_min[b] = value;	/* absolute min, max */
		if (statistics->band_max[b] < value) {
		    statistics->band_max[b] = value;
		    G_debug(5,
			    "make_statistics() statistics->band_max[%d]: %d",
			    b, statistics->band_max[b]);
		}

		for (b2 = 0; b2 <= b; b2++)	/* products for variance */
		    statistics->band_product[b][b2] +=
			value * band_buffer[b2][x];
	    }
	}
    }
    statistics->ncells += ncells;

    return 1;
}

/*!
   \brief Create raster map based on statistics.

   \param statistics pointer to statistics structure
   \param band_buffer buffer to read band rows into
   \param band_fd band files descriptors
   \param raster_name name of new raster map
 */
void create_raster(IClass_statistics * statistics, CELL ** band_buffer,
		   int *band_fd, const char *raster_name)
{
    int fd;

    CELL *buffer;

    int n;

    int col;

    int nbands;

    int row, nrows, ncols;

    struct Colors raster_colors;

    int r, g, b;

    int cell_in_ranges;

    nbands = statistics->nbands;

    /* build new raster based on current signature and Nstd */

    fd = Rast_open_c_new(raster_name);
    buffer = Rast_allocate_c_buf();
    nrows = Rast_window_rows();
    ncols = Rast_window_cols();

    for (row = 0; row < nrows; row++) {
	read_band_row(band_buffer, band_fd, nbands, row);
	for (col = 0; col < ncols; col++) {
	    buffer[col] = (CELL) 0;
	    cell_in_ranges = 1;
	    for (n = 0; n < nbands; n++) {
		if (band_buffer[n][col] < statistics->band_range_min[n] ||
		    band_buffer[n][col] > statistics->band_range_max[n]) {
		    /* out of at least 1 range */
		    cell_in_ranges = 0;
		}
	    }
	    if (cell_in_ranges) {
		/* if in range do the assignment */
		buffer[col] = (CELL) 1;
	    }
	}
	Rast_put_row(fd, buffer, CELL_TYPE);
    }
    Rast_close(fd);

    /* generate and write the color table for the mask */
    Rast_init_colors(&raster_colors);
    G_str_to_color(statistics->color, &r, &g, &b);
    Rast_set_c_color((CELL) 1, r, g, b, &raster_colors);
    Rast_write_colors(raster_name, G_mapset(), &raster_colors);
}

/* helpers */
/*!
   \brief Helper function for computing min and max range in one band.

   Computing min and max range value (distance from mean
   dependent on number od std ddevs).

   \param statistics pointer to statistics structure
   \param band band index
 */
void band_range(IClass_statistics * statistics, int band)
{
    float dist;

    dist = statistics->nstd * statistics->band_stddev[band];
    statistics->band_range_min[band] =
	statistics->band_mean[band] - dist + 0.5;
    statistics->band_range_max[band] =
	statistics->band_mean[band] + dist + 0.5;
}

/*!
   \brief Helper function for computing mean.

   Computing mean value of cell category values
   in one band within training area.

   \param statistics pointer to statistics structure
   \param band band index

   \return mean value
 */
float mean(IClass_statistics * statistics, int band)
{
    return statistics->band_sum[band] / statistics->ncells;
}

/*!
   \brief Helper function for standard deviation.

   Computing standard deviation of cell category values
   in one band within training area.

   \param statistics pointer to statistics structure
   \param band band index

   \return standard deviation
 */
float stddev(IClass_statistics * statistics, int band)
{
    return sqrt(var(statistics, band, band));
}

/*!
   \brief Helper function for computing variance.

   Computing variance of cell category values
   in one band within training area.

   \param statistics pointer to statistics structure
   \param band1 band index
   \param band2 band index

   \return variance

   \see var_signature
 */
float var(IClass_statistics * statistics, int band1, int band2)
{
    float product;

    float mean1, mean2;

    int n;

    product = statistics->band_product[band1][band2];
    mean1 = mean(statistics, band1);
    mean2 = mean(statistics, band2);
    n = statistics->ncells;

    return product / n - mean1 * mean2;
}

/*!
   \brief Helper function for computing variance for signature file.

   Computing variance of cell category values
   in one band within training area. Variance is computed
   in special way.

   \param statistics pointer to statistics structure
   \param band1 band index
   \param band2 band index

   \return variance

   \see var

   \todo verify the computation
 */
float var_signature(IClass_statistics * statistics, int band1, int band2)
{
    float product;

    float sum1, sum2;

    int n;

    product = statistics->band_product[band1][band2];
    sum1 = statistics->band_sum[band1];
    sum2 = statistics->band_sum[band2];
    n = statistics->ncells;

    return (product - sum1 * sum2 / n) / (n - 1);
}

/* getters */
/*!
   \brief Get number of bands.

   \param statistics pointer to statistics structure
   \param[out] nbands number of bands
 */
void I_iclass_statistics_get_nbands(IClass_statistics * statistics,
				    int *nbands)
{
    *nbands = statistics->nbands;
}

/*!
   \brief Get category (class).

   \param statistics pointer to statistics structure
   \param[out] cat category
 */
void I_iclass_statistics_get_cat(IClass_statistics * statistics, int *cat)
{
    *cat = statistics->cat;
}

/*!
   \brief Get category (class) name.

   \note \a name is pointer to already allocated
   const char * in \a statistics.
   You should not free it.

   \param statistics pointer to statistics structure
   \param[out] name category name
 */
void I_iclass_statistics_get_name(IClass_statistics * statistics,
				  const char **name)
{
    *name = statistics->name;
}

/*!
   \brief Get category (class) color.

   \note \a color is pointer to already allocated
   const char * in \a statistics.
   You should not free it.

   \param statistics pointer to statistics structure
   \param[out] color category color
 */
void I_iclass_statistics_get_color(IClass_statistics * statistics,
				   const char **color)
{
    *color = statistics->color;
}


/*!
   \brief Get number of cells in training areas.

   \param statistics pointer to statistics structure
   \param[out] ncells number of cells
 */
void I_iclass_statistics_get_ncells(IClass_statistics * statistics,
				    int *ncells)
{
    *ncells = statistics->ncells;
}

/*!
   \brief Get the multiplier of standard deviation.

   \param statistics pointer to statistics structure
   \param[out] nstd multiplier of standard deviation
 */
void I_iclass_statistics_get_nstd(IClass_statistics * statistics, float *nstd)
{
    *nstd = statistics->nstd;
}

/*!
   \brief Set the multiplier of standard deviation.

   \param statistics pointer to statistics structure
   \param nstd multiplier of standard deviation
 */
void I_iclass_statistics_set_nstd(IClass_statistics * statistics, float nstd)
{
    statistics->nstd = nstd;
}

/*!
   \brief Get minimum value in band.

   \param statistics pointer to statistics structure
   \param band band index
   \param[out] min minimum value

   \return 1 on success
   \return 0 band index out of range
 */
int I_iclass_statistics_get_min(IClass_statistics * statistics, int band,
				int *min)
{
    if (band >= statistics->nbands) {
	G_warning(_("Band index out of range"));
	return 0;
    }

    *min = statistics->band_min[band];

    return 1;
}

/*!
   \brief Get maximum value in band.

   \param statistics pointer to statistics structure
   \param band band index
   \param[out] max maximum value

   \return 1 on success
   \return 0 band index out of range
 */
int I_iclass_statistics_get_max(IClass_statistics * statistics, int band,
				int *max)
{
    if (band >= statistics->nbands) {
	G_warning(_("Band index out of range"));
	return 0;
    }

    *max = statistics->band_max[band];

    return 1;
}

/*!
   \brief Get sum of values in band.

   \param statistics pointer to statistics structure
   \param band band index
   \param[out] sum sum

   \return 1 on success
   \return 0 band index out of range
 */
int I_iclass_statistics_get_sum(IClass_statistics * statistics, int band,
				float *sum)
{
    if (band >= statistics->nbands) {
	G_warning(_("Band index out of range"));
	return 0;
    }

    *sum = statistics->band_sum[band];

    return 1;
}

/*!
   \brief Get mean of cell category values in band.

   \param statistics pointer to statistics structure
   \param band band index
   \param[out] mean mean

   \return 1 on success
   \return 0 band index out of range
 */
int I_iclass_statistics_get_mean(IClass_statistics * statistics, int band,
				 float *mean)
{
    if (band >= statistics->nbands) {
	G_warning(_("Band index out of range"));
	return 0;
    }

    *mean = statistics->band_mean[band];

    return 1;
}

/*!
   \brief Get standard deviation of cell category values in band.

   \param statistics pointer to statistics structure
   \param band band index
   \param[out] stddev standard deviation

   \return 1 on success
   \return 0 band index out of range
 */
int I_iclass_statistics_get_stddev(IClass_statistics * statistics, int band,
				   float *stddev)
{
    if (band >= statistics->nbands) {
	G_warning(_("Band index out of range"));
	return 0;
    }

    *stddev = statistics->band_stddev[band];

    return 1;
}

/*!
   \brief Get histogram value in band.

   Each band has one value for each raster cell category.
   Value is number of cells in category.

   \param statistics pointer to statistics structure
   \param band band index
   \param cat raster cell category
   \param[out] value number of cells in category

   \return 1 on success
   \return 0 band index or cell category value out of range
 */
int I_iclass_statistics_get_histo(IClass_statistics * statistics, int band,
				  int cat, int *value)
{
    if (band >= statistics->nbands) {
	G_warning(_("Band index out of range"));
	return 0;
    }
    if (cat >= MAX_CATS) {
	G_warning(_("Cell category value out of range"));
	return 0;
    }

    *value = statistics->band_histo[band][cat];

    return 1;
}

/*!
   \brief Get product value

   Product value of two bands is sum of products
   of cell category values of two bands.
   Only cells from training areas are taken into account.

   \param statistics statistics object
   \param band1 index of first band
   \param band2 index of second band
   \param[out] value product value

   \return 1 on success
   \return 0 band index out of range
 */
int I_iclass_statistics_get_product(IClass_statistics * statistics, int band1,
				    int band2, float *value)
{
    if (band1 >= statistics->nbands || band2 >= statistics->nbands) {
	G_warning(_("Band index out of range"));
	return 0;
    }

    *value = statistics->band_product[band1][band2];

    return 1;
}

/*!
   \brief Get minimum cell value based on mean and standard deviation for band.

   \param statistics pointer to statistics structure
   \param band band index
   \param[out] min minumum value

   \return 1 on success
   \return 0 band index out of range
 */
int I_iclass_statistics_get_range_min(IClass_statistics * statistics,
				      int band, int *min)
{
    if (band >= statistics->nbands) {
	G_warning(_("Band index out of range"));
	return 0;
    }

    *min = statistics->band_range_min[band];

    return 1;
}

/*!
   \brief Get maximum cell value based on mean and standard deviation for band.

   \param statistics pointer to statistics structure
   \param band band index
   \param[out] max maximum value

   \return 1 on success
   \return 0 band index out of range
 */
int I_iclass_statistics_get_range_max(IClass_statistics * statistics,
				      int band, int *max)
{
    if (band >= statistics->nbands) {
	G_warning(_("Band index out of range"));
	return 0;
    }

    *max = statistics->band_range_max[band];

    return 1;
}
