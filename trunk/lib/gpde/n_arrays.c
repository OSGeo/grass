
/*****************************************************************************
*
* MODULE:       Grass PDE Numerical Library
* AUTHOR(S):    Soeren Gebbert, Berlin (GER) Dec 2006
* 		soerengebbert <at> gmx <dot> de
*
* PURPOSE:     	Array management functions
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
 * \brief Allocate memory for a N_array_2d data structure.
 *
 * This function allocates memory for an array of type N_array_2d
 * and returns the pointer to the new allocated memory.
 * <br><br>
 * The data type of this array is set by "type" and must be
 * CELL_TYPE, FCELL_TYPE or DCELL_TYPE accordingly to the raster map data types.
 * The offset sets the number of boundary cols and rows.
 * This option is useful to generate homogeneous Neumann boundary conditions around
 * an array or to establish overlapping boundaries. The array is initialized with 0 by default.
 * <br><br>
 * If the offset is greater then 0, negative indices are possible.
 * <br><br>
 *
 * The data structure of a array with 3 rows and cols and an offset of 1
 * will looks like this:
 * <br><br>
 *
 \verbatim
 0 0 0 0 0
 0 0 1 2 0
 0 3 4 5 0
 0 6 7 8 0
 0 0 0 0 0
 \endverbatim
 *
 * 0 is the boundary.
 * <br><br>
 * Internal a one dimensional array is allocated to save memory and to speed up the memory access.
 * To access the one dimensional array with a two dimensional index use the provided
 * get and put functions. The internal representation of the above data will look like this:
 *
 \verbatim
 0 0 0 0 0 0 0 1 2 0 0 3 4 5 0 0 6 7 8 0 0 0 0 0 0
 \endverbatim
 *
 * \param cols int
 * \param rows int
 * \param offset int
 * \param type int
 * \return N_array_2d *
 *
 * */
N_array_2d *N_alloc_array_2d(int cols, int rows, int offset, int type)
{
    N_array_2d *data = NULL;

    if (rows < 1 || cols < 1)
	G_fatal_error("N_alloc_array_2d: cols and rows should be > 0");

    if (type != CELL_TYPE && type != FCELL_TYPE && type != DCELL_TYPE)
	G_fatal_error
	    ("N_alloc_array_2d: Wrong data type, should be CELL_TYPE, FCELL_TYPE or DCELL_TYPE");

    data = (N_array_2d *) G_calloc(1, sizeof(N_array_2d));

    data->cols = cols;
    data->rows = rows;
    data->type = type;
    data->offset = offset;
    data->rows_intern = rows + 2 * offset;	/*offset position at booth sides */
    data->cols_intern = cols + 2 * offset;	/*offset position at booth sides */
    data->cell_array = NULL;
    data->fcell_array = NULL;
    data->dcell_array = NULL;

    if (data->type == CELL_TYPE) {
	data->cell_array =
	    (CELL *) G_calloc((size_t) data->rows_intern * data->cols_intern,
			      sizeof(CELL));
	G_debug(3,
		"N_alloc_array_2d: CELL array allocated rows_intern %i cols_intern %i offset %i",
		data->rows_intern, data->cols_intern, data->offset = offset);
    }
    else if (data->type == FCELL_TYPE) {
	data->fcell_array =
	    (FCELL *) G_calloc((size_t) data->rows_intern * data->cols_intern,
			       sizeof(FCELL));
	G_debug(3,
		"N_alloc_array_2d: FCELL array allocated rows_intern %i cols_intern %i offset %i",
		data->rows_intern, data->cols_intern, data->offset = offset);

    }
    else if (data->type == DCELL_TYPE) {
	data->dcell_array =
	    (DCELL *) G_calloc((size_t) data->rows_intern * data->cols_intern,
			       sizeof(DCELL));
	G_debug(3,
		"N_alloc_array_2d: DCELL array allocated rows_intern %i cols_intern %i offset %i",
		data->rows_intern, data->cols_intern, data->offset = offset);
    }

    return data;
}

/*!
 * \brief Release the memory of a N_array_2d structure
 *
 * \param data N_array_2d *
 * \return void
 * */
void N_free_array_2d(N_array_2d * data)
{

    if (data != NULL) {
	G_debug(3, "N_free_array_2d: free N_array_2d");

	if (data->type == CELL_TYPE && data->cell_array != NULL) {
	    G_free(data->cell_array);
	}
	else if (data->type == FCELL_TYPE && data->fcell_array != NULL) {
	    G_free(data->fcell_array);

	}
	else if (data->type == DCELL_TYPE && data->dcell_array != NULL) {
	    G_free(data->dcell_array);
	}

	G_free(data);
	data = NULL;

    }

    return;
}


