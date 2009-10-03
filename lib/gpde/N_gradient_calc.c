
/*****************************************************************************
*
* MODULE:       Grass PDE Numerical Library
* AUTHOR(S):    Soeren Gebbert, Berlin (GER) Dec 2006
* 		soerengebbert <at> gmx <dot> de
*               
* PURPOSE:     	gradient management functions 
* 		part of the gpde library
*
* COPYRIGHT:    (C) 2000 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*               License (>=v2). Read the file COPYING that comes with GRASS
*               for details.
*
*****************************************************************************/

#include <grass/N_pde.h>

/*! \brief Calculate basic statistics of a gradient field
 *
 * The statistic is stored in the gradient field struct
 *
 * \param field N_gradient_2d_field *
 * \return void
 *
 * */
void N_calc_gradient_field_2d_stats(N_gradient_field_2d * field)
{
    double minx, miny;
    double maxx, maxy;
    double sumx, sumy;
    int nonullx, nonully;

    G_debug(3,
	    "N_calc_gradient_field_2d_stats: compute gradient field stats");

    N_calc_array_2d_stats(field->x_array, &minx, &maxx, &sumx, &nonullx, 0);
    N_calc_array_2d_stats(field->y_array, &miny, &maxy, &sumy, &nonully, 0);

    if (minx < miny)
	field->min = minx;
    else
	field->min = miny;

    if (maxx > maxy)
	field->max = maxx;
    else
	field->max = maxy;

    field->sum = sumx + sumy;
    field->nonull = nonullx + nonully;
    field->mean = field->sum / (double)field->nonull;

    return;
}

/*!
 * \brief This function computes the gradient based on the input N_array_2d pot
 * (potential), a weighting factor N_array_2d named weight and the distance between two cells 
 * saved in the N_geom_data struct.
 *
 * The gradient is calculated between cells for each cell and direction.
 * An existing gradient field can be filled with new data or, if a NULL pointer is
 * given, a new gradient field will be allocated with the appropriate size.
 *
 *
 \verbatim
 ______________ 
 |    |    |    |
 |    |    |    |
 |----|-NC-|----|
 |    |    |    |
 |   WC    EC   |
 |    |    |    |
 |----|-SC-|----|
 |    |    |    |
 |____|____|____|


 x - direction:

 r = 2 * weight[row][col]*weight[row][col + 1] / (weight[row][col]*weight[row][col + 1])
 EC = r * (pot[row][col] - pot[row][col + 1])/dx

 y - direction:

 r = 2 * weight[row][col]*weight[row + 1][col] / (weight[row][col]*weight[row + 1][col])
 SC = r * (pot[row][col] - pot[row + 1][col])/dy

 the values SC and EC are the values of the next row/col


 \endverbatim
 * \param pot N_array_2d * - the potential N_array_2d 
 * \param weight_x N_array_2d * - the weighting factor N_array_2d used to modify the gradient in x-direction
 * \param weight_y N_array_2d * - the weighting factor N_array_2d used to modify the gradient in y-direction
 * \param geom N_geom_data * - geometry data structure
 * \param gradfield N_gradient_field_2d * - a gradient field of the correct size, if a NULL pointer is provided this gradient field will be new allocated
 * \return N_gradient_field_2d * - the pointer to the computed gradient field

 *
 * */
