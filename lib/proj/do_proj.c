/**
   \file do_proj.c

   \brief GProj library - Functions for re-projecting point data

   \author Original Author unknown, probably Soil Conservation Service
   Eric Miller, Paul Kelly

   (C) 2003-2008 by the GRASS Development Team

   This program is free software under the GNU General Public
   License (>=v2). Read the file COPYING that comes with GRASS
   for details.
**/

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <grass/gis.h>
#include <grass/gprojects.h>
#include <grass/glocale.h>

/* a couple defines to simplify reading the function */
#define MULTIPLY_LOOP(x,y,c,m) \
do {\
   int i; \
   for (i = 0; i < c; ++i) {\
       x[i] *= m; \
       y[i] *= m; \
   }\
} while (0)

#define DIVIDE_LOOP(x,y,c,m) \
do {\
   int i; \
   for (i = 0; i < c; ++i) {\
       x[i] /= m; \
       y[i] /= m; \
   }\
} while (0)

static double METERS_in = 1.0, METERS_out = 1.0;

/** 
 * \brief Re-project a point between two co-ordinate systems
 * 
 * This function takes pointers to two pj_info structures as arguments, 
 * and projects a point between the co-ordinate systems represented by them. 
 * The easting and northing of the point are contained in two pointers passed 
 * to the function; these will be overwritten by the co-ordinates of the 
 * re-projected point.
 * 
 * \param x Pointer to a double containing easting or longitude
 * \param y Pointer to a double containing northing or latitude
 * \param info_in pointer to pj_info struct for input co-ordinate system
 * \param info_out pointer to pj_info struct for output co-ordinate system
 * 
 * \return Return value from PROJ pj_transform() function
 **/

int pj_do_proj(double *x, double *y,
	       struct pj_info *info_in, struct pj_info *info_out)
{
    int ok;
    double u, v;
    double h = 0.0;

    METERS_in = info_in->meters;
    METERS_out = info_out->meters;

    if (strncmp(info_in->proj, "ll", 2) == 0) {
	if (strncmp(info_out->proj, "ll", 2) == 0) {
	    u = (*x) / RAD_TO_DEG;
	    v = (*y) / RAD_TO_DEG;
	    ok = pj_transform(info_in->pj, info_out->pj, 1, 0, &u, &v, &h);
	    *x = u * RAD_TO_DEG;
	    *y = v * RAD_TO_DEG;
	}
	else {
	    u = (*x) / RAD_TO_DEG;
	    v = (*y) / RAD_TO_DEG;
	    ok = pj_transform(info_in->pj, info_out->pj, 1, 0, &u, &v, &h);
	    *x = u / METERS_out;
	    *y = v / METERS_out;
	}
    }
    else {
	if (strncmp(info_out->proj, "ll", 2) == 0) {
	    u = *x * METERS_in;
	    v = *y * METERS_in;
	    ok = pj_transform(info_in->pj, info_out->pj, 1, 0, &u, &v, &h);
	    *x = u * RAD_TO_DEG;
	    *y = v * RAD_TO_DEG;
	}
	else {
	    u = *x * METERS_in;
	    v = *y * METERS_in;
	    ok = pj_transform(info_in->pj, info_out->pj, 1, 0, &u, &v, &h);
	    *x = u / METERS_out;
	    *y = v / METERS_out;
	}
    }
    if (ok < 0) {
	G_warning (_("pj_transform() failed: %s"), pj_strerrno(ok));
    }
    return ok;
}

/** 
 * \brief Re-project an array of points between two co-ordinate systems with
 *        optional ellipsoidal height conversion
 * 
 * This function takes pointers to two pj_info structures as arguments, 
 * and projects an array of points between the co-ordinate systems 
 * represented by them. Pointers to the three arrays of easting, northing,
 * and ellipsoidal height of the point (this one may be NULL) are passed
 * to the function; these will be overwritten by the co-ordinates of the 
 * re-projected points.
 * 
 * \param count Number of points in the arrays to be transformed
 * \param x Pointer to an array of type double containing easting or longitude
 * \param y Pointer to an array of type double containing northing or latitude
 * \param h Pointer to an array of type double containing ellipsoidal height. 
 *          May be null in which case a two-dimensional re-projection will be 
 *          done
 * \param info_in pointer to pj_info struct for input co-ordinate system
 * \param info_out pointer to pj_info struct for output co-ordinate system
 * 
 * \return Return value from PROJ pj_transform() function
 **/

int pj_do_transform(int count, double *x, double *y, double *h,
		    struct pj_info *info_in, struct pj_info *info_out)
{
    int ok;
    int has_h = 1;

    METERS_in = info_in->meters;
    METERS_out = info_out->meters;

    if (h == NULL) {
	int i;
	h = G_malloc(sizeof *h * count);
	/* they say memset is only guaranteed for chars ;-( */
	for (i = 0; i < count; ++i)
	    h[i] = 0.0;
	has_h = 0;
    }
    if (strncmp(info_in->proj, "ll", 2) == 0) {
	if (strncmp(info_out->proj, "ll", 2) == 0) {
	    DIVIDE_LOOP(x, y, count, RAD_TO_DEG);
	    ok = pj_transform(info_in->pj, info_out->pj, count, 1, x, y, h);
	    MULTIPLY_LOOP(x, y, count, RAD_TO_DEG);
	}
	else {
	    DIVIDE_LOOP(x, y, count, RAD_TO_DEG);
	    ok = pj_transform(info_in->pj, info_out->pj, count, 1, x, y, h);
	    DIVIDE_LOOP(x, y, count, METERS_out);
	}
    }
    else {
	if (strncmp(info_out->proj, "ll", 2) == 0) {
	    MULTIPLY_LOOP(x, y, count, METERS_in);
	    ok = pj_transform(info_in->pj, info_out->pj, count, 1, x, y, h);
	    MULTIPLY_LOOP(x, y, count, RAD_TO_DEG);
	}
	else {
	    MULTIPLY_LOOP(x, y, count, METERS_in);
	    ok = pj_transform(info_in->pj, info_out->pj, count, 1, x, y, h);
	    DIVIDE_LOOP(x, y, count, METERS_out);
	}
    }
    if (!has_h)
	G_free(h);

    if (ok < 0) {
	G_warning(_("pj_transform() failed: %s"), pj_strerrno(ok));
    }
    return ok;
}
