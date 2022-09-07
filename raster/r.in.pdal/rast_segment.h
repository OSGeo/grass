
 /****************************************************************************
 *
 * MODULE:    r.in.pdal
 *
 * AUTHOR(S): Vaclav Petras
 *
 * PURPOSE:   This file is a wrapper for a subset of Segment functions
 *
 * COPYRIGHT: (C) 2015 by Vaclav Petras and the GRASS Development Team
 *
 *            This program is free software under the GNU General Public
 *            License (>=v2). Read the file COPYING that comes with
 *            GRASS for details.
 *
 *****************************************************************************/

#ifndef GRASS_RAST_SEGMENT_H
#define GRASS_RAST_SEGMENT_H

#include <grass/raster.h>
#include <grass/segment.h>

void rast_segment_open(SEGMENT * segment, const char *name,
                       RASTER_MAP_TYPE * map_type);
int rast_segment_get_value_xy(SEGMENT * base_segment,
                              struct Cell_head *input_region,
                              RASTER_MAP_TYPE rtype, double x, double y,
                              double *value);

#endif /* GRASS_RAST_SEGMENT_H */
