/*
* $Id$
*/

/*  gs_util.c
    Bill Brown, USACERL  
    January 1993
*/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include <grass/gstypes.h>

/************************************************************************/
double GS_geodistance(double *from, double *to, char *units)
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

/************************************************************************/
float GS_distance(float *from, float *to)
{
    float x, y, z;

    x = from[X] - to[X];
    y = from[Y] - to[Y];
    z = from[Z] - to[Z];

    return (float) sqrt(x * x + y * y + z * z);
}

/************************************************************************/
float GS_P2distance(float *from, float *to)
{
    float x, y;

    x = from[X] - to[X];
    y = from[Y] - to[Y];

    return (float) sqrt(x * x + y * y);
}

/************************************************************************/
/* v1 = v2 */
void GS_v3eq(float *v1, float *v2)
{
    v1[X] = v2[X];
    v1[Y] = v2[Y];
    v1[Z] = v2[Z];

    return;
}

/************************************************************************/
/* v1 += v2 */
void GS_v3add(float *v1, float *v2)
{
    v1[X] += v2[X];
    v1[Y] += v2[Y];
    v1[Z] += v2[Z];

    return;
}

/************************************************************************/
/* v1 -= v2 */
void GS_v3sub(float *v1, float *v2)
{
    v1[X] -= v2[X];
    v1[Y] -= v2[Y];
    v1[Z] -= v2[Z];

    return;
}

/************************************************************************/
/* v1 *= k */
void GS_v3mult(float *v1, float k)
{
    v1[X] *= k;
    v1[Y] *= k;
    v1[Z] *= k;

    return;
}

/************************************************************************/
/* Changes v1 so that it is a unit vector */
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

/************************************************************************/
/* Changes v1 so that it is a unit vector */
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

/************************************************************************/
/* Changes v1 so that it is a unit vector */
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


/************************************************************************/
/* Changes v2 so that v1v2 is a unit vector */
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


/************************************************************************/
/* get a normalized direction from v1 to v2, store in v3 */
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


/************************************************************************/
/* get a normalized direction from v1 to v2, store in v3 */
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

/************************************************************************/
/* return the cross product v3 = v1 cross v2 */
void GS_v3cross(float *v1, float *v2, float *v3)
{
    v3[X] = (v1[Y] * v2[Z]) - (v1[Z] * v2[Y]);
    v3[Y] = (v1[Z] * v2[X]) - (v1[X] * v2[Z]);
    v3[Z] = (v1[X] * v2[Y]) - (v1[Y] * v2[X]);

    return;
}

/************************************************************************/
/* magnitude of vector */
void GS_v3mag(float *v1, float *mag)
{
    *mag = sqrt(v1[X] * v1[X] + v1[Y] * v1[Y] + v1[Z] * v1[Z]);

    return;
}

/************************************************************************/
/* initialize by calling with a number nhist to represent number of
 * previous entrys to check, then call with zero as nhist
*/
int GS_coordpair_repeats(float *p1, float *p2, int nhist)
{
    static float *entrys = NULL;
    static int next = 0;
    static int len = 0;
    int i;

    if (nhist) {
	if (entrys) {
	    free(entrys);
	}

	if (NULL == (entrys = (float *) malloc(4 * nhist * sizeof(float)))) {
	    return (-1);
	}

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