N_gradient_field_2d *N_compute_gradient_field_2d(N_array_2d * pot,
						 N_array_2d * weight_x,
						 N_array_2d * weight_y,
						 N_geom_data * geom,
						 N_gradient_field_2d *
						 gradfield)
{
    int i, j;
    int rows, cols;
    double dx, dy, p1, p2, r1, r2, mean, grad, res;
    N_gradient_field_2d *field = gradfield;


    if (pot->cols != weight_x->cols || pot->cols != weight_y->cols)
	G_fatal_error
	    ("N_compute_gradient_field_2d: the arrays are not of equal size");

    if (pot->rows != weight_x->rows || pot->rows != weight_y->rows)
	G_fatal_error
	    ("N_compute_gradient_field_2d: the arrays are not of equal size");

    if (pot->cols != geom->cols || pot->rows != geom->rows)
	G_fatal_error
	    ("N_compute_gradient_field_2d: array sizes and geometry data are different");


    G_debug(3, "N_compute_gradient_field_2d: compute gradient field");

    rows = pot->rows;
    cols = pot->cols;
    dx = geom->dx;
    dy = geom->dy;

    if (field == NULL) {
	field = N_alloc_gradient_field_2d(cols, rows);
    }
    else {
	if (field->cols != geom->cols || field->rows != geom->rows)
	    G_fatal_error
		("N_compute_gradient_field_2d: gradient field sizes and geometry data are different");
    }


    for (j = 0; j < rows; j++)
	for (i = 0; i < cols - 1; i++) {
	    grad = 0;
	    mean = 0;

	    /* Only compute if the arrays are not null */
	    if (!N_is_array_2d_value_null(pot, i, j) &&
		!N_is_array_2d_value_null(pot, i + 1, j)) {
		p1 = N_get_array_2d_d_value(pot, i, j);
		p2 = N_get_array_2d_d_value(pot, i + 1, j);
		grad = (p1 - p2) / dx;	/* gradient */
	    }
	    if (!N_is_array_2d_value_null(weight_x, i, j) &&
		!N_is_array_2d_value_null(weight_x, i + 1, j)) {
		r1 = N_get_array_2d_d_value(weight_x, i, j);
		r2 = N_get_array_2d_d_value(weight_x, i + 1, j);
		mean = N_calc_harmonic_mean(r1, r2);	/*harmonical mean */
	    }

	    res = mean * grad;

	    N_put_array_2d_d_value(field->x_array, i + 1, j, res);

	}

    for (j = 0; j < rows - 1; j++)
	for (i = 0; i < cols; i++) {
	    grad = 0;
	    mean = 0;

	    /* Only compute if the arrays are not null */
	    if (!N_is_array_2d_value_null(pot, i, j) &&
		!N_is_array_2d_value_null(pot, i, j + 1)) {
		p1 = N_get_array_2d_d_value(pot, i, j);
		p2 = N_get_array_2d_d_value(pot, i, j + 1);
		grad = (p1 - p2) / dy;	/* gradient */
	    }
	    if (!N_is_array_2d_value_null(weight_y, i, j) &&
		!N_is_array_2d_value_null(weight_y, i, j + 1)) {
		r1 = N_get_array_2d_d_value(weight_y, i, j);
		r2 = N_get_array_2d_d_value(weight_y, i, j + 1);
		mean = N_calc_harmonic_mean(r1, r2);	/*harmonical mean */
	    }

	    res = -1 * mean * grad;

	    N_put_array_2d_d_value(field->y_array, i, j + 1, res);

	}

    /*Compute gradient field statistics */
    N_calc_gradient_field_2d_stats(field);

    return field;
}

/*! 
 * \brief Calculate the x and y vector components from a gradient field for each 
 * cell and stores them in the provided N_array_2d structures
 *
 * The arrays must have the same size as the gradient field.

 \verbatim

 Based on this storages scheme the gradient vector for each cell is 
 calculated and stored in the provided  N_array_2d structures

 ______________ 
 |    |    |    |
 |    |    |    |
 |----|-NC-|----|
 |    |    |    |
 |   WC    EC   |
 |    |    |    |
 |----|-SC-|----|
 |    |    |    |
 |____|____|____|

 x vector component:

 x = (WC + EC) / 2

 y vector component:

 y = (NC + SC) / 2

 \endverbatim
 *
 * \param field N_gradient_field_2d *
 * \param x_comp N_array_2d * - the array in which the x component will be written
 * \param y_comp N_array_2d * - the array in which the y component will be written
 *
 * \return void
 * */
void
N_compute_gradient_field_components_2d(N_gradient_field_2d * field,
				       N_array_2d * x_comp,
				       N_array_2d * y_comp)
{
    int i, j;

    int rows, cols;

    double vx, vy;

    N_array_2d *x = x_comp;

    N_array_2d *y = y_comp;

    N_gradient_2d grad;


    if (!x)
	G_fatal_error("N_compute_gradient_components_2d: x array is empty");
    if (!y)
	G_fatal_error("N_compute_gradient_components_2d: y array is empty");

    cols = field->x_array->cols;
    rows = field->x_array->rows;

    /*Check the array sizes */
    if (x->cols != cols || x->rows != rows)
	G_fatal_error
	    ("N_compute_gradient_components_2d: the size of the x array doesn't fit the gradient field size");
    if (y->cols != cols || y->rows != rows)
	G_fatal_error
	    ("N_compute_gradient_components_2d: the size of the y array doesn't fit the gradient field size");

    for (j = 0; j < rows; j++)
	for (i = 0; i < cols; i++) {
	    N_get_gradient_2d(field, &grad, i, j);

	    /* in case a gradient is zero, we expect a no flow boundary */
	    if (grad.WC == 0.0 || grad.EC == 0.0)
		vx = (grad.WC + grad.EC);
	    else
		vx = (grad.WC + grad.EC) / 2;
	    if (grad.NC == 0.0 || grad.SC == 0.0)
		vy = (grad.NC + grad.SC);
	    else
		vy = (grad.NC + grad.SC) / 2;

	    N_put_array_2d_d_value(x, i, j, vx);
	    N_put_array_2d_d_value(y, i, j, vy);
	}

    return;
}

