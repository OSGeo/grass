
/*****************************************************************************
*
* MODULE:       Grass PDE Numerical Library
* AUTHOR(S):    Soeren Gebbert, Berlin (GER) Dec 2006
* 		soerengebbert <at> gmx <dot> de
*               
* PURPOSE:     	Higher level array management functions 
* 		part of the gpde library
*
* COPYRIGHT:    (C) 2000 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*               License (>=v2). Read the file COPYING that comes with GRASS
*               for details.
*
*****************************************************************************/
#include <math.h>

#include <grass/N_pde.h>
#include <grass/raster.h>
#include <grass/glocale.h>


/* ******************** 2D ARRAY FUNCTIONS *********************** */

/*!
 * \brief Copy the source N_array_2d struct to the target N_array_2d struct
 *
 * The arrays must have the same size and the same offset.
 *
 * The array types can be mixed, the values are automatically casted
 * and the null values are set accordingly.
 * <br><br>
 * If you copy a cell array into a dcell array, the values are casted to dcell and 
 * the null values are converted from cell-null to dcell-null
 * <br><br>
 * This function can be called in a parallel region defined with OpenMP.
 * The copy loop is parallelize with a openmp for pragma.
 *
 * \param source N_array_2d *
 * \param target N_array_2d *
 * \return void
 * */
void N_copy_array_2d(N_array_2d * source, N_array_2d * target)
{
    int i;
    int null = 0;

#pragma omp single
    {
	if (source->cols_intern != target->cols_intern)
	    G_fatal_error
		("N_copy_array_2d: the arrays are not of equal size");

	if (source->rows_intern != target->rows_intern)
	    G_fatal_error
		("N_copy_array_2d: the arrays are not of equal size");

	G_debug(3,
		"N_copy_array_2d: copy source array to target array size %i",
		source->cols_intern * source->rows_intern);
    }

#pragma omp for
    for (i = 0; i < source->cols_intern * source->rows_intern; i++) {
	null = 0;
	if (source->type == CELL_TYPE) {
	    if (Rast_is_c_null_value((void *)&source->cell_array[i]))
		null = 1;

	    if (target->type == CELL_TYPE) {
		target->cell_array[i] = source->cell_array[i];
	    }
	    if (target->type == FCELL_TYPE) {
		if (null)
		    Rast_set_f_null_value((void *)&(target->fcell_array[i]), 1);
		else
		    target->fcell_array[i] = (FCELL) source->cell_array[i];
	    }
	    if (target->type == DCELL_TYPE) {
		if (null)
		    Rast_set_d_null_value((void *)&(target->dcell_array[i]), 1);
		else
		    target->dcell_array[i] = (DCELL) source->cell_array[i];
	    }

	}
	if (source->type == FCELL_TYPE) {
	    if (Rast_is_f_null_value((void *)&source->fcell_array[i]))
		null = 1;

	    if (target->type == CELL_TYPE) {
		if (null)
		    Rast_set_c_null_value((void *)&(target->cell_array[i]), 1);
		else
		    target->cell_array[i] = (CELL) source->fcell_array[i];
	    }
	    if (target->type == FCELL_TYPE) {
		target->fcell_array[i] = source->fcell_array[i];
	    }
	    if (target->type == DCELL_TYPE) {
		if (null)
		    Rast_set_d_null_value((void *)&(target->dcell_array[i]), 1);
		else
		    target->dcell_array[i] = (DCELL) source->fcell_array[i];
	    }
	}
	if (source->type == DCELL_TYPE) {
	    if (Rast_is_d_null_value((void *)&source->dcell_array[i]))
		null = 1;

	    if (target->type == CELL_TYPE) {
		if (null)
		    Rast_set_c_null_value((void *)&(target->cell_array[i]), 1);
		else
		    target->cell_array[i] = (CELL) source->dcell_array[i];
	    }
	    if (target->type == FCELL_TYPE) {
		if (null)
		    Rast_set_f_null_value((void *)&(target->fcell_array[i]), 1);
		else
		    target->fcell_array[i] = (FCELL) source->dcell_array[i];
	    }
	    if (target->type == DCELL_TYPE) {
		target->dcell_array[i] = source->dcell_array[i];
	    }
	}
    }

    return;
}