/*!
 * \brief Return the data type of the N_array_2d struct
 *
 * The data type can be CELL_TYPE, FCELL_TYPE or DCELL_TYPE accordingly to the raster map data types.
 *
 * \param array N_array_2d *
 * \return type int
 * */
int N_get_array_2d_type(N_array_2d * array)
{
    return array->type;
}

/*!
 * \brief Write the value of the N_array_2d struct at position col, row to value
 *
 * The value must be of the same type as the array. Otherwise you will risk data losses.
 *
 * \param data N_array_2d *
 * \param col int
 * \param row int
 * \param value void * - this variable contains the array value at col, row position
 * \return void
 * */

void N_get_array_2d_value(N_array_2d * data, int col, int row, void *value)
{

    if (data->offset == 0) {
	if (data->type == CELL_TYPE && data->cell_array != NULL) {
	    *((CELL *) value) =
		data->cell_array[row * data->cols_intern + col];
	}
	else if (data->type == FCELL_TYPE && data->fcell_array != NULL) {
	    *((FCELL *) value) =
		data->fcell_array[row * data->cols_intern + col];
	}
	else if (data->type == DCELL_TYPE && data->dcell_array != NULL) {
	    *((DCELL *) value) =
		data->dcell_array[row * data->cols_intern + col];
	}
    }
    else {
	if (data->type == CELL_TYPE && data->cell_array != NULL) {
	    *((CELL *) value) =
		data->cell_array[(row + data->offset) * data->cols_intern +
				 col + data->offset];
	}
	else if (data->type == FCELL_TYPE && data->fcell_array != NULL) {
	    *((FCELL *) value) =
		data->fcell_array[(row + data->offset) * data->cols_intern +
				  col + data->offset];
	}
	else if (data->type == DCELL_TYPE && data->dcell_array != NULL) {
	    *((DCELL *) value) =
		data->dcell_array[(row + data->offset) * data->cols_intern +
				  col + data->offset];
	}
    }

    return;
}

/*!
 * \brief Returns 1 if the value of N_array_2d struct at position col, row
 * is of type null, otherwise 0
 *
 * This function checks automatically the type of the array and checks for the
 * data type null value.
 *
 * \param data N_array_2d *
 * \param col int
 * \param row int
 * \return int - 1 = is null, 0 otherwise
 * */
int N_is_array_2d_value_null(N_array_2d * data, int col, int row)
{

    if (data->offset == 0) {
	if (data->type == CELL_TYPE && data->cell_array != NULL) {
	    G_debug(6,
		    "N_is_array_2d_value_null: null value is of type CELL at pos [%i][%i]",
		    col, row);
	    return Rast_is_null_value((void *)
				   &(data->
				     cell_array[row * data->cols_intern +
						col]), CELL_TYPE);
	}
	else if (data->type == FCELL_TYPE && data->fcell_array != NULL) {
	    G_debug(6,
		    "N_is_array_2d_value_null: null value is of type FCELL at pos [%i][%i]",
		    col, row);
	    return Rast_is_null_value((void *)
				   &(data->
				     fcell_array[row * data->cols_intern +
						 col]), FCELL_TYPE);
	}
	else if (data->type == DCELL_TYPE && data->dcell_array != NULL) {
	    G_debug(6,
		    "N_is_array_2d_value_null: null value is of type DCELL at pos [%i][%i]",
		    col, row);
	    return Rast_is_null_value((void *)
				   &(data->
				     dcell_array[row * data->cols_intern +
						 col]), DCELL_TYPE);
	}
    }
    else {
	if (data->type == CELL_TYPE && data->cell_array != NULL) {
	    G_debug(6,
		    "N_is_array_2d_value_null: null value is of type CELL at pos [%i][%i]",
		    col, row);
	    return Rast_is_null_value((void *)
				   &(data->
				     cell_array[(row +
						 data->offset) *
						data->cols_intern + col +
						data->offset]), CELL_TYPE);
	}
	else if (data->type == FCELL_TYPE && data->fcell_array != NULL) {
	    G_debug(6,
		    "N_is_array_2d_value_null: null value is of type FCELL at pos [%i][%i]",
		    col, row);
	    return Rast_is_null_value((void *)
				   &(data->
				     fcell_array[(row +
						  data->offset) *
						 data->cols_intern + col +
						 data->offset]), FCELL_TYPE);
	}
	else if (data->type == DCELL_TYPE && data->dcell_array != NULL) {
	    G_debug(6,
		    "N_is_array_2d_value_null: null value is of type DCELL at pos [%i][%i]",
		    col, row);
	    return Rast_is_null_value((void *)
				   &(data->
				     dcell_array[(row +
						  data->offset) *
						 data->cols_intern + col +
						 data->offset]), DCELL_TYPE);
	}
    }

    return 0;
}


