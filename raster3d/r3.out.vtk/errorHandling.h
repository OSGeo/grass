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

#ifndef __R3_OUT_VTK_ERROR_HANDLING_H__
#define __R3_OUT_VTK_ERROR_HANDLING_H__

struct input_maps;

/*Simple Error message */
void fatal_error(char *errorMsg, input_maps *in);

/*Free the input maps structure und close all open maps */
void release_input_maps_struct(input_maps *in);

#endif