/*! \brief Calculate basic statistics of a gradient field
 *
 * The statistic is stored in the gradient field struct
 *
 * \param field N_gradient_3d_field *
 * \return void
 *
 * */
void N_calc_gradient_field_3d_stats(N_gradient_field_3d * field)
{
    double minx, miny, minz;

    double maxx, maxy, maxz;

    double sumx, sumy, sumz;

    int nonullx, nonully, nonullz;

    G_debug(3,
	    "N_calc_gradient_field_3d_stats: compute gradient field stats");

    N_calc_array_3d_stats(field->x_array, &minx, &maxx, &sumx, &nonullx, 0);
    N_calc_array_3d_stats(field->y_array, &miny, &maxy, &sumy, &nonully, 0);
    N_calc_array_3d_stats(field->z_array, &minz, &maxz, &sumz, &nonullz, 0);

    if (minx <= minz && minx <= miny)
	field->min = minx;
    if (miny <= minz && miny <= minx)
	field->min = miny;
    if (minz <= minx && minz <= miny)
	field->min = minz;

    if (maxx >= maxz && maxx >= maxy)
	field->max = maxx;
    if (maxy >= maxz && maxy >= maxx)
	field->max = maxy;
    if (maxz >= maxx && maxz >= maxy)
	field->max = maxz;

    field->sum = sumx + sumy + sumz;
    field->nonull = nonullx + nonully + nonullz;
    field->mean = field->sum / (double)field->nonull;

    return;
}


/*!
 * \brief This function computes the gradient based on the input N_array_3d pot
 * (that means potential), a weighting factor N_array_3d named weight and the distance between two cells 
 * saved in the N_geom_data struct.
 *
 * The gradient is calculated between cells for each cell and direction.
 * An existing gradient field can be filled with new data or, if a NULL pointer is
 * given, a new gradient field will be allocated with the appropriate size.
 *
 *
 *
 *
 \verbatim

 |  /
 TC NC
 |/
 --WC-----EC--
 /|
 SC BC
 /  |

 x - direction:

 r = 2 * weight_x[depth][row][col]*weight_x[depth][row][col + 1] / (weight_X[depth][row][col]*weight_x[depth][row][col + 1])
 EC = r * (pot[depth][row][col] - pot[depth][row][col + 1])/dx

 y - direction:

 r = 2 * weight_y[depth][row][col]*weight_y[depth][row + 1][col] / (weight_y[depth][row][col]*weight_y[depth][row + 1][col])
 SC = r * (pot[depth][row][col] - pot[depth][row + 1][col])/dy

 z - direction:

 r = 2 * weight_z[depth][row][col]*weight_z[depth + 1][row][col] / (weight_z[depth][row][col]*weight_z[depth + 1][row][col])
 TC = r * (pot[depth][row][col] - pot[depth + 1][row][col])/dy

 the values BC, NC, WC are the values of the next depth/row/col


 \endverbatim
 * \param pot N_array_3d * - the potential N_array_2d 
 * \param weight_x N_array_3d * - the weighting factor N_array_3d used to modify the gradient in x-direction
 * \param weight_y N_array_3d * - the weighting factor N_array_3d used to modify the gradient in y-direction
 * \param weight_z N_array_3d * - the weighting factor N_array_3d used to modify the gradient in z-direction
 * \param geom N_geom_data * - geometry data structure
 * \param gradfield N_gradient_field_3d * - a gradient field of the correct size, if a NULL pointer is provided this gradient field will be new allocated
 * \return N_gradient_field_3d * - the pointer to the computed gradient field
 *
 * */
