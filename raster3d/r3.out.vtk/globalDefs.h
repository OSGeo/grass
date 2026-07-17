/****************************************************************************
 *
 * MODULE:       r3.out.vtk
 *
 * AUTHOR(S):    Original author
 *               Soeren Gebbert soerengebbert at gmx de
 *                 27 Feb 2006 Berlin
 * PURPOSE:      Converts 3D raster maps (RASTER3D) into the VTK-Ascii format
 *
 * SPDX-FileCopyrightText: 2005 Other GRASS authors
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 *****************************************************************************/

#ifndef __R3_OUT_VTK_GLOBALDEFS_H__
#define __R3_OUT_VTK_GLOBALDEFS_H__

#include <grass/raster3d.h>

typedef struct {
    /*RASTER3D maps */
    RASTER3D_Map *map;
    RASTER3D_Map *map_r;
    RASTER3D_Map *map_g;
    RASTER3D_Map *map_b;
    RASTER3D_Map *map_x;
    RASTER3D_Map *map_y;
    RASTER3D_Map *map_z;
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