/*!
 * \brief Calculate the norm of the two input arrays
 *
 * The norm can be of type N_MAXIMUM_NORM or N_EUKLID_NORM.
 * All arrays must have equal sizes and offsets.
 * The complete data array inclusively offsets is used for norm calucaltion.
 * Only non-null values are used to calculate the norm.
 *

 * \param a N_array_2d *
 * \param b N_array_2d *
 * \param type the type of the norm -> N_MAXIMUM_NORM, N_EUKLID_NORM
 * \return double the calculated norm
 * */
double N_norm_array_2d(N_array_2d * a, N_array_2d * b, int type)
{
    int i = 0;
    double norm = 0.0, tmp = 0.0;
    double v1 = 0.0, v2 = 0.0;

    if (a->cols_intern != b->cols_intern)
	G_fatal_error("N_norm_array_2d: the arrays are not of equal size");

    if (a->rows_intern != b->rows_intern)
	G_fatal_error("N_norm_array_2d: the arrays are not of equal size");

    G_debug(3, "N_norm_array_2d: norm of a and b size %i",
	    a->cols_intern * a->rows_intern);

    for (i = 0; i < a->cols_intern * a->rows_intern; i++) {
	v1 = 0.0;
	v2 = 0.0;

	if (a->type == CELL_TYPE) {
	    if (!Rast_is_f_null_value((void *)&(a->cell_array[i])))
		v1 = (double)a->cell_array[i];
	}
	if (a->type == FCELL_TYPE) {
	    if (!Rast_is_f_null_value((void *)&(a->fcell_array[i])))
		v1 = (double)a->fcell_array[i];
	}
	if (a->type == DCELL_TYPE) {
	    if (!Rast_is_f_null_value((void *)&(a->dcell_array[i])))
		v1 = (double)a->dcell_array[i];
	}
	if (b->type == CELL_TYPE) {
	    if (!Rast_is_f_null_value((void *)&(b->cell_array[i])))
		v2 = (double)b->cell_array[i];
	}
	if (b->type == FCELL_TYPE) {
	    if (!Rast_is_f_null_value((void *)&(b->fcell_array[i])))
		v2 = (double)b->fcell_array[i];
	}
	if (b->type == DCELL_TYPE) {
	    if (!Rast_is_f_null_value((void *)&(b->dcell_array[i])))
		v2 = (double)b->dcell_array[i];
	}

	if (type == N_MAXIMUM_NORM) {
	    tmp = fabs(v2 - v1);
	    if ((tmp > norm))
		norm = tmp;
	}
	if (type == N_EUKLID_NORM) {
	    norm += fabs(v2 - v1);
	}
    }

    return norm;
}

/*!
 * \brief Calculate basic statistics of the N_array_2d struct 
 *
 * Calculates the minimum, maximum, sum and the number of 
 * non null values. The array offset can be included in the calculation.
 *
 * \param a N_array_2d * - input array
 * \param min double* - variable to store the computed minimum
 * \param max double* - variable to store the computed maximum
 * \param sum double* - variable to store the computed sum
 * \param nonull int* - variable to store the number of non null values
 * \param withoffset - if 1 include offset values in statistic calculation, 0 otherwise 
 * \return void
 * */
void N_calc_array_2d_stats(N_array_2d * a, double *min, double *max,
			   double *sum, int *nonull, int withoffset)
{
    int i, j;
    double val;

    *sum = 0.0;
    *nonull = 0;

    if (withoffset == 1) {

	*min =
	    (double)N_get_array_2d_d_value(a, 0 - a->offset, 0 - a->offset);
	*max =
	    (double)N_get_array_2d_d_value(a, 0 - a->offset, 0 - a->offset);

	for (j = 0 - a->offset; j < a->rows + a->offset; j++) {
	    for (i = 0 - a->offset; i < a->cols + a->offset; i++) {
		if (!N_is_array_2d_value_null(a, i, j)) {
		    val = (double)N_get_array_2d_d_value(a, i, j);
		    if (*min > val)
			*min = val;
		    if (*max < val)
			*max = val;
		    *sum += val;
		    (*nonull)++;
		}
	    }
	}
    }
    else {

	*min = (double)N_get_array_2d_d_value(a, 0, 0);
	*max = (double)N_get_array_2d_d_value(a, 0, 0);


	for (j = 0; j < a->rows; j++) {
	    for (i = 0; i < a->cols; i++) {
		if (!N_is_array_2d_value_null(a, i, j)) {
		    val = (double)N_get_array_2d_d_value(a, i, j);
		    if (*min > val)
			*min = val;
		    if (*max < val)
			*max = val;
		    *sum += val;
		    (*nonull)++;
		}
	    }
	}
    }

    G_debug(3,
	    "N_calc_array_2d_stats: compute array stats, min %g, max %g, sum %g, nonull %i",
	    *min, *max, *sum, *nonull);
    return;
}


