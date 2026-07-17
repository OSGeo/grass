/****************************************************************
 *
 * MODULE:       v.in.ogr
 *
 * AUTHOR(S):    Radim Blazek
 *               Markus Neteler (spatial parm, projection support)
 *               Paul Kelly (projection support)
 *
 * PURPOSE:      Import OGR vectors
 *
 * SPDX-FileCopyrightText: 2003 Other GRASS authors
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * TODO: - make fixed field length of OFTIntegerList dynamic
 *       - several other TODOs below
 **************************************************************/

#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <gdal.h>
#include <gdal_version.h>
#include <ogr_api.h>

extern int n_polygons;
extern int n_polygon_boundaries;
extern double split_distance;

/* centroid structure */
typedef struct {
    double x, y;
    struct line_cats *cats;
    int valid;
} CENTR;

#endif /* __GLOBAL_H__ */
