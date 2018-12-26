
/****************************************************************************
 *
 * MODULE:       r.viewshed
 *
 * AUTHOR(S):    Laura Toma, Bowdoin College - ltoma@bowdoin.edu
 *               Yi Zhuang - yzhuang@bowdoin.edu

 *               Ported to GRASS by William Richard -
 *               wkrichar@bowdoin.edu or willster3021@gmail.com
 *               Markus Metz: surface interpolation
 *
 * Date:         april 2011 
 * 
 * PURPOSE: To calculate the viewshed (the visible cells in the
 * raster) for the given viewpoint (observer) location.  The
 * visibility model is the following: Two points in the raster are
 * considered visible to each other if the cells where they belong are
 * visible to each other.  Two cells are visible to each other if the
 * line-of-sight that connects their centers does not intersect the
 * terrain. The terrain is NOT viewed as a tesselation of flat cells, 
 * i.e. if the line-of-sight does not pass through the cell center, 
 * elevation is determined using bilinear interpolation.
 * The viewshed algorithm is efficient both in
 * terms of CPU operations and I/O operations. It has worst-case
 * complexity O(n lg n) in the RAM model and O(sort(n)) in the
 * I/O-model.  For the algorithm and all the other details see the
 * paper: "Computing Visibility on * Terrains in External Memory" by
 * Herman Haverkort, Laura Toma and Yi Zhuang.
 *
 * COPYRIGHT: (C) 2008 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 *****************************************************************************/

/* 
   A grid in ArcInfo Ascii Grid Format 
 */


#ifndef __GRID_H
#define __GRID_H

#include <stdio.h>
#include <limits.h>

extern "C"
{
#include <grass/gis.h>
#include <grass/raster.h>
}

#define G_SURFACE_TYPE FCELL_TYPE
typedef float surface_type;
typedef FCELL G_SURFACE_T;

/* this should accommodate grid sizes up to 2^16-1=65,535
   If this is not enough, change type and recompile */
typedef unsigned short int dimensionType;
static const dimensionType maxDimension = USHRT_MAX - 1;


typedef struct grid_header
{
    dimensionType ncols;	/*number of columns in the grid */
    dimensionType nrows;	/*number of rows in the grid */
    double xllcorner;		/*xllcorner refers to the western edge of grid */
    double yllcorner;		/*yllcorner refers to the southern edge of grid */
    double ew_res;		/*the ew resolution of the grid */
    double ns_res;		/*the ns resolution of the grid */
    surface_type nodata_value;		/*the value that represents missing data */

    struct Cell_head window;
} GridHeader;



typedef struct grid_
{
    GridHeader *hd;

    /*two dimensional array holding all the values in the grid */
    float **grid_data;

    float minvalue;		/*the minimum value in the grid */
    float maxvalue;		/*the maximum value in the grid */
} Grid;



/*copy from b to a */
void copy_header(GridHeader * a, GridHeader b);


/*returns 1 if value is Nodata, 0 if it is not */
int is_nodata(GridHeader * hd, float value);
int is_nodata(Grid * grid, float value);

/* create and return an empty grid */
Grid *create_empty_grid();

/*allocate memory for grid data, grid must have a header */
void alloc_grid_data(Grid * grid);

/*destroy the structure and reclaim all memory allocated */
void destroy_grid(Grid * grid);


#endif