/*!
 * \brief Perform calculations with two input arrays, 
 * the result is written to a third array.
 *
 * All arrays must have equal sizes and offsets.
 * The complete data array inclusively offsets is used for calucaltions.
 * Only non-null values are computed. If one array value is null, 
 * the result array value will be null too.
 * <br><br>
 * If a division with zero is detected, the resulting arrays 
 * value will set to null and not to NaN.
 * <br><br>
 * The result array is optional, if the result arrays points to NULL,
 * a new array will be allocated with the largest arrays data type
 * (CELL, FCELL or DCELL) used by the input arrays.
 * <br><br>
 * the array computations can be of the following forms:
 *
 * <ul>
 * <li>result = a + b -> N_ARRAY_SUM</li>
 * <li>result = a - b -> N_ARRAY_DIF</li>
 * <li>result = a * b -> N_ARRAY_MUL</li>
 * <li>result = a / b -> N_ARRAY_DIV</li>
 * </ul>
 *
 * \param a N_array_2d * - first input array
 * \param b N_array_2d * - second input array
 * \param result N_array_2d * - the optional result array
 * \param type  - the type of calculation
 * \return N_array_2d * - the pointer to the result array
 * */
N_array_2d *N_math_array_2d(N_array_2d * a, N_array_2d * b,
			    N_array_2d * result, int type)
{
    N_array_2d *c;
    int i, j, setnull = 0;
    double va = 0.0, vb = 0.0, vc = 0.0;	/*variables used for calculation */

    /*Set the pointer */
    c = result;

#pragma omp single
    {
	/*Check the array sizes */
	if (a->cols_intern != b->cols_intern)
	    G_fatal_error
		("N_math_array_2d: the arrays are not of equal size");
	if (a->rows_intern != b->rows_intern)
	    G_fatal_error
		("N_math_array_2d: the arrays are not of equal size");
	if (a->offset != b->offset)
	    G_fatal_error
		("N_math_array_2d: the arrays have different offsets");

	G_debug(3, "N_math_array_2d: mathematical calculations, size: %i",
		a->cols_intern * a->rows_intern);

	/*if the result array is null, allocate a new one, use the 
	 * largest data type of the input arrays*/
	if (c == NULL) {
	    if (a->type == DCELL_TYPE || b->type == DCELL_TYPE) {
		c = N_alloc_array_2d(a->cols, a->rows, a->offset, DCELL_TYPE);
		G_debug(3,
			"N_math_array_2d: array of type DCELL_TYPE created");
	    }
	    else if (a->type == FCELL_TYPE || b->type == FCELL_TYPE) {
		c = N_alloc_array_2d(a->cols, a->rows, a->offset, FCELL_TYPE);
		G_debug(3,
			"N_math_array_2d: array of type FCELL_TYPE created");
	    }
	    else {
		c = N_alloc_array_2d(a->cols, a->rows, a->offset, CELL_TYPE);
		G_debug(3,
			"N_math_array_2d: array of type CELL_TYPE created");
	    }
	}
	else {
	    /*Check the array sizes */
	    if (a->cols_intern != c->cols_intern)
		G_fatal_error
		    ("N_math_array_2d: the arrays are not of equal size");
	    if (a->rows_intern != c->rows_intern)
		G_fatal_error
		    ("N_math_array_2d: the arrays are not of equal size");
	    if (a->offset != c->offset)
		G_fatal_error
		    ("N_math_array_2d: the arrays have different offsets");
	}
    }

#pragma omp for private(va, vb, vc, setnull)
    for (j = 0 - a->offset; j < a->rows + a->offset; j++) {
	for (i = 0 - a->offset; i < a->cols + a->offset; i++) {
	    if (!N_is_array_2d_value_null(a, i, j) &&
		!N_is_array_2d_value_null(b, i, j)) {
		/*we always calculate internally with double values */
		va = (double)N_get_array_2d_d_value(a, i, j);
		vb = (double)N_get_array_2d_d_value(b, i, j);
		vc = 0;
		setnull = 0;

		switch (type) {
		case N_ARRAY_SUM:
		    vc = va + vb;
		    break;
		case N_ARRAY_DIF:
		    vc = va - vb;
		    break;
		case N_ARRAY_MUL:
		    vc = va * vb;
		    break;
		case N_ARRAY_DIV:
		    if (vb != 0)
			vc = va / vb;
		    else
			setnull = 1;
		    break;
		}

		if (c->type == CELL_TYPE) {
		    if (setnull)
			N_put_array_2d_value_null(c, i, j);
		    else
			N_put_array_2d_c_value(c, i, j, (CELL) vc);
		}
		if (c->type == FCELL_TYPE) {
		    if (setnull)
			N_put_array_2d_value_null(c, i, j);
		    else
			N_put_array_2d_f_value(c, i, j, (FCELL) vc);
		}
		if (c->type == DCELL_TYPE) {
		    if (setnull)
			N_put_array_2d_value_null(c, i, j);
		    else
			N_put_array_2d_d_value(c, i, j, (DCELL) vc);
		}

	    }
	    else {
		N_put_array_2d_value_null(c, i, j);
	    }
	}
    }

    return c;
}

