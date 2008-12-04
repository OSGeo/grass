#include <grass/gis.h>
#include "pngdriver.h"

void PNG_Point(double x, double y)
{
    static double point_size = 1.0;
    double half_point_size = point_size / 2;

    PNG_Box(x - half_point_size, y - half_point_size,
	    point_size, point_size);
}