N_gradient_field_3d *N_compute_gradient_field_3d(N_array_3d * pot,
						 N_array_3d * weight_x,
						 N_array_3d * weight_y,
						 N_array_3d * weight_z,
						 N_geom_data * geom,
						 N_gradient_field_3d *
						 gradfield)
{
    int i, j, k;

    int cols, rows, depths;

    double dx, dy, dz, p1, p2, r1, r2, mean, grad, res;

    N_gradient_field_3d *field = gradfield;


    if (pot->cols != weight_x->cols || pot->cols != weight_y->cols ||
	pot->cols != weight_z->cols)
	G_fatal_error
	    ("N_compute_gradient_field_3d: the arrays are not of equal size");

    if (pot->rows != weight_x->rows || pot->rows != weight_y->rows ||
	pot->rows != weight_z->rows)
	G_fatal_error
	    ("N_compute_gradient_field_3d: the arrays are not of equal size");

    if (pot->depths != weight_x->depths || pot->depths != weight_y->depths ||
	pot->depths != weight_z->depths)
	G_fatal_error
	    ("N_compute_gradient_field_3d: the arrays are not of equal size");

    if (pot->cols != geom->cols || pot->rows != geom->rows ||
	pot->depths != geom->depths)
	G_fatal_error
	    ("N_compute_gradient_field_3d: array sizes and geometry data are different");

    G_debug(3, "N_compute_gradient_field_3d: compute gradient field");

    cols = geom->cols;
    rows = geom->rows;
    depths = geom->depths;
    dx = geom->dx;
    dy = geom->dy;
    dz = geom->dz;

    if (gradfield == NULL) {
	field = N_alloc_gradient_field_3d(cols, rows, depths);
    }
    else {
	if (field->cols != geom->cols || field->rows != geom->rows ||
	    field->depths != geom->depths)
	    G_fatal_error
		("N_compute_gradient_field_3d: gradient field sizes and geometry data are different");
    }

    for (k = 0; k < depths; k++)
	for (j = 0; j < rows; j++)
	    for (i = 0; i < cols - 1; i++) {
		grad = 0;
		mean = 0;

		/*Only compute if the arrays are not null */
		if (!N_is_array_3d_value_null(pot, i, j, k) &&
		    !N_is_array_3d_value_null(pot, i + 1, j, k)) {
		    p1 = N_get_array_3d_d_value(pot, i, j, k);
		    p2 = N_get_array_3d_d_value(pot, i + 1, j, k);
		    grad = (p1 - p2) / dx;	/* gradient */
		}
		if (!N_is_array_3d_value_null(weight_x, i, j, k) &&
		    !N_is_array_3d_value_null(weight_x, i + 1, j, k)) {
		    r1 = N_get_array_3d_d_value(weight_x, i, j, k);
		    r2 = N_get_array_3d_d_value(weight_x, i + 1, j, k);
		    mean = N_calc_harmonic_mean(r1, r2);	/*harmonical mean */
		}

		res = mean * grad;

		G_debug(6,
			"N_compute_gradient_field_3d: X-direction insert value %6.5g at %i %i %i ",
			res, k, j, i + 1);

		N_put_array_3d_d_value(field->x_array, i + 1, j, k, res);

	    }

    for (k = 0; k < depths; k++)
	for (j = 0; j < rows - 1; j++)
	    for (i = 0; i < cols; i++) {
		grad = 0;
		mean = 0;

		/* Only compute if the arrays are not null */
		if (!N_is_array_3d_value_null(pot, i, j, k) &&
		    !N_is_array_3d_value_null(pot, i, j + 1, k)) {
		    p1 = N_get_array_3d_d_value(pot, i, j, k);
		    p2 = N_get_array_3d_d_value(pot, i, j + 1, k);
		    grad = (p1 - p2) / dy;	/* gradient */
		}
		if (!N_is_array_3d_value_null(weight_y, i, j, k) &&
		    !N_is_array_3d_value_null(weight_y, i, j + 1, k)) {
		    r1 = N_get_array_3d_d_value(weight_y, i, j, k);
		    r2 = N_get_array_3d_d_value(weight_y, i, j + 1, k);
		    mean = N_calc_harmonic_mean(r1, r2);	/*harmonical mean */
		}

		res = -1 * mean * grad;	/*invert the direction, because we count from north to south,
					 * but the gradient is defined in y direction */

		G_debug(6,
			"N_compute_gradient_field_3d: Y-direction insert value %6.5g at %i %i %i ",
			res, k, j + 1, i);

		N_put_array_3d_d_value(field->y_array, i, j + 1, k, res);

	    }

    for (k = 0; k < depths - 1; k++)
	for (j = 0; j < rows; j++)
	    for (i = 0; i < cols; i++) {
		grad = 0;
		mean = 0;

		/* Only compute if the arrays are not null */
		if (!N_is_array_3d_value_null(pot, i, j, k) &&
		    !N_is_array_3d_value_null(pot, i, j, k + 1)) {
		    p1 = N_get_array_3d_d_value(pot, i, j, k);
		    p2 = N_get_array_3d_d_value(pot, i, j, k + 1);
		    grad = (p1 - p2) / dz;	/* gradient */
		}
		if (!N_is_array_3d_value_null(weight_z, i, j, k) &&
		    !N_is_array_3d_value_null(weight_z, i, j, k + 1)) {
		    r1 = N_get_array_3d_d_value(weight_z, i, j, k);
		    r2 = N_get_array_3d_d_value(weight_z, i, j, k + 1);
		    mean = N_calc_harmonic_mean(r1, r2);	/*harmonical mean */
		}

		res = mean * grad;

		G_debug(6,
			"N_compute_gradient_field_3d: Z-direction insert value %6.5g at %i %i %i ",
			res, k + 1, j, i);

		N_put_array_3d_d_value(field->z_array, i, j, k + 1, res);

	    }

    /*Compute gradient field statistics */
    N_calc_gradient_field_3d_stats(field);

    return field;
}

