/*****************************************************************************
 *
 * MODULE:       Grass raster3d Library
 * AUTHOR(S):    Soeren Gebbert, Braunschweig (GER) Jun 2011
 *               soerengebbert <at> googlemail <dot> com
 *
 * PURPOSE:      Unit and Integration tests
 *
 * SPDX-FileCopyrightText: 2000 by the GRASS Development Team
 *
 * SPDX-License-Identifier: GPL-2.0-or-later.
 *
 *****************************************************************************/

#ifndef _TEST_RASTER3D_LIB_H_
#define _TEST_RASTER3D_LIB_H_

#include <grass/raster3d.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <time.h>
#endif
double compute_time_difference(struct timeval, struct timeval);
int unit_test_coordinate_transform(void);
int unit_test_put_get_value(void);
int unit_test_put_get_value_large_file(int, int, int, int);

#endif
