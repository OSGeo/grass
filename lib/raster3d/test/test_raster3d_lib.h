
/*****************************************************************************
*
* MODULE:       Grass raster3d Library
* AUTHOR(S):    Soeren Gebbert, Braunschweig (GER) Jun 2011
*               soerengebbert <at> googlemail <dot> com
*               
* PURPOSE:	Unit and Integration tests
*
* COPYRIGHT:    (C) 2000 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*               License (>=v2). Read the file COPYING that comes with GRASS
*               for details.
*
*****************************************************************************/

#ifndef _TEST_RASTER3D_LIB_H_
#define _TEST_RASTER3D_LIB_H_

#include <grass/raster3d.h>
#include <grass/gis.h>
#include <grass/glocale.h>

double compute_time_difference(struct timeval start, struct timeval end);
int unit_test_coordinate_transform(void);
int unit_test_put_get_value(void);
int unit_test_put_get_value_large_file(int depths, int rows, int cols, int tile_size);

#endif
