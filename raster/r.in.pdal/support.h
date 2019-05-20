
 /****************************************************************************
 *
 * MODULE:    r.in.pdal
 *
 * AUTHOR(S): Vaclav Petras
 *            Based on r.in.xyz and r.in.lidar by Markus Metz,
 *            Hamish Bowman, Volker Wichmann
 *
 * PURPOSE:   Imports LAS LiDAR point clouds to a raster map using
 *            aggregate statistics.
 *
 * COPYRIGHT: (C) 2011-2019 by Vaclav Petras and the GRASS Development Team
 *
 *            This program is free software under the GNU General Public
 *            License (>=v2). Read the file COPYING that comes with
 *            GRASS for details.
 *
 *****************************************************************************/

#ifndef __SUPPORT_H__
#define __SUPPORT_H__

#include <grass/raster.h>

#define METHOD_NONE        0
#define METHOD_N           1
#define METHOD_MIN         2
#define METHOD_MAX         3
#define METHOD_RANGE       4
#define METHOD_SUM         5
#define METHOD_MEAN        6
#define METHOD_STDDEV      7
#define METHOD_VARIANCE    8
#define METHOD_COEFF_VAR   9
#define METHOD_MEDIAN     10
#define METHOD_PERCENTILE 11
#define METHOD_SKEWNESS   12
#define METHOD_TRIMMEAN   13

int blank_array(void *, int, int, RASTER_MAP_TYPE, int);

int update_n(void *, int, int, int);
int update_min(void *, int, int, int, RASTER_MAP_TYPE, double);
int update_max(void *, int, int, int, RASTER_MAP_TYPE, double);
int update_sum(void *, int, int, int, RASTER_MAP_TYPE, double);
int update_sumsq(void *, int, int, int, RASTER_MAP_TYPE, double);

int row_array_get_value_row_col(void *array, int arr_row, int arr_col,
                                int cols, RASTER_MAP_TYPE rtype,
                                double *value);


#endif /* __SUPPORT_H__ */