/*!
 * \brief Convert all null values to zero values
 *
 * The complete data array inclusively offsets is used.
 * The array data types are automatically recognized.
 *
 * \param a N_array_2d *
 * \return int - number of replaced values
 * */
int N_convert_array_2d_null_to_zero(N_array_2d * a)
{
    int i = 0, count = 0;

    G_debug(3, "N_convert_array_2d_null_to_zero: convert array of size %i",
	    a->cols_intern * a->rows_intern);

    if (a->type == CELL_TYPE)
	for (i = 0; i < a->cols_intern * a->rows_intern; i++) {
	    if (Rast_is_c_null_value((void *)&(a->cell_array[i]))) {
		a->cell_array[i] = 0;
		count++;
	    }
	}

    if (a->type == FCELL_TYPE)
	for (i = 0; i < a->cols_intern * a->rows_intern; i++) {
	    if (Rast_is_f_null_value((void *)&(a->fcell_array[i]))) {
		a->fcell_array[i] = 0.0;
		count++;
	    }
	}


    if (a->type == DCELL_TYPE)
	for (i = 0; i < a->cols_intern * a->rows_intern; i++) {
	    if (Rast_is_d_null_value((void *)&(a->dcell_array[i]))) {
		a->dcell_array[i] = 0.0;
		count++;
	    }
	}


    if (a->type == CELL_TYPE)
	G_debug(2,
		"N_convert_array_2d_null_to_zero: %i values of type CELL_TYPE are converted",
		count);
    if (a->type == FCELL_TYPE)
	G_debug(2,
		"N_convert_array_2d_null_to_zero: %i valuess of type FCELL_TYPE are converted",
		count);
    if (a->type == DCELL_TYPE)
	G_debug(2,
		"N_convert_array_2d_null_to_zero: %i valuess of type DCELL_TYPE are converted",
		count);

    return count;
}

/* ******************** 3D ARRAY FUNCTIONS *********************** */

/*!
 * \brief Copy the source N_array_3d struct to the target N_array_3d struct
 *
 * The arrays must have the same size and the same offset.
 *
 * The array data types can be mixed, the values are automatically casted
 * and the null values are set accordingly.
 *
 * If you copy a float array to a double array, the values are casted to DCELL and 
 * the null values are converted from FCELL-null to DCELL-null
 *
 * \param source N_array_3d *
 * \param target N_array_3d *
 * \return void
 * */
