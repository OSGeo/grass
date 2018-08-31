/*!
   \file integrate.c

   \brief Flowline integration using Runge-Kutta 45 method
   with adaptive step size control. Implementation based
   on VTK class vtkRungeKutta45.

   (C) 2014 by the GRASS Development Team

   This program is free software under the GNU General Public
   License (>=v2).  Read the file COPYING that comes with GRASS
   for details.

   \author Anna Petrasova
 */

#include <math.h>
#include <float.h>
#include <string.h>
#include <grass/raster3d.h>

#include "interpolate.h"
#include "integrate.h"
#include "r3flow_structs.h"

/*!
   \brief Computes vector norm.

   \param x,y,z vector components

   \return norm
 */
double norm(const double x, const double y, const double z)
{
    return sqrt(x * x + y * y + z * z);
}

/*!
   \brief Converts integration step to time

   \param unit type of unit
   \param step step value
   \param velocity velocity value
   \param cell_size size of voxel diagonal

   \return time step
 */
double get_time_step(const char *unit, const double step,
		     const double velocity, const double cell_size)
{
    if (strcmp(unit, "time") == 0)
	return step;
    else if (strcmp(unit, "length") == 0)
	return step / velocity;
    else			/* cell */
	return (step * cell_size) / velocity;
}


int get_velocity(RASTER3D_Region * region, struct Gradient_info *gradient_info,
		 const double x, const double y, const double z,
		 double *vel_x, double *vel_y, double *vel_z)
{
    if (gradient_info->compute_gradient)
        return get_gradient(region, gradient_info, y, x, z, vel_x, vel_y, vel_z);

    return interpolate_velocity(region, gradient_info->velocity_maps, y, x, z,
				vel_x, vel_y, vel_z);
}

/*!
   \brief Runge-Kutta45 (inner steps) 

   \param region pointer to current 3D region
   \param velocity_field pointer to array of 3 3D raster maps (velocity field)
   \param point pointer to array of geographic coordinates of starting point
   \param[out] next_point pointer to array of geographic coordinates of integrated point
   \param delta_t integration time step
   \param error pointer to array of error values

   \return 0 success
   \return -1 out of region or null values
 */
static int rk45_next(RASTER3D_Region * region, struct Gradient_info *gradient_info,
		     const double *point, double *next_point,
		     const double delta_t, double *velocity, double *error)
{
    double tmp1[6][3];		/* 3 is 3 dimensions, 6 is the number of k's */
    double tmp_point[3];
    double vel_x, vel_y, vel_z;
    double sum_tmp;
    int i, j, k;
    double vel_sq;

    if (get_velocity(region, gradient_info, point[0], point[1], point[2],
		     &vel_x, &vel_y, &vel_z) < 0)
	return -1;

    tmp1[0][0] = vel_x;
    tmp1[0][1] = vel_y;
    tmp1[0][2] = vel_z;

    /* compute k's */
    for (i = 1; i < 6; i++) {
	for (j = 0; j < 3; j++) {	/* for each coordinate */
	    sum_tmp = 0;
	    for (k = 0; k < i; k++) {	/* k1; k1, k2; ... */
		sum_tmp += B[i - 1][k] * tmp1[k][j];
	    }
	    tmp_point[j] = point[j] + delta_t * sum_tmp;
	}
	if (get_velocity
	    (region, gradient_info, tmp_point[0], tmp_point[1], tmp_point[2],
	     &vel_x, &vel_y, &vel_z) < 0)
	    return -1;

	tmp1[i][0] = vel_x;
	tmp1[i][1] = vel_y;
	tmp1[i][2] = vel_z;
    }

    vel_sq = 0;
    /* compute next point */
    for (j = 0; j < 3; j++) {
	sum_tmp = 0;
	for (i = 0; i < 6; i++) {
	    sum_tmp += C[i] * tmp1[i][j];
	}
	next_point[j] = point[j] + delta_t * sum_tmp;
	vel_sq += sum_tmp * sum_tmp;
    }
    *velocity = sqrt(vel_sq);

    if (!Rast3d_is_valid_location
	(region, next_point[1], next_point[0], next_point[2]))
	return -1;

    /* compute error vector */
    for (j = 0; j < 3; j++) {
	sum_tmp = 0;
	for (i = 0; i < 6; i++) {
	    sum_tmp += DC[i] * tmp1[i][j];
	}
	error[j] = delta_t * sum_tmp;
    }

    return 0;
}

/*!
   \brief Runge-Kutta45 with step size control 

   \param region pointer to current 3D region
   \param velocity_field pointer to array of 3 3D raster maps (velocity field)
   \param point pointer to array of geographic coordinates of starting point
   \param[out] next_point pointer to array of geographic coordinates of integrated point
   \param[in,out] delta_t integration time step
   \param min_step minimum time step
   \param max_step maximum time step

   \return 0 success
   \return -1 out of region or null values
 */
int rk45_integrate_next(RASTER3D_Region * region,
			struct Gradient_info *gradient_info, const double *point,
			double *next_point, double *delta_t, double *velocity,
			const double min_step, const double max_step,
			const double max_error)
{
    double estimated_error;
    double error_ratio;
    double error[3];
    double tmp, tmp2;
    int do_break;

    estimated_error = DBL_MAX;

    /* check if min_step < delta_t < max_step */
    if (fabs(*delta_t) < min_step)
	*delta_t = min_step * (*delta_t > 0 ? 1 : -1);
    else if (fabs(*delta_t) > max_step)
	*delta_t = max_step * (*delta_t > 0 ? 1 : -1);

    /* try to iteratively decrease error to less than max error */
    while (estimated_error > max_error) {
	/* compute next point and get estimated error */
	if (rk45_next
	    (region, gradient_info, point, next_point, *delta_t,
	     velocity, error) == 0)
	    estimated_error = norm(error[0], error[1], error[2]);
	else
	    return -1;

	/* compute new step size (empirically) */
	error_ratio = estimated_error / max_error;
	if (error_ratio == 0.0)
	    tmp = *delta_t > 0 ? min_step : -min_step;
	else if (error_ratio > 1)
	    tmp = 0.9 * *delta_t * pow(error_ratio, -0.25);
	else
	    tmp = 0.9 * *delta_t * pow(error_ratio, -0.2);
	tmp2 = fabs(tmp);

	do_break = FALSE;

	/* adjust new step size to be within min max limits */
	if (tmp2 > max_step) {
	    *delta_t = max_step * (*delta_t > 0 ? 1 : -1);
	    do_break = TRUE;
	}
	else if (tmp2 < min_step) {
	    *delta_t = min_step * (*delta_t > 0 ? 1 : -1);
	    do_break = TRUE;
	}
	else
	    *delta_t = tmp;

	/* break when the adjustment was needed (not sure why) */
	if (do_break) {
	    if (rk45_next(region, gradient_info, point, next_point,
			  *delta_t, velocity, error) < 0)
		return -1;
	    break;
	}
    }
    return 0;
}
