/*!
   \file lib/pngdriver/point.c

   \brief GRASS png display driver - draw point

   SPDX-FileCopyrightText: 2007-2014 Glynn Clements
   SPDX-FileCopyrightText: Other GRASS authors
   SPDX-License-Identifier: GPL-2.0-or-later

   \author Glynn Clements
 */

#include <grass/gis.h>
#include "pngdriver.h"

/*!
   \brief Draw point
 */
void PNG_Point(double x, double y)
{
    static double point_size = 1.0;
    double half_point_size = point_size / 2;

    PNG_Box(x - half_point_size, y - half_point_size, point_size, point_size);
}
