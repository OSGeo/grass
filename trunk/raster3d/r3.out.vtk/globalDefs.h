
/****************************************************************************
*
* MODULE:       r3.out.vtk  
*   	    	
* AUTHOR(S):    Original author 
*               Soeren Gebbert soerengebbert at gmx de
* 		27 Feb 2006 Berlin
* PURPOSE:      Converts 3D raster maps (RASTER3D) into the VTK-Ascii format  
*
* COPYRIGHT:    (C) 2005 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*   	    	License (>=v2). Read the file COPYING that comes with GRASS
*   	    	for details.
*
*****************************************************************************/
#ifndef __R3_OUT_VTK_GLOBALDEFS_H__
#define __R3_OUT_VTK_GLOBALDEFS_H__


typedef struct
{
    /*RASTER3D maps */
    void *map;
    void *map_r;
    void *map_g;
    void *map_b;
    void *map_x;
    void *map_y;
    void *map_z;
    /*raster maps */
    int top;
    int bottom;
    int *elevmaps;
    int *elevmaptypes;
    int numelevmaps;

    int topMapType;
    int bottomMapType;

} input_maps;

extern double x_extent;
extern double y_extent;

#endif