/*! 
 * \brief Calculate the x, y and z vector components from a gradient field for each cell 
 *  and store them in the provided N_array_3d structures
 *
 * The arrays must have the same size as the gradient field.
 *
 \verbatim

 Based on this storages scheme the gradient vector for each cell is 
 calculated and stored in the provided  N_array_3d structures


 |  /
 TC NC
 |/
 --WC-----EC--
 /|
 SC BC
 /  |


 x vector component:

 x = (WC + EC) / 2

 y vector component:

 y = (NC + SC) / 2

 z vector component:

 z = (TC + BC) / 2

 \endverbatim

 * \param field N_gradient_field_3d *
 * \param x_comp N_array_3d * - the array in which the x component will be written
 * \param y_comp N_array_3d * - the array in which the y component will be written
 * \param z_comp N_array_3d * - the array in which the z component will be written
 *
 * \return void
 * */
void
N_compute_gradient_field_components_3d(N_gradient_field_3d * field,
				       N_array_3d * x_comp,
				       N_array_3d * y_comp,
				       N_array_3d * z_comp)
{
    int i, j, k;

    int rows, cols, depths;

    double vx, vy, vz;

    N_array_3d *x = x_comp;

    N_array_3d *y = y_comp;

    N_array_3d *z = z_comp;

    N_gradient_3d grad;


    if (!x)
	G_fatal_error("N_compute_gradient_components_3d: x array is empty");
    if (!y)
	G_fatal_error("N_compute_gradient_components_3d: y array is empty");
    if (!z)
	G_fatal_error("N_compute_gradient_components_3d: z array is empty");

    cols = field->x_array->cols;
    rows = field->x_array->rows;
    depths = field->x_array->depths;

    /*Check the array sizes */
    if (x->cols != cols || x->rows != rows || x->depths != depths)
	G_fatal_error
	    ("N_compute_gradient_components_3d: the size of the x array doesn't fit the gradient field size");
    if (y->cols != cols || y->rows != rows || y->depths != depths)
	G_fatal_error
	    ("N_compute_gradient_components_3d: the size of the y array doesn't fit the gradient field size");
    if (z->cols != cols || z->rows != rows || z->depths != depths)
	G_fatal_error
	    ("N_compute_gradient_components_3d: the size of the z array doesn't fit the gradient field size");

    for (k = 0; k < depths; k++)
	for (j = 0; j < rows; j++)
	    for (i = 0; i < cols; i++) {
		N_get_gradient_3d(field, &grad, i, j, k);
		/* in case a gradient is zero, we expect a no flow boundary */
		if (grad.WC == 0.0 || grad.EC == 0.0)
		    vx = (grad.WC + grad.EC);
		else
		    vx = (grad.WC + grad.EC) / 2;
		if (grad.NC == 0.0 || grad.SC == 0.0)
		    vy = (grad.NC + grad.SC);
		else
		    vy = (grad.NC + grad.SC) / 2;
		if (grad.TC == 0.0 || grad.BC == 0.0)
		    vz = (grad.TC + grad.BC);
		else
		    vz = (grad.TC + grad.BC) / 2;

		N_put_array_3d_d_value(x, i, j, k, vx);
		N_put_array_3d_d_value(y, i, j, k, vy);
		N_put_array_3d_d_value(z, i, j, k, vz);
	    }


    return;
}