/*!
 * \brief Returns the value of type CELL at position col, row
 *
 * The data array can be of type CELL, FCELL or DCELL, the value will be casted to the CELL type.
 *
 * \param data N_array_2d *
 * \param col int
 * \param row int
 * \return CELL
 *
 * */
CELL N_get_array_2d_c_value(N_array_2d * data, int col, int row)
{
    CELL value = 0;
    FCELL fvalue = 0.0;
    DCELL dvalue = 0.0;

    switch (data->type) {
    case CELL_TYPE:
	N_get_array_2d_value(data, col, row, (void *)&value);
	return (CELL) value;
    case FCELL_TYPE:
	N_get_array_2d_value(data, col, row, (void *)&fvalue);
	return (CELL) fvalue;
    case DCELL_TYPE:
	N_get_array_2d_value(data, col, row, (void *)&dvalue);
	return (CELL) dvalue;
    }

    return value;
}

/*!
 * \brief Returns the value of type FCELL at position col, row
 *
 * The data array can be of type CELL, FCELL or DCELL, the value will be casted to the FCELL type.
 *
 * \param data N_array_2d *
 * \param col int
 * \param row int
 * \return FCELL

 * */
FCELL N_get_array_2d_f_value(N_array_2d * data, int col, int row)
{
    CELL value = 0;
    FCELL fvalue = 0.0;
    DCELL dvalue = 0.0;

    switch (data->type) {
    case CELL_TYPE:
	N_get_array_2d_value(data, col, row, (void *)&value);
	return (FCELL) value;
    case FCELL_TYPE:
	N_get_array_2d_value(data, col, row, (void *)&fvalue);
	return (FCELL) fvalue;
    case DCELL_TYPE:
	N_get_array_2d_value(data, col, row, (void *)&dvalue);
	return (FCELL) dvalue;
    }

    return fvalue;
}

/*!
 * \brief Returns the value of type DCELL at position col, row
 *
 * The data array can be of type CELL, FCELL or DCELL, the value will be casted to the DCELL type.
 *
 * \param data N_array_2d *
 * \param col int
 * \param row int
 * \return DCELL
 *
 * */
DCELL N_get_array_2d_d_value(N_array_2d * data, int col, int row)
{
    CELL value = 0;
    FCELL fvalue = 0.0;
    DCELL dvalue = 0.0;

    switch (data->type) {
    case CELL_TYPE:
	N_get_array_2d_value(data, col, row, (void *)&value);
	return (DCELL) value;
    case FCELL_TYPE:
	N_get_array_2d_value(data, col, row, (void *)&fvalue);
	return (DCELL) fvalue;
    case DCELL_TYPE:
	N_get_array_2d_value(data, col, row, (void *)&dvalue);
	return (DCELL) dvalue;
    }

    return dvalue;

}

/*!
 * \brief Writes a value to the N_array_2d struct at position col, row
 *
 * The value will be automatically cast to the array type.
 *
 * \param data N_array_2d *
 * \param col int
 * \param row int
 * \param value char *
 * \return void
 * */
void N_put_array_2d_value(N_array_2d * data, int col, int row, char *value)
{

    G_debug(6, "N_put_array_2d_value: put value to array");

    if (data->offset == 0) {
	if (data->type == CELL_TYPE && data->cell_array != NULL) {
	    data->cell_array[row * data->cols_intern + col] =
		*((CELL *) value);
	}
	else if (data->type == FCELL_TYPE && data->fcell_array != NULL) {
	    data->fcell_array[row * data->cols_intern + col] =
		*((FCELL *) value);
	}
	else if (data->type == DCELL_TYPE && data->dcell_array != NULL) {
	    data->dcell_array[row * data->cols_intern + col] =
		*((DCELL *) value);
	}
    }
    else {
	if (data->type == CELL_TYPE && data->cell_array != NULL) {
	    data->cell_array[(row + data->offset) * data->cols_intern + col +
			     data->offset] = *((CELL *) value);
	}
	else if (data->type == FCELL_TYPE && data->fcell_array != NULL) {
	    data->fcell_array[(row + data->offset) * data->cols_intern + col +
			      data->offset] = *((FCELL *) value);
	}
	else if (data->type == DCELL_TYPE && data->dcell_array != NULL) {
	    data->dcell_array[(row + data->offset) * data->cols_intern + col +
			      data->offset] = *((DCELL *) value);
	}
    }

    return;
}

