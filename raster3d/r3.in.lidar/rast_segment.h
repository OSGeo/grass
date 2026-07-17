/*
 * segment library convenient wrapper functions
 *
 * Authors:
 *  Vaclav Petras
 *
 * SPDX-FileCopyrightText: 2015 Vaclav Petras
 * SPDX-FileCopyrightText: Other GRASS authors
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 */

#ifndef GRASS_RAST_SEGMENT_H
#define GRASS_RAST_SEGMENT_H

#include <grass/raster.h>
#include <grass/segment.h>

void rast_segment_open(SEGMENT *segment, const char *name,
                       RASTER_MAP_TYPE *map_type);
int rast_segment_get_value_xy(SEGMENT *base_segment,
                              struct Cell_head *input_region,
                              RASTER_MAP_TYPE rtype, double x, double y,
                              double *value);

#endif /* GRASS_RAST_SEGMENT_H */
