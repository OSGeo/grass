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
 * SPDX-FileCopyrightText: 2006, 2011 Other GRASS authors
 * SPDX-License-Identifier: GPL-2.0-or-later
 ***************************************************************************/

#include "local_proto.h"

/* This is the main function for r3.colors */
int main(int argc, char **argv)
{
    return edit_colors(argc, argv, RASTER3D_TYPE, "raster3d", "Raster3d");
}