/*!
 * \brief Writes the null value to the N_array_2d struct at position col, row
 *
 * The null value will be automatically set to the array data type (CELL, FCELL or DCELL).
 *
 * \param data N_array_2d *
 * \param col int
 * \param row int
 * \return void
 * */
void N_put_array_2d_value_null(N_array_2d * data, int col, int row)
{

    G_debug(6,
	    "N_put_array_2d_value_null: put null value to array pos [%i][%i]",
	    col, row);

    if (data->offset == 0) {
	if (data->type == CELL_TYPE && data->cell_array != NULL) {
	    Rast_set_c_null_value((void *)
			       &(data->
				 cell_array[row * data->cols_intern + col]),
			       1);
	}
	else if (data->type == FCELL_TYPE && data->fcell_array != NULL) {
	    Rast_set_f_null_value((void *)
			       &(data->
				 fcell_array[row * data->cols_intern + col]),
			       1);
	}
	else if (data->type == DCELL_TYPE && data->dcell_array != NULL) {
	    Rast_set_d_null_value((void *)
			       &(data->
				 dcell_array[row * data->cols_intern + col]),
			       1);
	}
    }
    else {
	if (data->type == CELL_TYPE && data->cell_array != NULL) {
	    Rast_set_c_null_value((void *)
			       &(data->
				 cell_array[(row +
					     data->offset) *
					    data->cols_intern + col +
					    data->offset]), 1);
	}
	else if (data->type == FCELL_TYPE && data->fcell_array != NULL) {
	    Rast_set_f_null_value((void *)
			       &(data->
				 fcell_array[(row +
					      data->offset) *
					     data->cols_intern + col +
					     data->offset]), 1);
	}
	else if (data->type == DCELL_TYPE && data->dcell_array != NULL) {
	    Rast_set_d_null_value((void *)
			       &(data->
				 dcell_array[(row +
					      data->offset) *
					     data->cols_intern + col +
					     data->offset]), 1);
	}
    }

    return;
}

/*!
 * \brief Writes a CELL value to the N_array_2d struct at position col, row
 *
 * \param data N_array_2d *
 * \param col int
 * \param row int
 * \param value CELL
 * \return void
 * */
void N_put_array_2d_c_value(N_array_2d * data, int col, int row, CELL value)
{
    FCELL fvalue;
    DCELL dvalue;

    switch (data->type) {
    case FCELL_TYPE:
	fvalue = (FCELL) value;
	N_put_array_2d_value(data, col, row, (char *)&fvalue);
	return;
    case DCELL_TYPE:
	dvalue = (DCELL) value;
	N_put_array_2d_value(data, col, row, (char *)&dvalue);
	return;
    }

    N_put_array_2d_value(data, col, row, (char *)&value);

    return;
}

/*!
 * \brief Writes a FCELL value to the N_array_2d struct at position col, row
 *
 * \param data N_array_2d *
 * \param col int
 * \param row int
 * \param value FCELL
 * \return void
 * */
void N_put_array_2d_f_value(N_array_2d * data, int col, int row, FCELL value)
{
    CELL cvalue;
    DCELL dvalue;

    switch (data->type) {
    case CELL_TYPE:
	cvalue = (CELL) value;
	N_put_array_2d_value(data, col, row, (char *)&cvalue);
	return;
    case DCELL_TYPE:
	dvalue = (DCELL) value;
	N_put_array_2d_value(data, col, row, (char *)&dvalue);
	return;
    }

    N_put_array_2d_value(data, col, row, (char *)&value);

    return;
}

/*!
 * \brief Writes a DCELL value to the N_array_2d struct at position col, row
 *
 * \param data N_array_2d *
 * \param col int
 * \param row int
 * \param value DCELL
 * \return void
 * */
void N_put_array_2d_d_value(N_array_2d * data, int col, int row, DCELL value)
{
    CELL cvalue;
    FCELL fvalue;

    switch (data->type) {
    case CELL_TYPE:
	cvalue = (CELL) value;
	N_put_array_2d_value(data, col, row, (char *)&cvalue);
	return;
    case FCELL_TYPE:
	fvalue = (FCELL) value;
	N_put_array_2d_value(data, col, row, (char *)&fvalue);
	return;
    }

    N_put_array_2d_value(data, col, row, (char *)&value);

    return;
}

/*!
 * \brief This function writes the data info of the array data to stdout
 *
 * \param data N_array_2d *
 * \return void
 * */