void N_copy_array_3d(N_array_3d * source, N_array_3d * target)
{
    int i;
    int null;

    if (source->cols_intern != target->cols_intern)
	G_fatal_error("N_copy_array_3d: the arrays are not of equal size");

    if (source->rows_intern != target->rows_intern)
	G_fatal_error("N_copy_array_3d: the arrays are not of equal size");

    if (source->depths_intern != target->depths_intern)
	G_fatal_error("N_copy_array_3d: the arrays are not of equal size");


    G_debug(3, "N_copy_array_3d: copy source array to target array size %i",
	    source->cols_intern * source->rows_intern *
	    source->depths_intern);

    for (i = 0;
	 i <
	 source->cols_intern * source->rows_intern * source->depths_intern;
	 i++) {
	null = 0;
	if (source->type == FCELL_TYPE) {
	    if (Rast3d_is_null_value_num
		((void *)&(source->fcell_array[i]), FCELL_TYPE))
		null = 1;

	    if (target->type == FCELL_TYPE) {
		target->fcell_array[i] = source->fcell_array[i];
	    }
	    if (target->type == DCELL_TYPE) {
		if (null)
		    Rast3d_set_null_value((void *)&(target->dcell_array[i]), 1,
				     DCELL_TYPE);
		else
		    target->dcell_array[i] = (double)source->fcell_array[i];
	    }

	}
	if (source->type == DCELL_TYPE) {
	    if (Rast3d_is_null_value_num
		((void *)&(source->dcell_array[i]), DCELL_TYPE))
		null = 1;

	    if (target->type == FCELL_TYPE) {
		if (null)
		    Rast3d_set_null_value((void *)&(target->fcell_array[i]), 1,
				     FCELL_TYPE);
		else
		    target->fcell_array[i] = (float)source->dcell_array[i];
	    }
	    if (target->type == DCELL_TYPE) {
		target->dcell_array[i] = source->dcell_array[i];
	    }
	}
    }

    return;
}


/*!
 * \brief Calculate the norm of the two input arrays
 *
 * The norm can be of type N_MAXIMUM_NORM or N_EUKLID_NORM.
 * All arrays must have equal sizes and offsets.
 * The complete data array inclusively offsets is used for norm calucaltion.
 * Only non-null values are used to calculate the norm.
 *
 * \param a N_array_3d *
 * \param b N_array_3d *
 * \param type the type of the norm -> N_MAXIMUM_NORM, N_EUKLID_NORM
 * \return double the calculated norm
 * */
double N_norm_array_3d(N_array_3d * a, N_array_3d * b, int type)
{
    int i = 0;
    double norm = 0.0, tmp = 0.0;
    double v1 = 0.0, v2 = 0.0;

    if (a->cols_intern != b->cols_intern)
	G_fatal_error("N_norm_array_3d: the arrays are not of equal size");

    if (a->rows_intern != b->rows_intern)
	G_fatal_error("N_norm_array_3d: the arrays are not of equal size");

    if (a->depths_intern != b->depths_intern)
	G_fatal_error("N_norm_array_3d: the arrays are not of equal size");

    G_debug(3, "N_norm_array_3d: norm of a and b size %i",
	    a->cols_intern * a->rows_intern * a->depths_intern);

    for (i = 0; i < a->cols_intern * a->rows_intern * a->depths_intern; i++) {
	v1 = 0.0;
	v2 = 0.0;

	if (a->type == FCELL_TYPE) {
	    if (!Rast3d_is_null_value_num((void *)&(a->fcell_array[i]), FCELL_TYPE))
		v1 = (double)a->fcell_array[i];
	}
	if (a->type == DCELL_TYPE) {
	    if (!Rast3d_is_null_value_num((void *)&(a->dcell_array[i]), DCELL_TYPE))
		v1 = (double)a->dcell_array[i];
	}
	if (b->type == FCELL_TYPE) {
	    if (!Rast3d_is_null_value_num((void *)&(b->fcell_array[i]), FCELL_TYPE))
		v2 = (double)b->fcell_array[i];
	}
	if (b->type == DCELL_TYPE) {
	    if (!Rast3d_is_null_value_num((void *)&(b->dcell_array[i]), DCELL_TYPE))
		v2 = (double)b->dcell_array[i];
	}

	if (type == N_MAXIMUM_NORM) {
	    tmp = fabs(v2 - v1);
	    if ((tmp > norm))
		norm = tmp;
	}
	if (type == N_EUKLID_NORM) {
	    norm += fabs(v2 - v1);
	}
    }

    return norm;
}

/*!
 * \brief Calculate basic statistics of the N_array_3d struct
 *
 * Calculates the minimum, maximum, sum and the number of 
 * non null values. The array offset can be included in the statistical calculation.
 *
 * \param a N_array_3d * - input array
 * \param min double* - variable to store the computed minimum
 * \param max double* - variable to store the computed maximum
 * \param sum double* - variable to store the computed sum
 * \param nonull int* - variable to store the number of non null values
 * \param withoffset - if 1 include offset values in statistic calculation, 0 otherwise 
 * \return void
 * */
