#include <grass/gis.h>
#include <grass/raster.h>
/*
 * patch in non-zero data over zero data
 * keep track of the categories which are patched in
 * for later use in constructing the new category and color files
 *
 * returns: 1 the result still contains nulls
 *          0 the result contains no zero nulls
 */

int is_zero_value(void *rast, RASTER_MAP_TYPE data_type)
{

    /* insert 0 check here */

    return Rast_is_null_value(rast, data_type) ||
	Rast_get_d_value(rast, data_type) != 0.0 ? 0 : 1;
}


int do_patch(void *result, void *patch,
	     struct Cell_stats *statf, int ncols,
             RASTER_MAP_TYPE out_type, size_t out_cell_size, int use_zero)
{
    int more;

    more = 0;
    while (ncols-- > 0) {
	if (use_zero) {		/* use 0 for transparency instead of NULL */
	    if (is_zero_value(result, out_type) ||
		Rast_is_null_value(result, out_type)) {
		/* Don't patch hole with a null, just mark as more */
		if (Rast_is_null_value(patch, out_type))
		    more = 1;
		else {
		    /* Mark that there is more to be done if we patch with 0 */
		    if (is_zero_value(patch, out_type))
			more = 1;
		    Rast_raster_cpy(result, patch, 1, out_type);
		    if (out_type == CELL_TYPE)
			Rast_update_cell_stats((CELL *) result, 1, statf);
		}
	    }			/* ZERO support */
	}

	else {			/* use NULL for transparency instead of 0 */

	    if (Rast_is_null_value(result, out_type)) {
		if (Rast_is_null_value(patch, out_type))
		    more = 1;
		else {
		    Rast_raster_cpy(result, patch, 1, out_type);
		    if (out_type == CELL_TYPE)
			Rast_update_cell_stats((CELL *) result, 1, statf);
		}
	    }			/* NULL support */
	}
        result = G_incr_void_ptr(result, out_cell_size);
        patch = G_incr_void_ptr(patch, out_cell_size);
    }
    return more;
}