void N_print_array_2d_info(N_array_2d * data)
{

    fprintf(stdout, "N_array_2d \n");
    fprintf(stdout, "Cols %i\n", data->cols);
    fprintf(stdout, "Rows: %i\n", data->rows);
    fprintf(stdout, "Array type: %i\n", data->type);
    fprintf(stdout, "Offset: %i\n", data->offset);
    fprintf(stdout, "Internal cols: %i\n", data->cols_intern);
    fprintf(stdout, "Internal rows: %i\n", data->rows_intern);
    fprintf(stdout, "CELL array pointer: %p\n", data->cell_array);
    fprintf(stdout, "FCELL array pointer: %p\n", data->fcell_array);
    fprintf(stdout, "DCELL array pointer: %p\n", data->dcell_array);


    return;
}

/*!
 * \brief Write info and content of the N_array_2d struct to stdout
 *
 * Offsets are ignored
 *
 * \param data N_array_2d *
 * \return void
 * */
void N_print_array_2d(N_array_2d * data)
{
    int i, j;

    N_print_array_2d_info(data);

    for (j = 0 - data->offset; j < data->rows + data->offset; j++) {
	for (i = 0 - data->offset; i < data->cols + data->offset; i++) {
	    if (data->type == CELL_TYPE)
		fprintf(stdout, "%6d ", N_get_array_2d_c_value(data, i, j));
	    else if (data->type == FCELL_TYPE)
		fprintf(stdout, "%6.6f ", N_get_array_2d_f_value(data, i, j));
	    else if (data->type == DCELL_TYPE)
		printf("%6.6f ", N_get_array_2d_d_value(data, i, j));
	}
	fprintf(stdout, "\n");
    }
    fprintf(stdout, "\n");

    return;
}


/* ******************** 3D ARRAY FUNCTIONS *********************** */

/*!
 * \brief Allocate memory for a N_array_3d data structure.
 *
 * This functions allocates an array of type N_array_3d and returns a pointer
 * to the new allocated memory.
 * <br><br>
 * The data type of this array set by "type" must be
 * FCELL_TYPE or DCELL_TYPE accordingly to the raster3d map data types.
 * The offsets sets the number of boundary cols, rows and depths.
 * This option is useful to generate homogeneous Neumann boundary conditions around
 * an array or to establish overlapping boundaries. The arrays are initialized with 0 by default.
 * <br><br>
 * If the offset is greater then 0, negative indices are possible.
 * The data structure of a array with 3 depths, rows and cols and an offset of 1
 * will looks like this:
 *
 \verbatim
 0  0  0  0  0
 0  0  0  0  0
 0  0  0  0  0
 0  0  0  0  0
 0  0  0  0  0

 0  0  0  0  0
 0  0  1  2  0
 0  3  4  5  0
 0  6  7  8  0
 0  0  0  0  0

 0  0  0  0  0
 0  9 10 11  0
 0 12 13 14  0
 0 15 16 17  0
 0  0  0  0  0

 0  0  0  0  0
 0 18 19 20  0
 0 21 22 23  0
 0 24 25 26  0
 0  0  0  0  0

 0  0  0  0  0
 0  0  0  0  0
 0  0  0  0  0
 0  0  0  0  0
 0  0  0  0  0

 \endverbatim

 The depth counts from the bottom to the top.

 * <br><br>
 * Internal a one dimensional array is allocated to speed up the memory access.
 * To access the dimensional array with a three dimensional indexing use the provided
 * get and put functions.
 *
 * \param cols int
 * \param rows int
 * \param depths int
 * \param offset int
 * \param type int
 * \return N_array_3d *
 *
 * */
N_array_3d *N_alloc_array_3d(int cols, int rows, int depths, int offset,
			     int type)
{
    N_array_3d *data = NULL;

    if (rows < 1 || cols < 1 || depths < 1)
	G_fatal_error
	    ("N_alloc_array_3d: depths, cols and rows should be > 0");

    if (type != DCELL_TYPE && type != FCELL_TYPE)
	G_fatal_error
	    ("N_alloc_array_3d: Wrong data type, should be FCELL_TYPE or DCELL_TYPE");

    data = (N_array_3d *) G_calloc(1, sizeof(N_array_3d));

    data->cols = cols;
    data->rows = rows;
    data->depths = depths;
    data->type = type;
    data->offset = offset;
    data->rows_intern = rows + 2 * offset;
    data->cols_intern = cols + 2 * offset;
    data->depths_intern = depths + 2 * offset;
    data->fcell_array = NULL;
    data->dcell_array = NULL;

    if (data->type == FCELL_TYPE) {
	data->fcell_array =
	    (float *)G_calloc((size_t) data->depths_intern * data->rows_intern *
			      data->cols_intern, sizeof(float));
	G_debug(3,
		"N_alloc_array_3d: float array allocated rows_intern %i cols_intern %i depths_intern %i offset %i",
		data->rows_intern, data->cols_intern, data->depths_intern,
		data->offset = offset);
    }
    else if (data->type == DCELL_TYPE) {
	data->dcell_array =
	    (double *)G_calloc((size_t) data->depths_intern * data->rows_intern *
			       data->cols_intern, sizeof(double));
	G_debug(3,
		"N_alloc_array_3d: double array allocated rows_intern %i cols_intern %i depths_intern %i offset %i",
		data->rows_intern, data->cols_intern, data->depths_intern,
		data->offset = offset);
    }

    return data;
}

