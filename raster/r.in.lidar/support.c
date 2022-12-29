/*
 * r.in.lidar support fns, from r.in.xyz.
 *   Copyright 2006 by M. Hamish Bowman, and The GRASS Development Team
 *   Author: M. Hamish Bowman, University of Otago, Dunedin, New Zealand
 *
 *   This program is free software licensed under the GPL (>=v2).
 *   Read the COPYING file that comes with GRASS for details.
 *
 */

#include <grass/gis.h>
#include <grass/raster.h>
#include "local_proto.h"

static void *get_cell_ptr(void *array, int cols, int row, int col,
			  RASTER_MAP_TYPE map_type)
{
    return G_incr_void_ptr(array,
			   ((row * (size_t) cols) +
			    col) * Rast_cell_size(map_type));
}

int blank_array(void *array, int nrows, int ncols, RASTER_MAP_TYPE map_type,
		int value)
{
    /* flood fill initialize the array to either 0 or NULL */
    /*  "value" can be either 0 (for 0.0) or -1 (for NULL) */
    int row, col;
    void *ptr;

    ptr = array;

    switch (value) {
    case 0:
	/* fill with 0 */
	/* simpler to use Rast_raster_cpy() or similar ?? */

	for (row = 0; row < nrows; row++) {
	    for (col = 0; col < ncols; col++) {
		Rast_set_c_value(ptr, 0, map_type);
		ptr = G_incr_void_ptr(ptr, Rast_cell_size(map_type));
	    }
	}
	break;

    case -1:
	/* fill with NULL */
	/* alloc for col+1, do we come up (nrows) short? no. */
	Rast_set_null_value(array, nrows * ncols, map_type);
	break;

    default:
	return -1;
    }

    return 0;
}

/* make all these fns return void? */
int update_n(void *array, int cols, int row, int col)
{
    void *ptr = get_cell_ptr(array, cols, row, col, CELL_TYPE);
    CELL old_n;

    old_n = Rast_get_c_value(ptr, CELL_TYPE);
    Rast_set_c_value(ptr, (1 + old_n), CELL_TYPE);

    return 0;
}


int update_min(void *array, int cols, int row, int col,
	       RASTER_MAP_TYPE map_type, double value)
{
    void *ptr = get_cell_ptr(array, cols, row, col, map_type);
    DCELL old_val;

    if (Rast_is_null_value(ptr, map_type))
	Rast_set_d_value(ptr, (DCELL) value, map_type);
    else {
	old_val = Rast_get_d_value(ptr, map_type);
	if (value < old_val)
	    Rast_set_d_value(ptr, (DCELL) value, map_type);
    }
    return 0;
}


int update_max(void *array, int cols, int row, int col,
	       RASTER_MAP_TYPE map_type, double value)
{
    void *ptr = get_cell_ptr(array, cols, row, col, map_type);
    DCELL old_val;

    if (Rast_is_null_value(ptr, map_type))
	Rast_set_d_value(ptr, (DCELL) value, map_type);
    else {
	old_val = Rast_get_d_value(ptr, map_type);
	if (value > old_val)
	    Rast_set_d_value(ptr, (DCELL) value, map_type);
    }

    return 0;
}


int update_sum(void *array, int cols, int row, int col,
	       RASTER_MAP_TYPE map_type, double value)
{
    void *ptr = get_cell_ptr(array, cols, row, col, map_type);
    DCELL old_val;

    old_val = Rast_get_d_value(ptr, map_type);
    Rast_set_d_value(ptr, value + old_val, map_type);

    return 0;
}


int update_sumsq(void *array, int cols, int row, int col,
		 RASTER_MAP_TYPE map_type, double value)
{
    void *ptr = get_cell_ptr(array, cols, row, col, map_type);
    DCELL old_val;

    old_val = Rast_get_d_value(ptr, map_type);
    Rast_set_d_value(ptr, (value * value) + old_val, map_type);

    return 0;
}

/* 0 on NULL, 1 on success */
int row_array_get_value_row_col(void *array, int arr_row, int arr_col,
                                int cols, RASTER_MAP_TYPE rtype, double *value)
{
    void *ptr = array;
    ptr =
        G_incr_void_ptr(ptr,
                        (((size_t) arr_row * cols) +
                         arr_col) * Rast_cell_size(rtype));
    if (Rast_is_null_value(ptr, rtype))
        return 0;
    if (rtype == DCELL_TYPE)
        *value = (double) *(DCELL *) ptr;
    else if (rtype == FCELL_TYPE)
        *value = (double) *(FCELL *) ptr;
    else
        *value = (double) *(CELL *) ptr;
    return 1;
}
