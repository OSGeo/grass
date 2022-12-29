/*!
   \file lib/ogsf/gs_util.c

   \brief OGSF library - loading and manipulating surfaces

   GRASS OpenGL gsurf OGSF Library 

   (C) 1999-2008 by the GRASS Development Team

   This program is free software under the 
   GNU General Public License (>=v2). 
   Read the file COPYING that comes with GRASS
   for details.

   \author Bill Brown USACERL, GMSL/University of Illinois
   \author Doxygenized by Martin Landa <landa.martin gmail.com> (May 2008)
 */

#include <stdlib.h>
#include <math.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/ogsf.h>

/*!
   \brief Calculate distance between 2 coordinates

   Units is one of:
   - "meters",
   - "miles",
   - "kilometers",
   - "feet",
   - "yards",
   - "nmiles" (nautical miles),
   - "rods",
   - "inches",
   - "centimeters",
   - "millimeters",
   - "micron",
   - "nanometers",
   - "cubits",
   - "hands",
   - "furlongs",
   - "chains"

   Default is meters.

   \param from starting point
   \param to ending point
   \param units map units

   \return distance between two geographic coordinates in current projection
 */
double GS_geodistance(double *from, double *to, const char *units)
{
    double meters;

    meters = Gs_distance(from, to);

    if (!units) {
	return (meters);
    }

    if (strcmp(units, "meters") == 0) {
	return (meters);
    }

    if (strcmp(units, "miles") == 0) {
	return (meters * .0006213712);
    }

    if (strcmp(units, "kilometers") == 0) {
	return (meters * .001);
    }

    if (strcmp(units, "feet") == 0) {
	return (meters * 3.280840);
    }

    if (strcmp(units, "yards") == 0) {
	return (meters * 1.093613);
    }

    if (strcmp(units, "rods") == 0) {
	return (meters * .1988388);
    }

    if (strcmp(units, "inches") == 0) {
	return (meters * 39.37008);
    }

    if (strcmp(units, "centimeters") == 0) {
	return (meters * 100.0);
    }

    if (strcmp(units, "millimeters") == 0) {
	return (meters * 1000.0);
    }

    if (strcmp(units, "micron") == 0) {
	return (meters * 1000000.0);
    }

    if (strcmp(units, "nanometers") == 0) {
	return (meters * 1000000000.0);
    }

    if (strcmp(units, "cubits") == 0) {
	return (meters * 2.187227);
    }

    if (strcmp(units, "hands") == 0) {
	return (meters * 9.842520);
    }

    if (strcmp(units, "furlongs") == 0) {
	return (meters * .004970970);
    }

    if (strcmp(units, "nmiles") == 0) {
	/* nautical miles */
	return (meters * .0005399568);
    }

    if (strcmp(units, "chains") == 0) {
	return (meters * .0497097);
    }

    return (meters);
}

/*!
   \brief Calculate distance

   \param from 'from' point (X,Y,Z)
   \param to 'to' point (X,Y,Z)

   \return distance
 */
float GS_distance(float *from, float *to)
{
    float x, y, z;

    x = from[X] - to[X];
    y = from[Y] - to[Y];
    z = from[Z] - to[Z];

    return (float)sqrt(x * x + y * y + z * z);
}

/*!
   \brief Calculate distance in plane

   \param from 'from' point (X,Y)
   \param to 'to' point (X,Y)

   \return distance
 */
float GS_P2distance(float *from, float *to)
{
    float x, y;

    x = from[X] - to[X];
    y = from[Y] - to[Y];

    return (float)sqrt(x * x + y * y);
}

/*!
   \brief Copy vector values

   v1 = v2

   \param[out] v1 first vector
   \param v2 second vector
 */
void GS_v3eq(float *v1, float *v2)
{
    v1[X] = v2[X];
    v1[Y] = v2[Y];
    v1[Z] = v2[Z];

    return;
}

/*!
   \brief Sum vectors

   v1 += v2

   \param[in,out] v1 first vector
   \param v2 second vector
 */
void GS_v3add(float *v1, float *v2)
{
    v1[X] += v2[X];
    v1[Y] += v2[Y];
    v1[Z] += v2[Z];

    return;
}

/*!
   \brief Subtract vectors

   v1 -= v2

   \param[in,out] v1 first vector
   \param v2 second vector
 */
void GS_v3sub(float *v1, float *v2)
{
    v1[X] -= v2[X];
    v1[Y] -= v2[Y];
    v1[Z] -= v2[Z];

    return;
}

/*!
   \brief Multiple vectors

   v1 *= k

   \param[in,out] v1 vector
   \param k multiplicator
 */
void GS_v3mult(float *v1, float k)
{
    v1[X] *= k;
    v1[Y] *= k;
    v1[Z] *= k;

    return;
}

/*!
   \brief Change v1 so that it is a unit vector (2D)

   \param[in,out] v1 vector

   \return 0 if magnitude of v1 is zero
   \return 1 if magnitude of v1 > 0
 */
int GS_v3norm(float *v1)
{
    float n;

    n = sqrt(v1[X] * v1[X] + v1[Y] * v1[Y] + v1[Z] * v1[Z]);

    if (n == 0.0) {
	return (0);
    }

    v1[X] /= n;
    v1[Y] /= n;
    v1[Z] /= n;

    return (1);
}