/*!
 * \brief Release the memory of a N_array_3d
 *
 * \param data N_array_3d *
 * \return void
 * */
void N_free_array_3d(N_array_3d * data)
{

    if (data != NULL) {
	G_debug(3, "N_free_array_3d: free N_array_3d");

	if (data->type == FCELL_TYPE && data->fcell_array != NULL) {
	    G_free(data->fcell_array);
	}
	else if (data->type == DCELL_TYPE && data->dcell_array != NULL) {
	    G_free(data->dcell_array);
	}

	G_free(data);
	data = NULL;

    }

    return;
}

/*!
 * \brief Return the data type of the N_array_3d
 *
 * The data type can be FCELL_TYPE and DCELL_TYPE accordingly to the raster map data types.
 *
 * \param array N_array_3d *
 * \return type int -- FCELL_TYPE or DCELL_TYPE
 * */
int N_get_array_3d_type(N_array_3d * array)
{
    return array->type;
}


/*!
 * \brief This function writes the value of N_array_3d data at position col, row, depth
 *        to the variable value
 *
 * The value must be from the same type as the array. Otherwise you will risk data losses.
 *
 * \param data N_array_3d *
 * \param col int
 * \param row int
 * \param depth int
 * \param value void *
 * \return void
 * */
void
N_get_array_3d_value(N_array_3d * data, int col, int row, int depth,
		     void *value)
{

    if (data->offset == 0) {
	if (data->type == FCELL_TYPE && data->fcell_array != NULL) {
	    *((float *)value) =
		data->fcell_array[depth *
				  (data->rows_intern * data->cols_intern) +
				  row * data->cols_intern + col];
	}
	else if (data->type == DCELL_TYPE && data->dcell_array != NULL) {
	    *((double *)value) =
		data->dcell_array[depth *
				  (data->rows_intern * data->cols_intern) +
				  row * data->cols_intern + col];
	}
    }
    else {
	if (data->type == FCELL_TYPE && data->fcell_array != NULL) {
	    *((float *)value) =
		data->fcell_array[(depth + data->offset) *
				  (data->rows_intern * data->cols_intern) +
				  (row + data->offset) * data->cols_intern +
				  (col + data->offset)];

	}
	else if (data->type == DCELL_TYPE && data->dcell_array != NULL) {
	    *((double *)value) =
		data->dcell_array[(depth + data->offset) *
				  (data->rows_intern * data->cols_intern) +
				  (row + data->offset) * data->cols_intern +
				  (col + data->offset)];
	}
    }

    return;
}

/*!
 * \brief This function returns 1 if value of N_array_3d data at position col, row, depth
 * is of type null, otherwise 0
 *
 * This function checks automatically the type of the array and checks for the
 * data type null value.
 *
 * \param data N_array_3d *
 * \param col int
 * \param row int
 * \param depth int
 * \return void
 * */
int N_is_array_3d_value_null(N_array_3d * data, int col, int row, int depth)
{

    if (data->offset == 0) {
	if (data->type == FCELL_TYPE && data->fcell_array != NULL) {
	    G_debug(6,
		    "N_is_array_3d_value_null: null value is of type DCELL_TYPE at pos [%i][%i][%i]",
		    depth, row, col);
	    return Rast3d_is_null_value_num((void *)
				      &(data->
					fcell_array[depth *
						    (data->rows_intern *
						     data->cols_intern) +
						    row * data->cols_intern +
						    col]), FCELL_TYPE);
	}
	else if (data->type == DCELL_TYPE && data->dcell_array != NULL) {
	    G_debug(6,
		    "N_is_array_3d_value_null: null value is of type DCELL_TYPE at pos [%i][%i][%i]",
		    depth, row, col);
	    return Rast3d_is_null_value_num((void *)
				      &(data->
					dcell_array[depth *
						    (data->rows_intern *
						     data->cols_intern) +
						    row * data->cols_intern +
						    col]), DCELL_TYPE);
	}
    }
    else {
	if (data->type == FCELL_TYPE && data->fcell_array != NULL) {
	    G_debug(6,
		    "N_is_array_3d_value_null: null value is of type DCELL_TYPE at pos [%i][%i][%i]",
		    depth, row, col);
	    return Rast3d_is_null_value_num((void *)
				      &(data->
					fcell_array[(depth +
						     data->offset) *
						    (data->rows_intern *
						     data->cols_intern) +
						    (row + data->offset)
						    * data->cols_intern +
						    (col + data->offset)]),
				      FCELL_TYPE);

	}
	else if (data->type == DCELL_TYPE && data->dcell_array != NULL) {
	    G_debug(6,
		    "N_is_array_3d_value_null: null value is of type DCELL_TYPE at pos [%i][%i][%i]",
		    depth, row, col);
	    return Rast3d_is_null_value_num((void *)
				      &(data->
					dcell_array[(depth +
						     data->offset) *
						    (data->rows_intern *
						     data->cols_intern) +
						    (row +
						     data->offset) *
						    data->cols_intern + (col +
									 data->
									 offset)]),
				      DCELL_TYPE);
	}
    }

    return 0;
}

