/*!
  \file lib/pngdriver/point.c

  \brief GRASS png display driver - draw point

  (C) 2007-2014 by Glynn Clements and the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
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

    PNG_Box(x - half_point_size, y - half_point_size,
	    point_size, point_size);
}