/*!
   \brief Change v1 so that it is a unit vector (3D)

   \param[in,out] v1 vector

   \return 0 if magnitude of v1 is zero
   \return 1 if magnitude of v1 > 0
 */
int GS_v2norm(float *v1)
{
    float n;

    n = sqrt(v1[X] * v1[X] + v1[Y] * v1[Y]);

    if (n == 0.0) {
	return (0);
    }

    v1[X] /= n;
    v1[Y] /= n;

    return (1);
}

/*!
   \brief Changes v1 so that it is a unit vector

   \param dv1 vector

   \return 0 if magnitude of dv1 is zero
   \return 1 if magnitude of dv1 > 0
 */
int GS_dv3norm(double *dv1)
{
    double n;

    n = sqrt(dv1[X] * dv1[X] + dv1[Y] * dv1[Y] + dv1[Z] * dv1[Z]);

    if (n == 0.0) {
	return (0);
    }

    dv1[X] /= n;
    dv1[Y] /= n;
    dv1[Z] /= n;

    return (1);
}


/*!
   \brief Change v2 so that v1v2 is a unit vector

   \param v1 first vector
   \param v2[in,out] second vector

   \return 0 if magnitude of dx is zero
   \return 1 if magnitude of dx > 0
 */
int GS_v3normalize(float *v1, float *v2)
{
    float n, dx, dy, dz;

    dx = v2[X] - v1[X];
    dy = v2[Y] - v1[Y];
    dz = v2[Z] - v1[Z];
    n = sqrt(dx * dx + dy * dy + dz * dz);

    if (n == 0.0) {
	return (0);
    }

    v2[X] = v1[X] + dx / n;
    v2[Y] = v1[Y] + dy / n;
    v2[Z] = v1[Z] + dz / n;

    return (1);
}


/*!
   \brief Get a normalized direction from v1 to v2, store in v3

   \param v1 first vector
   \param v2 second vector
   \param[out] v3 output vector

   \return 0 if magnitude of dx is zero
   \return 1 if magnitude of dx > 0
 */
int GS_v3dir(float *v1, float *v2, float *v3)
{
    float n, dx, dy, dz;

    dx = v2[X] - v1[X];
    dy = v2[Y] - v1[Y];
    dz = v2[Z] - v1[Z];
    n = sqrt(dx * dx + dy * dy + dz * dz);

    if (n == 0.0) {
	v3[X] = v3[Y] = v3[Z] = 0.0;
	return (0);
    }

    v3[X] = dx / n;
    v3[Y] = dy / n;
    v3[Z] = dz / n;

    return (1);
}


/*!
   \brief Get a normalized direction from v1 to v2, store in v3 (2D)

   \param v1 first vector
   \param v2 second vector
   \param[out] v3 output vector

   \return 0 if magnitude of dx is zero
   \return 1 if magnitude of dx > 0
 */
void GS_v2dir(float *v1, float *v2, float *v3)
{
    float n, dx, dy;

    dx = v2[X] - v1[X];
    dy = v2[Y] - v1[Y];
    n = sqrt(dx * dx + dy * dy);

    v3[X] = dx / n;
    v3[Y] = dy / n;

    return;
}

/*!
   \brief Get the cross product v3 = v1 cross v2

   \param v1 first vector
   \param v2 second vector
   \param[out] v3 output vector
 */
void GS_v3cross(float *v1, float *v2, float *v3)
{
    v3[X] = (v1[Y] * v2[Z]) - (v1[Z] * v2[Y]);
    v3[Y] = (v1[Z] * v2[X]) - (v1[X] * v2[Z]);
    v3[Z] = (v1[X] * v2[Y]) - (v1[Y] * v2[X]);

    return;
}

/*!
   \brief Magnitude of vector

   \param v1 vector
   \param[out] mag magnitude value
 */
void GS_v3mag(float *v1, float *mag)
{
    *mag = sqrt(v1[X] * v1[X] + v1[Y] * v1[Y] + v1[Z] * v1[Z]);

    return;
}

/*!
   \brief ADD

   Initialize by calling with a number nhist to represent number of
   previous entrys to check, then call with zero as nhist

   \param p1 first point
   \param p2 second point
   \param nhist ?

   \return -1 on error
   \return -2
   \return 1
   \return 9
 */
int GS_coordpair_repeats(float *p1, float *p2, int nhist)
{
    static float *entrys = NULL;
    static int next = 0;
    static int len = 0;
    int i;

    if (nhist) {
	if (entrys) {
	    G_free(entrys);
	}

	entrys = (float *)G_malloc(4 * nhist * sizeof(float));

	if (!entrys)
	    return (-1);

	len = nhist;
	next = 0;
    }

    if (!len) {
	return (-2);
    }

    for (i = 0; i < next; i += 4) {
	if (entrys[i] == p1[0] && entrys[i + 1] == p1[1]
	    && entrys[i + 2] == p2[0] && entrys[i + 3] == p2[1]) {
	    return (1);
	}
    }

    if (len == next / 4) {
	next = 0;
    }

    entrys[next] = p1[0];
    entrys[next + 1] = p1[1];
    entrys[next + 2] = p2[0];
    entrys[next + 3] = p2[1];
    next += 4;

    return (0);
}
