
/****************************************************************************
 *
 * MODULE:       r3.colors
 *
 * AUTHOR(S):    Michael Shapiro - CERL
 *               David Johnson
 *               Support for 3D rasters by Soeren Gebbert
 *
 * PURPOSE:      Allows creation and/or modification of the color table
 *               for a raster map layer.
 *
 * COPYRIGHT:    (C) 2006, 2011 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 ***************************************************************************/

#include "local_proto.h"

/* This is the main function for r3.colors*/
int main(int argc, char **argv)
{
    return edit_colors(argc, argv, RASTER3D_TYPE, "raster3d", "Raster3d");
}
