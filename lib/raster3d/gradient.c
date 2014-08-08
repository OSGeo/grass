
/*!
   \file gradient.c

   \brief Gradient computation

   (C) 2014 by the GRASS Development Team

   This program is free software under the GNU General Public
   License (>=v2).  Read the file COPYING that comes with GRASS
   for details.

   \author Anna Petrasova
 */

/*!
   \brief Gradient computation

   Gradient computation (second order approximation)
   using central differencing scheme (plus forward and backward
   difference of second order approx.)
   
   \param array pointer to RASTER3D_Array with input values
   \param step array of x, y, z steps for gradient (resolution values)
   \param[out] grad_x pointer to RASTER3D_Array_double with gradient in x direction
   \param[out] grad_y pointer to RASTER3D_Array_double with gradient in y direction
   \param[out] grad_z pointer to RASTER3D_Array_double with gradient in z direction

 */
#include <grass/raster3d.h>

void Rast3d_gradient_double(RASTER3D_Array_double *array, double *step,
			    RASTER3D_Array_double *grad_x,
			    RASTER3D_Array_double *grad_y,
			    RASTER3D_Array_double *grad_z)
{
    int col, row, depth;
    double val;

    for (depth = 0; depth < array->sz; depth++) {
	for (row = 0; row < array->sy; row++) {
	    /* row start */
	    val = RASTER3D_ARRAY_ACCESS(array, 0, row, depth);
	    if (Rast_is_d_null_value(&val))
		Rast_set_null_value(&RASTER3D_ARRAY_ACCESS(grad_x, 0, row, depth),
				    1, DCELL_TYPE);
	    else
		RASTER3D_ARRAY_ACCESS(grad_x, 0, row, depth) =
			(-3 * val + 4 * RASTER3D_ARRAY_ACCESS(array, 1, row, depth) -
			 RASTER3D_ARRAY_ACCESS(array, 2, row, depth)) / (2 * step[0]);

	    /* row end */
	    val = RASTER3D_ARRAY_ACCESS(array, array->sx - 1, row, depth);
	    if (Rast_is_d_null_value(&val))
		Rast_set_null_value(
			&RASTER3D_ARRAY_ACCESS(grad_x, array->sx - 1, row, depth),
			1, DCELL_TYPE);
	    else
		RASTER3D_ARRAY_ACCESS(grad_x, array->sx - 1, row, depth) =
			(3 * val - 4 * RASTER3D_ARRAY_ACCESS(array, array->sx - 2, row, depth) +
			 RASTER3D_ARRAY_ACCESS(array, array->sx - 3, row, depth)) / (2 * step[0]);

	    /* row */
	    for (col = 1; col < array->sx - 1; col++) {
		val = RASTER3D_ARRAY_ACCESS(array, col, row, depth);
		if (Rast_is_d_null_value(&val))
		    Rast_set_null_value(
			    &RASTER3D_ARRAY_ACCESS(grad_x, col, row, depth),
			    1, DCELL_TYPE);
		else
		    RASTER3D_ARRAY_ACCESS(grad_x, col, row, depth) =
			    (RASTER3D_ARRAY_ACCESS(array, col + 1, row, depth) -
			     RASTER3D_ARRAY_ACCESS(array, col - 1, row, depth)) / (2 * step[0]);
	    }
	}
    }
    for (depth = 0; depth < array->sz; depth++) {
	for (col = 0; col < array->sx; col++) {
	    /* col start */
	    val = RASTER3D_ARRAY_ACCESS(array, col, 0, depth);
	    if (Rast_is_d_null_value(&val))
		Rast_set_null_value(&RASTER3D_ARRAY_ACCESS(grad_y, col, 0, depth),
				    1, DCELL_TYPE);
	    else
		RASTER3D_ARRAY_ACCESS(grad_y, col, 0, depth) =
			-(-3 * val + 4 * RASTER3D_ARRAY_ACCESS(array, col, 1, depth) -
			  RASTER3D_ARRAY_ACCESS(array, col, 2, depth)) / (2 * step[1]);

	    /* col end */
	    val = RASTER3D_ARRAY_ACCESS(array, col, array->sy - 1, depth);
	    if (Rast_is_d_null_value(&val))
		Rast_set_null_value(
			&RASTER3D_ARRAY_ACCESS(grad_y, col, array->sy - 1, depth),
			1, DCELL_TYPE);
	    else
		RASTER3D_ARRAY_ACCESS(grad_y, col, array->sy - 1, depth) =
			-(3 * val - 4 * RASTER3D_ARRAY_ACCESS(array, col, array->sy - 2, depth) +
			  RASTER3D_ARRAY_ACCESS(array, col, array->sy - 3, depth)) / (2 * step[1]);

	    /* col */
	    for (row = 1; row < array->sy - 1; row++) {
		val = RASTER3D_ARRAY_ACCESS(array, col, row, depth);
		if (Rast_is_d_null_value(&val))
		    Rast_set_null_value(
			    &RASTER3D_ARRAY_ACCESS(grad_y, col, row, depth),
			    1, DCELL_TYPE);
		else
		    RASTER3D_ARRAY_ACCESS(grad_y, col, row, depth) =
			    -(RASTER3D_ARRAY_ACCESS(array, col, row + 1, depth) -
			      RASTER3D_ARRAY_ACCESS(array, col, row - 1, depth)) / (2 * step[1]);
	    }
	}
    }
    for (row = 0; row < array->sy; row++) {
	for (col = 0; col < array->sx; col++) {
	    /* vertical col start */
	    val = RASTER3D_ARRAY_ACCESS(array, col, row, 0);
	    if (Rast_is_d_null_value(&val))
		Rast_set_null_value(&RASTER3D_ARRAY_ACCESS(grad_z, col, row, 0),
				    1, DCELL_TYPE);
	    else
		RASTER3D_ARRAY_ACCESS(grad_z, col, row, 0) =
			(-3 * val + 4 * RASTER3D_ARRAY_ACCESS(array, col, row, 1) -
			 RASTER3D_ARRAY_ACCESS(array, col, row, 2)) / (2 * step[2]);

	    /* vertical col end */
	    val = RASTER3D_ARRAY_ACCESS(array, col, row, array->sz - 1);
	    if (Rast_is_d_null_value(&val))
		Rast_set_null_value(
			&RASTER3D_ARRAY_ACCESS(grad_z, col, row, array->sz - 1),
			1, DCELL_TYPE);
	    else
		RASTER3D_ARRAY_ACCESS(grad_z, col, row, array->sz - 1) =
			(3 * val - 4 * RASTER3D_ARRAY_ACCESS(array, col, row, array->sz - 2) +
			 RASTER3D_ARRAY_ACCESS(array, col, row, array->sz - 3)) / (2 * step[2]);
	    /* vertical col */
	    for (depth = 1; depth < array->sz - 1; depth++) {
		val = RASTER3D_ARRAY_ACCESS(array, col, row, depth);
		if (Rast_is_d_null_value(&val))
		    Rast_set_null_value(
			    &RASTER3D_ARRAY_ACCESS(grad_z, col, row, depth),
			    1, DCELL_TYPE);
		else
		    RASTER3D_ARRAY_ACCESS(grad_z, col, row, depth) =
			    (RASTER3D_ARRAY_ACCESS(array, col, row, depth + 1) -
			     RASTER3D_ARRAY_ACCESS(array, col, row, depth - 1)) / (2 * step[2]);
	    }
	}
    }
}