/*!
 * \brief This function returns the value of type float at position col, row, depth
 *
 * The data type can be FCELL_TYPE or DCELL_TYPE accordingly to the raster map data types.
 *
 * \param data N_array_3d *
 * \param col int
 * \param row int
 * \param depth int
 * \return float
 *
 * */
float N_get_array_3d_f_value(N_array_3d * data, int col, int row, int depth)
{
    float fvalue = 0.0;
    double dvalue = 0.0;

    switch (data->type) {
    case FCELL_TYPE:
	N_get_array_3d_value(data, col, row, depth, (void *)&fvalue);
	return (float)fvalue;
    case DCELL_TYPE:
	N_get_array_3d_value(data, col, row, depth, (void *)&dvalue);
	return (float)dvalue;
    }

    return fvalue;
}

/*!
 * \brief This function returns the value of type float at position col, row, depth
 *
 * The data type can be FCELL_TYPE or DCELL_TYPE accordingly to the raster map data types.
 *
 * \param data N_array_3d *
 * \param col int
 * \param row int
 * \param depth int
 * \return double
 *
 * */
double N_get_array_3d_d_value(N_array_3d * data, int col, int row, int depth)
{
    float fvalue = 0.0;
    double dvalue = 0.0;

    switch (data->type) {

    case FCELL_TYPE:
	N_get_array_3d_value(data, col, row, depth, (void *)&fvalue);
	return (double)fvalue;
    case DCELL_TYPE:
	N_get_array_3d_value(data, col, row, depth, (void *)&dvalue);
	return (double)dvalue;
    }

    return dvalue;
}

/*!
 * \brief This function writes a value to the N_array_3d data at position col, row, depth
 *
 * The value will be automatically cast to the array type.
 *
 * \param data N_array_3d *
 * \param col int
 * \param row int
 * \param depth int
 * \param value cahr *
 * \return void
 * */
void
N_put_array_3d_value(N_array_3d * data, int col, int row, int depth,
		     char *value)
{

    G_debug(6, "N_put_array_3d_value: put value to array at pos [%i][%i][%i]",
	    depth, row, col);

    if (data->offset == 0) {
	if (data->type == FCELL_TYPE && data->fcell_array != NULL) {
	    data->fcell_array[depth *
			      (data->rows_intern * data->cols_intern) +
			      row * data->cols_intern + col]
		= *((float *)value);
	}
	else if (data->type == DCELL_TYPE && data->dcell_array != NULL) {

	    data->dcell_array[depth *
			      (data->rows_intern * data->cols_intern) +
			      row * data->cols_intern + col]
		= *((double *)value);
	}
    }
    else {
	if (data->type == FCELL_TYPE && data->fcell_array != NULL) {
	    data->fcell_array[(depth + data->offset) *
			      (data->rows_intern * data->cols_intern) + (row +
									 data->
									 offset)
			      * data->cols_intern + (col + data->offset)] =
		*((float *)value);
	}
	else if (data->type == DCELL_TYPE && data->dcell_array != NULL) {
	    data->dcell_array[(depth + data->offset) *
			      (data->rows_intern * data->cols_intern) + (row +
									 data->
									 offset)
			      * data->cols_intern + (col + data->offset)] =
		*((double *)value);
	}
    }

    return;
}

/*!
 * \brief This function writes a null value to the N_array_3d data at position col, row, depth
 *
 * The null value will be automatically set to the array type.
 *
 * \param data N_array_3d *
 * \param col int
 * \param row int
 * \param depth int
 * \return void
 * */
