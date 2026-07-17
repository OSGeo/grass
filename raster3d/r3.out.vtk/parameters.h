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

#ifndef __R_OUT_VTK_PARAMETERS_H__
#define __R_OUT_VTK_PARAMETERS_H__

/** Parameters and global variables ******************************************/
typedef struct {
    struct Option *input, *output, *rgbmaps, *vectormaps, *null_val, *top,
        *bottom, *decimals, *elevscale;
    struct Flag *mask, *point, *origin, *structgrid, *coorcorr, *scalell;
    /*struct Flag *xml; */ /*maybe xml support in the future */
} paramType;

/*global structs */
extern paramType param; /*Parameters */

/*prototype */
void set_params(void);

#endif
