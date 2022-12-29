
/****************************************************************************
 *
 * MODULE:       r.buffer
 *
 * AUTHOR(S):    Michael Shapiro - CERL
 *
 * PURPOSE:      This program creates distance zones from non-zero
 *               cells in a grid layer. Distances are specified in
 *               meters (on the command-line). Window does not have to
 *               have square cells. Works both for planimetric
 *               (UTM, State Plane) and lat-long.
 *
 * COPYRIGHT:    (C) 2005 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
****************************************************************************/

#ifndef __DISTANCE_H__
#define __DISTANCE_H__

#include <grass/gis.h>

struct Distance
{
    int ncols;
    int prev_ncols;
    double dist;
    char *label;
};

typedef unsigned char MAPTYPE;

#define MAX_DIST 254
/* if MAPTYPE is changed to unsigned short, MAX_DIST can be set to 2^16-2
 * (if short is 2 bytes)
 */

extern struct Distance *distances;
extern int ndist;
extern int wrap_ncols;
extern MAPTYPE *map;
extern struct Cell_head window;
extern int minrow, maxrow, mincol, maxcol;
extern char *pgm_name;
extern double meters_to_grid;
extern double ns_to_ew_squared;
extern int count_rows_with_data;

#define MAPINDEX(r,c) ((size_t)(r) * window.cols + (c))
#define ZONE_INCR 2

#define FEET_TO_METERS 0.3048
#define MILES_TO_METERS 1609.344
#define NAUT_MILES_TO_METERS 1852.0
#define KILOMETERS_TO_METERS 1000.0

#endif /* __DISTANCE_H__ */
