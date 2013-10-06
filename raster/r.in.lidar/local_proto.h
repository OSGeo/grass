/****************************************************************************
 *
 * MODULE:       r.in.Lidar
 *               
 * AUTHOR(S):    Markus Metz
 *               Based on r.in.xyz by Hamish Bowman, Volker Wichmann
 *
 * PURPOSE:      Imports LAS LiDAR point clouds to a raster map using 
 *               aggregate statistics.
 *
 * COPYRIGHT:    (C) 2011 Markus Metz and the The GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#ifndef __LOCAL_PROTO_H__
#define __LOCAL_PROTO_H__


#include <grass/gis.h>
#include <liblas/capi/liblas.h>


#define BUFFSIZE 256

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

/* main.c */
void print_lasinfo(LASHeaderH, LASSRSH);
int scan_bounds(LASReaderH, int, int, double, struct Cell_head *);

/* support.c */
int blank_array(void *, int, int, RASTER_MAP_TYPE, int);
int update_n(void *, int, int, int);
int update_min(void *, int, int, int, RASTER_MAP_TYPE, double);
int update_max(void *, int, int, int, RASTER_MAP_TYPE, double);
int update_sum(void *, int, int, int, RASTER_MAP_TYPE, double);
int update_sumsq(void *, int, int, int, RASTER_MAP_TYPE, double);


#endif /* __LOCAL_PROTO_H__ */