void N_calc_array_3d_stats(N_array_3d * a, double *min, double *max,
			   double *sum, int *nonull, int withoffset)
{
    int i, j, k;
    double val;

    *sum = 0.0;
    *nonull = 0;

    if (withoffset == 1) {

	*min =
	    (double)N_get_array_3d_d_value(a, 0 - a->offset, 0 - a->offset,
					   0 - a->offset);
	*max =
	    (double)N_get_array_3d_d_value(a, 0 - a->offset, 0 - a->offset,
					   0 - a->offset);

	for (k = 0 - a->offset; k < a->depths + a->offset; k++) {
	    for (j = 0 - a->offset; j < a->rows + a->offset; j++) {
		for (i = 0 - a->offset; i < a->cols + a->offset; i++) {
		    if (!N_is_array_3d_value_null(a, i, j, k)) {
			val = (double)N_get_array_3d_d_value(a, i, j, k);
			if (*min > val)
			    *min = val;
			if (*max < val)
			    *max = val;
			*sum += val;
			(*nonull)++;
		    }
		}
	    }
	}
    }
    else {

	*min = (double)N_get_array_3d_d_value(a, 0, 0, 0);
	*max = (double)N_get_array_3d_d_value(a, 0, 0, 0);

	for (k = 0; k < a->depths; k++) {
	    for (j = 0; j < a->rows; j++) {
		for (i = 0; i < a->cols; i++) {
		    if (!N_is_array_3d_value_null(a, i, j, k)) {
			val = (double)N_get_array_3d_d_value(a, i, j, k);
			if (*min > val)
			    *min = val;
			if (*max < val)
			    *max = val;
			*sum += val;
			(*nonull)++;
		    }
		}
	    }
	}
    }

    G_debug(3,
	    "N_calc_array_3d_stats: compute array stats, min %g, max %g, sum %g, nonull %i",
	    *min, *max, *sum, *nonull);

    return;
}

/*!
 * \brief Perform calculations with two input arrays, 
 * the result is written to a third array.
 *
 * All arrays must have equal sizes and offsets.
 * The complete data array inclusively offsets is used for calucaltions.
 * Only non-null values are used. If one array value is null, 
 * the result array value will be null too.
 * <br><br>
 *
 * If a division with zero is detected, the resulting arrays 
 * value will set to null and not to NaN.
 * <br><br>
 *
 * The result array is optional, if the result arrays points to NULL,
 * a new array will be allocated with the largest arrays data type
 * (FCELL_TYPE or DCELL_TYPE) used by the input arrays.
 * <br><br>
 *
 * the calculations are of the following form:
 *
 * <ul>
 * <li>result = a + b -> N_ARRAY_SUM</li>
 * <li>result = a - b -> N_ARRAY_DIF</li>
 * <li>result = a * b -> N_ARRAY_MUL</li>
 * <li>result = a / b -> N_ARRAY_DIV</li>
 * </ul>
 *
 * \param a N_array_3d * - first input array
 * \param b N_array_3d * - second input array
 * \param result N_array_3d * - the optional result array
 * \param type  - the type of calculation
 * \return N_array_3d * - the pointer to the result array
 * */
