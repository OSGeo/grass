/*
 * r.in.xyz
 *
 *   Calculates univariate statistics from the non-null cells of a GRASS 
 *   raster map
 *
 *   Copyright 2006 by M. Hamish Bowman, and The GRASS Development Team
 *   Author: M. Hamish Bowman, University of Otago, Dunedin, New Zealand
 *
 *   This program is free software licensed under the GPL (>=v2).
 *   Read the COPYING file that comes with GRASS for details.
 *
 *   This program is intended as a replacement for the GRASS 5 
 *   s.cellstats module.
 */

#ifndef __LOCAL_PROTO_H__
#define __LOCAL_PROTO_H__


#include <grass/gis.h>


#define BUFFSIZE 1024

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
int scan_bounds(FILE *, int, int, int, int, char *, int, int, double, double);

/* support.c */
int blank_array(void *, int, int, RASTER_MAP_TYPE, int);
int update_n(void *, int, int, int);
int update_min(void *, int, int, int, RASTER_MAP_TYPE, double);
int update_max(void *, int, int, int, RASTER_MAP_TYPE, double);
int update_sum(void *, int, int, int, RASTER_MAP_TYPE, double);
int update_sumsq(void *, int, int, int, RASTER_MAP_TYPE, double);


#endif /* __LOCAL_PROTO_H__ */
