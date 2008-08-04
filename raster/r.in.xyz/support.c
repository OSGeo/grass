/*
 * r.in.xyz support fns.
 *   Copyright 2006 by M. Hamish Bowman, and The GRASS Development Team
 *   Author: M. Hamish Bowman, University of Otago, Dunedin, New Zealand
 *
 *   This program is free software licensed under the GPL (>=v2).
 *   Read the COPYING file that comes with GRASS for details.
 *
 */

#include <grass/gis.h>
#include "local_proto.h"

static void *get_cell_ptr(void *array, int cols, int row, int col,
			  RASTER_MAP_TYPE map_type)
{
    return G_incr_void_ptr(array,
			   ((row * (size_t) cols) +
			    col) * G_raster_size(map_type));
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
	/* simpler to use G_raster_cpy() or similar ?? */

	for (row = 0; row < nrows; row++) {
	    for (col = 0; col < ncols; col++) {
		G_set_raster_value_c(ptr, 0, map_type);
		ptr = G_incr_void_ptr(ptr, G_raster_size(map_type));
	    }
	}
	break;

    case -1:
	/* fill with NULL */
	/* alloc for col+1, do we come up (nrows) short? no. */
	G_set_null_value(array, nrows * ncols, map_type);
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

    old_n = G_get_raster_value_c(ptr, CELL_TYPE);
    G_set_raster_value_c(ptr, (1 + old_n), CELL_TYPE);

    return 0;
}


int update_min(void *array, int cols, int row, int col,
	       RASTER_MAP_TYPE map_type, double value)
{
    void *ptr = get_cell_ptr(array, cols, row, col, map_type);
    DCELL old_val;

    if (G_is_null_value(ptr, map_type))
	G_set_raster_value_d(ptr, (DCELL) value, map_type);
    else {
	old_val = G_get_raster_value_d(ptr, map_type);
	if (value < old_val)
	    G_set_raster_value_d(ptr, (DCELL) value, map_type);
    }
    return 0;
}


int update_max(void *array, int cols, int row, int col,
	       RASTER_MAP_TYPE map_type, double value)
{
    void *ptr = get_cell_ptr(array, cols, row, col, map_type);
    DCELL old_val;

    if (G_is_null_value(ptr, map_type))
	G_set_raster_value_d(ptr, (DCELL) value, map_type);
    else {
	old_val = G_get_raster_value_d(ptr, map_type);
	if (value > old_val)
	    G_set_raster_value_d(ptr, (DCELL) value, map_type);
    }

    return 0;
}


int update_sum(void *array, int cols, int row, int col,
	       RASTER_MAP_TYPE map_type, double value)
{
    void *ptr = get_cell_ptr(array, cols, row, col, map_type);
    DCELL old_val;

    old_val = G_get_raster_value_d(ptr, map_type);
    G_set_raster_value_d(ptr, value + old_val, map_type);

    return 0;
}


int update_sumsq(void *array, int cols, int row, int col,
		 RASTER_MAP_TYPE map_type, double value)
{
    void *ptr = get_cell_ptr(array, cols, row, col, map_type);
    DCELL old_val;

    old_val = G_get_raster_value_d(ptr, map_type);
    G_set_raster_value_d(ptr, (value * value) + old_val, map_type);

    return 0;
}
