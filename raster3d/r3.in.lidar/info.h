/*
 * lidar-related printing and scanning functions
 *
 * Authors:
 *  Markus Metz (r.in.lidar)
 *  Vaclav Petras (refactoring and various additions)
 *
 * Copyright 2011-2016 by Vaclav Petras, and the GRASS Development Team
 *
 * This program is free software licensed under the GPL (>=v2).
 * Read the COPYING file that comes with GRASS for details.
 *
 */

#ifndef __INFO_H__
#define __INFO_H__

#include <grass/gis.h>
#include <grass/raster.h>
#include <liblas/capi/liblas.h>

void print_lasinfo(LASHeaderH, LASSRSH);
int scan_bounds(LASReaderH, int, int, int, double, struct Cell_head *);

#endif /* __INFO_H__ */
