/*
 * lidar-related printing and scanning functions
 *
 * Authors:
 *  Markus Metz (r.in.lidar)
 *  Vaclav Petras (refactoring and various additions)
 *
 * SPDX-FileCopyrightText: 2011-2016 Vaclav Petras
 * SPDX-FileCopyrightText: Other GRASS authors
 * SPDX-License-Identifier: GPL-2.0-or-later
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
