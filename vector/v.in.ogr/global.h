
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
 * COPYRIGHT:    (C) 2003 by the GRASS Development Team
 *
 *               This program is free software under the
 *               GNU General Public License (>=v2).
 *               Read the file COPYING that comes with GRASS
 *               for details.
 *
 * TODO: - make fixed field length of OFTIntegerList dynamic
 *       - several other TODOs below
**************************************************************/

#ifndef __GLOBAL_H__
#define __GLOBAL_H__


extern int n_polygons;
extern int n_polygon_boundaries;
extern double split_distance;

/* centroid structure */
typedef struct
{
    double x, y;
    struct line_cats *cats;
    int valid;
} CENTR;


#endif /* __GLOBAL_H__ */
