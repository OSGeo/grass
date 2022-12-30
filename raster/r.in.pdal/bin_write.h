/*
 * r.in.pdal point binning output value calculation
 *
 * Copyright 2011-2015, 2020 by Markus Metz, and The GRASS Development Team
 * Authors:
 *  Markus Metz (r.in.lidar)
 *  Vaclav Petras (move code to separate functions)
 *  Maris Nartiss (refactoring for r.in.pdal)
 *
 * This program is free software licensed under the GPL (>=v2).
 * Read the COPYING file that comes with GRASS for details.
 *
 */

#ifndef __BIN_WRITE_H__
#define __BIN_WRITE_H__

#include <grass/raster.h>

/* forward declaration for point_binning.h */
struct BinIndex;

double get_sum(void *, void *, int, int, int, RASTER_MAP_TYPE);
void write_sum(void *, void *, void *, int, int, RASTER_MAP_TYPE);
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
void write_variance(void *, void *, void *, void *, int, int, RASTER_MAP_TYPE,
                    int);
void write_median(struct BinIndex *, void *, void *, int, int, RASTER_MAP_TYPE);
void write_mode(struct BinIndex *, void *, void *, int, int);
void write_percentile(struct BinIndex *, void *, void *, int, int,
                      RASTER_MAP_TYPE, int);
void write_skewness(struct BinIndex *, void *, void *, int, int,
                    RASTER_MAP_TYPE);
void write_trimmean(struct BinIndex *, void *, void *, int, int,
                    RASTER_MAP_TYPE, double);
void write_sidn(struct BinIndex *, void *, void *, int, int, int);
void write_ev(struct BinIndex *, void *, void *, int, int, RASTER_MAP_TYPE,
              int);
<<<<<<< HEAD
=======
void write_variance(void *, void *, void *,
                    void *, int, int, RASTER_MAP_TYPE, int);
void write_median(struct BinIndex *, void *,
                  void *, int, int, RASTER_MAP_TYPE);
=======
void write_variance(void *, void *, void *, void *, int, int, RASTER_MAP_TYPE,
                    int);
void write_median(struct BinIndex *, void *, void *, int, int, RASTER_MAP_TYPE);
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
void write_mode(struct BinIndex *, void *, void *, int, int);
void write_percentile(struct BinIndex *, void *, void *, int, int,
                      RASTER_MAP_TYPE, int);
void write_skewness(struct BinIndex *, void *, void *, int, int,
                    RASTER_MAP_TYPE);
void write_trimmean(struct BinIndex *, void *, void *, int, int,
                    RASTER_MAP_TYPE, double);
void write_sidn(struct BinIndex *, void *, void *, int, int, int);
<<<<<<< HEAD
void write_ev(struct BinIndex *, void *, void *,
              int, int, RASTER_MAP_TYPE, int);

>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))
=======
void write_ev(struct BinIndex *, void *, void *, int, int, RASTER_MAP_TYPE,
              int);
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))

#endif /* __BIN_WRITE_H__ */
