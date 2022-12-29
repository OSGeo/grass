/*!
 * \file lib/gis/rotate.c
 *
 * \brief GIS Library - rotate
 *
 * (C) 2001-2014 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public
 * License (>=v2). Read the file COPYING that comes with GRASS
 * for details.
 *
 * \author Hamish Bowman, Glynn Clements
 */

#include <math.h>

# define RpD ((2 * M_PI) / 360.)	/* radians/degree */
# define D2R(d) (double)(d * RpD)	/* degrees->radians */
# define R2D(d) (double)(d / RpD)	/* radians->degrees */



/*!
 * \brief Rotate point (double version)
 *
 * Given a point, angle, and origin, rotate the point around the origin 
 * by the given angle. Coordinates and results are double prec floating point.
 *
 * \param X0  X component of origin (center of circle)
 * \param Y0  Y component of origin (center of circle)
 * \param[out] X1  X component of point to be rotated (variable is modified!)
 * \param[out] Y1  Y component of point to be rotated (variable is modified!)
 * \param angle  in degrees, measured CCW from east
 */
void G_rotate_around_point(double X0, double Y0, double *X1, double *Y1,
			   double angle)
{
    double dx = *X1 - X0;
    double dy = *Y1 - Y0;
    double c = cos(D2R(angle));
    double s = sin(D2R(angle));
    double dx1 = dx * c - dy * s;
    double dy1 = dx * s + dy * c;

    *X1 = X0 + dx1;
    *Y1 = Y0 + dy1;
}

/*!
 * \brief Rotate point (int version)
 *
 * Given a point, angle, and origin, rotate the point around the origin 
 * by the given angle. Coordinates are given in integer and results are rounded
 * back to integer.
 *
 * \param X0  X component of origin (center of circle)
 * \param Y0  Y component of origin (center of circle)
 * \param[out] X1  X component of point to be rotated (variable is modified!)
 * \param[out] Y1  Y component of point to be rotated (variable is modified!)
 * \param angle  in degrees, measured CCW from east
 */
void G_rotate_around_point_int(int X0, int Y0, int *X1, int *Y1, double angle)
{
    double x = (double)*X1;
    double y = (double)*Y1;

    if (angle == 0.0)
	return;

    G_rotate_around_point((double)X0, (double)Y0, &x, &y, angle);

    *X1 = (int)floor(x + 0.5);
    *Y1 = (int)floor(y + 0.5);
}
