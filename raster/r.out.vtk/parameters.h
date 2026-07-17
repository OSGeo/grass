/****************************************************************************
 *
 * MODULE:       r.out.vtk
 *
 * AUTHOR(S):    Original author
 *               Soeren Gebbert soerengebbert@gmx.de
 *                 08 23 2005 Berlin
 * PURPOSE:      Converts raster maps into the VTK-Ascii format
 *
 * SPDX-FileCopyrightText: 2005 Other GRASS authors
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 *****************************************************************************/

#ifndef __R_OUT_VTK_PARAMETERS_H__
#define __R_OUT_VTK_PARAMETERS_H__
struct Option;
struct Flag;

typedef struct {
    struct Option *input, *output, *elevationmap, *null_val, *elevscale, *elev,
        *rgbmaps, *vectmaps, *decimals;
    struct Flag *usestruct, *usetriangle, *usevertices, *origin, *point,
        *coorcorr;
    /*struct Flag *mask;          struct Flag *xml; */ /*maybe xml support
                                                          in the future */
} paramType;

/*global structs */

extern paramType param; /*Parameters */

/*prototype */
void set_params(void);

#endif
