/****************************************************************************
 *
 * MODULE:       r.viewshed
 *
 * AUTHOR(S):    Laura Toma, Bowdoin College - ltoma@bowdoin.edu
 *               Yi Zhuang - yzhuang@bowdoin.edu

 *               Ported to GRASS by William Richard -
 *               wkrichar@bowdoin.edu or willster3021@gmail.com
 *
 * Date:         july 2008 
 * 
 * PURPOSE: To calculate the viewshed (the visible cells in the
 * raster) for the given viewpoint (observer) location.  The
 * visibility model is the following: Two points in the raster are
 * considered visible to each other if the cells where they belong are
 * visible to each other.  Two cells are visible to each other if the
 * line-of-sight that connects their centers does not intersect the
 * terrain. The height of a cell is assumed to be constant, and the
 * terrain is viewed as a tesselation of flat cells.  This model is
 * suitable for high resolution rasters; it may not be accurate for
 * low resolution rasters, where it may be better to interpolate the
 * height at a point based on the neighbors, rather than assuming
 * cells are "flat".  The viewshed algorithm is efficient both in
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



/* this should accomodate grid sizes up to 2^16-1=65,535
   If this is not enough, change type and recompile */
typedef unsigned short int dimensionType;
static const dimensionType maxDimension = USHRT_MAX - 1;


typedef struct grid_header {
    dimensionType ncols;  /*number of columns in the grid */
    dimensionType nrows;  /*number of rows in the grid */
    float xllcorner;	  /*xllcorner refers to the western edge of grid */
    float yllcorner;	  /*yllcorner refers to the southern edge of grid */
    float cellsize;		  /*the resolution of the grid */
    float nodata_value;   /*the value that represents missing data */
} GridHeader;



typedef struct grid_ {
    GridHeader *hd;

    /*two dimensional array holding all the values in the grid */
    float **grid_data;

    float minvalue;		/*the minimum value in the grid */
    float maxvalue;		/*the maximum value in the grid */
} Grid;




/* create and return the header of the grid stored in this file;*/
GridHeader *read_header_from_arcascii_file(char* fname);

/* create and return the header of the grid stored in this file;*/
GridHeader *read_header_from_arcascii_file(FILE* fp);

/*copy from b to a */
void copy_header(GridHeader * a, GridHeader b);


/*print header */
void print_grid_header(GridHeader * hd);
void print_grid_header(FILE * fp, GridHeader * hd);


/*returns 1 if value is Nodata, 0 if it is not */
int is_nodata(GridHeader * hd, float value);
int is_nodata(Grid * grid, float value);

/* create and return an empty grid */
Grid *create_empty_grid();

/*allocate memory for grid data, grid must have a header */
void alloc_grid_data(Grid * grid);

/*scan an arcascii file and fill the information in the given structure */
Grid *read_grid_from_arcascii_file(char *filename);

/*destroy the structure and reclaim all memory allocated */
void destroy_grid(Grid * grid);

/*save grid into an arcascii file; Loops through all elements x in
  row-column order and writes fun(x) to file */
void save_grid_to_arcascii_file(Grid * grid, char *filename, 
								float (*fun) (float));



#endif