void N_put_array_3d_value_null(N_array_3d * data, int col, int row, int depth)
{

    G_debug(6,
	    "N_put_array_3d_value_null: put null value to array at pos [%i][%i][%i]",
	    depth, row, col);

    if (data->offset == 0) {
	if (data->type == FCELL_TYPE && data->fcell_array != NULL) {
	    Rast3d_set_null_value((void *)
			     &(data->
			       fcell_array[depth *
					   (data->rows_intern *
					    data->cols_intern) +
					   row * data->cols_intern + col]), 1,
			     FCELL_TYPE);
	}
	else if (data->type == DCELL_TYPE && data->dcell_array != NULL) {
	    Rast3d_set_null_value((void *)
			     &(data->
			       dcell_array[depth *
					   (data->rows_intern *
					    data->cols_intern) +
					   row * data->cols_intern + col]), 1,
			     DCELL_TYPE);
	}
    }
    else {
	if (data->type == FCELL_TYPE && data->fcell_array != NULL) {
	    Rast3d_set_null_value((void *)
			     &(data->
			       fcell_array[(depth +
					    data->offset) *
					   (data->rows_intern *
					    data->cols_intern) + (row +
								  data->
								  offset) *
					   data->cols_intern + (col +
								data->
								offset)]), 1,
			     FCELL_TYPE);
	}
	else if (data->type == DCELL_TYPE && data->dcell_array != NULL) {
	    Rast3d_set_null_value((void *)
			     &(data->
			       dcell_array[(depth +
					    data->offset) *
					   (data->rows_intern *
					    data->cols_intern) + (row +
								  data->
								  offset) *
					   data->cols_intern + (col +
								data->
								offset)]), 1,
			     DCELL_TYPE);
	}
    }

    return;
}

/*!
 * \brief This function writes a float value to the N_array_3d data at position col, row, depth
 *
 * \param data N_array_3d *
 * \param col int
 * \param row int
 * \param depth int
 * \param value float
 * \return void
 * */
void
N_put_array_3d_f_value(N_array_3d * data, int col, int row, int depth,
		       float value)
{
    double dval;

    if (data->type == DCELL_TYPE) {
	dval = (double)value;
	N_put_array_3d_value(data, col, row, depth, (void *)&dval);
    }
    else {
	N_put_array_3d_value(data, col, row, depth, (void *)&value);
    }

    return;
}

/*!
 * \brief Writes a double value to the N_array_3d struct at position col, row, depth
 *
 * \param data N_array_3d *
 * \param col int
 * \param row int
 * \param depth int
 * \param value double
 * \return void
 * */
void
N_put_array_3d_d_value(N_array_3d * data, int col, int row, int depth,
		       double value)
{
    float fval;

    if (data->type == FCELL_TYPE) {
	fval = (double)value;
	N_put_array_3d_value(data, col, row, depth, (void *)&fval);
    }
    else {
	N_put_array_3d_value(data, col, row, depth, (void *)&value);
    }

    return;
}

/*!
 * \brief Write the info of the array to stdout
 *
 * \param data N_array_3d *
 * \return void
 * */
void N_print_array_3d_info(N_array_3d * data)
{

    fprintf(stdout, "N_array_3d \n");
    fprintf(stdout, "Cols %i\n", data->cols);
    fprintf(stdout, "Rows: %i\n", data->rows);
    fprintf(stdout, "Depths: %i\n", data->depths);
    fprintf(stdout, "Array type: %i\n", data->type);
    fprintf(stdout, "Offset: %i\n", data->offset);
    fprintf(stdout, "Internal cols: %i\n", data->cols_intern);
    fprintf(stdout, "Internal rows: %i\n", data->rows_intern);
    fprintf(stdout, "Internal depths: %i\n", data->depths_intern);
    fprintf(stdout, "FCELL array pointer: %p\n", data->fcell_array);
    fprintf(stdout, "DCELL array pointer: %p\n", data->dcell_array);

    return;
}

/*!
 * \brief Write info and content of the array data to stdout
 *
 * Offsets are ignored
 *
 * \param data N_array_2d *
 * \return void
 * */
void N_print_array_3d(N_array_3d * data)
{
    int i, j, k;

    N_print_array_3d_info(data);

    for (k = 0; k < data->depths; k++) {
	for (j = 0; j < data->rows; j++) {
	    for (i = 0; i < data->cols; i++) {
		if (data->type == FCELL_TYPE)
		    printf("%6.6f ", N_get_array_3d_f_value(data, i, j, k));
		else if (data->type == DCELL_TYPE)
		    printf("%6.6f ", N_get_array_3d_d_value(data, i, j, k));
	    }
	    printf("\n");
	}
	printf("\n");
    }
    printf("\n");

    return;
}