N_array_3d *N_math_array_3d(N_array_3d * a, N_array_3d * b,
			    N_array_3d * result, int type)
{
    N_array_3d *c;
    int i, j, k, setnull = 0;
    double va = 0.0, vb = 0.0, vc = 0.0;	/*variables used for calculation */

    /*Set the pointer */
    c = result;

    /*Check the array sizes */
    if (a->cols_intern != b->cols_intern)
	G_fatal_error("N_math_array_3d: the arrays are not of equal size");
    if (a->rows_intern != b->rows_intern)
	G_fatal_error("N_math_array_3d: the arrays are not of equal size");
    if (a->depths_intern != b->depths_intern)
	G_fatal_error("N_math_array_3d: the arrays are not of equal size");
    if (a->offset != b->offset)
	G_fatal_error("N_math_array_3d: the arrays have different offsets");

    G_debug(3, "N_math_array_3d: mathematical calculations, size: %i",
	    a->cols_intern * a->rows_intern * a->depths_intern);

    /*if the result array is null, allocate a new one, use the 
     * largest data type of the input arrays*/
    if (c == NULL) {
	if (a->type == DCELL_TYPE || b->type == DCELL_TYPE) {
	    c = N_alloc_array_3d(a->cols, a->rows, a->depths, a->offset,
				 DCELL_TYPE);
	    G_debug(3, "N_math_array_3d: array of type DCELL_TYPE created");
	}
	else {
	    c = N_alloc_array_3d(a->cols, a->rows, a->depths, a->offset,
				 FCELL_TYPE);
	    G_debug(3, "N_math_array_3d: array of type FCELL_TYPE created");
	}
    }
    else {
	/*Check the array sizes */
	if (a->cols_intern != c->cols_intern)
	    G_fatal_error
		("N_math_array_3d: the arrays are not of equal size");
	if (a->rows_intern != c->rows_intern)
	    G_fatal_error
		("N_math_array_3d: the arrays are not of equal size");
	if (a->depths_intern != c->depths_intern)
	    G_fatal_error
		("N_math_array_3d: the arrays are not of equal size");
	if (a->offset != c->offset)
	    G_fatal_error
		("N_math_array_3d: the arrays have different offsets");
    }

    for (k = 0 - a->offset; k < a->depths + a->offset; k++) {
	for (j = 0 - a->offset; j < a->rows + a->offset; j++) {
	    for (i = 0 - a->offset; i < a->cols + a->offset; i++) {
		if (!N_is_array_3d_value_null(a, i, j, k) &&
		    !N_is_array_3d_value_null(a, i, j, k)) {
		    /*we always calculate internally with double values */
		    va = (double)N_get_array_3d_d_value(a, i, j, k);
		    vb = (double)N_get_array_3d_d_value(b, i, j, k);
		    vc = 0;
		    setnull = 0;

		    switch (type) {
		    case N_ARRAY_SUM:
			vc = va + vb;
			break;
		    case N_ARRAY_DIF:
			vc = va - vb;
			break;
		    case N_ARRAY_MUL:
			vc = va * vb;
			break;
		    case N_ARRAY_DIV:
			if (vb != 0)
			    vc = va / vb;
			else
			    setnull = 1;
			break;
		    }

		    if (c->type == FCELL_TYPE) {
			if (setnull)
			    N_put_array_3d_value_null(c, i, j, k);
			else
			    N_put_array_3d_f_value(c, i, j, k, (float)vc);
		    }
		    if (c->type == DCELL_TYPE) {
			if (setnull)
			    N_put_array_3d_value_null(c, i, j, k);
			else
			    N_put_array_3d_d_value(c, i, j, k, vc);
		    }
		}
		else {
		    N_put_array_3d_value_null(c, i, j, k);
		}
	    }
	}
    }

    return c;
}

/*!
 * \brief Convert all null values to zero values
 *
 * The complete data array inclusively offsets is used.
 *
 * \param a N_array_3d *
 * \return int - number of replaced null values
 * */
int N_convert_array_3d_null_to_zero(N_array_3d * a)
{
    int i = 0, count = 0;

    G_debug(3, "N_convert_array_3d_null_to_zero: convert array of size %i",
	    a->cols_intern * a->rows_intern * a->depths_intern);

    if (a->type == FCELL_TYPE)
	for (i = 0; i < a->cols_intern * a->rows_intern * a->depths_intern;
	     i++) {
	    if (Rast3d_is_null_value_num((void *)&(a->fcell_array[i]), FCELL_TYPE)) {
		a->fcell_array[i] = 0.0;
		count++;
	    }
	}

    if (a->type == DCELL_TYPE)
	for (i = 0; i < a->cols_intern * a->rows_intern * a->depths_intern;
	     i++) {
	    if (Rast3d_is_null_value_num((void *)&(a->dcell_array[i]), DCELL_TYPE)) {
		a->dcell_array[i] = 0.0;
		count++;
	    }
	}


    if (a->type == FCELL_TYPE)
	G_debug(3,
		"N_convert_array_3d_null_to_zero: %i values of type FCELL_TYPE are converted",
		count);

    if (a->type == DCELL_TYPE)
	G_debug(3,
		"N_convert_array_3d_null_to_zero: %i values of type DCELL_TYPE are converted",
		count);

    return count;
}
