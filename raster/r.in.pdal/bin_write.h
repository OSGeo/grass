/*
 * r.in.pdal point binning output value calculation
 *
 * Copyright 2011-2015, 2020 by Markus Metz, and the GRASS Development Team
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

#endif /* __BIN_WRITE_H__ */
