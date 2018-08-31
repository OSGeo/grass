
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

#include <gdal.h>
#include <gdal_version.h>
#include <ogr_api.h>

/* define type of input datasource
 * as of GDAL 2.2, all functions having as argument a GDAL/OGR dataset 
 * must use the GDAL version, not the OGR version */
#if GDAL_VERSION_NUM >= 2020000
typedef GDALDatasetH ds_t;
#define ds_getlayerbyindex(ds, i)	GDALDatasetGetLayer((ds), (i))
#define ds_close(ds)			GDALClose(ds)
#else
typedef OGRDataSourceH ds_t;
#define ds_getlayerbyindex(ds, i)	OGR_DS_GetLayer((ds), (i))
#define ds_close(ds)			OGR_DS_Destroy(ds)
#endif

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
