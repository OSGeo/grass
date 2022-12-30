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
                   Rast_get_d_value(rast, data_type) != 0.0
               ? 0
               : 1;
}

int do_patch(void *result, void *patch, struct Cell_stats *statf, int ncols,
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
             RASTER_MAP_TYPE out_type, size_t out_cell_size, int use_zero,
             int no_support)
=======
             RASTER_MAP_TYPE out_type, size_t out_cell_size, int use_zero)
>>>>>>> 268d757b7d (ci: Ignore paths in CodeQL (#1778))
=======
             RASTER_MAP_TYPE out_type, size_t out_cell_size, int use_zero,
             int no_support)
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
             RASTER_MAP_TYPE out_type, size_t out_cell_size, int use_zero,
             int no_support)
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
{
    int more;

    more = 0;
    while (ncols-- > 0) {
        if (use_zero) { /* use 0 for transparency instead of NULL */
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
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
                    if (out_type == CELL_TYPE && !no_support)
                        Rast_update_cell_stats((CELL *)result, 1, statf);
                }
            } /* ZERO support */
        }
        else { /* use NULL for transparency instead of 0 */
<<<<<<< HEAD
=======
                    if (out_type == CELL_TYPE)
                        Rast_update_cell_stats((CELL *) result, 1, statf);
                }
            }    /* ZERO support */
        } else { /* use NULL for transparency instead of 0 */
>>>>>>> 268d757b7d (ci: Ignore paths in CodeQL (#1778))
=======
                    if (out_type == CELL_TYPE && !no_support)
                        Rast_update_cell_stats((CELL *)result, 1, statf);
                }
            } /* ZERO support */
        }
        else { /* use NULL for transparency instead of 0 */
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))

            if (Rast_is_null_value(result, out_type)) {
                if (Rast_is_null_value(patch, out_type))
                    more = 1;
                else {
                    Rast_raster_cpy(result, patch, 1, out_type);
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
                    if (out_type == CELL_TYPE && !no_support)
                        Rast_update_cell_stats((CELL *)result, 1, statf);
=======
                    if (out_type == CELL_TYPE)
                        Rast_update_cell_stats((CELL *) result, 1, statf);
>>>>>>> 268d757b7d (ci: Ignore paths in CodeQL (#1778))
=======
                    if (out_type == CELL_TYPE && !no_support)
                        Rast_update_cell_stats((CELL *)result, 1, statf);
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
                    if (out_type == CELL_TYPE && !no_support)
                        Rast_update_cell_stats((CELL *)result, 1, statf);
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
                }
            } /* NULL support */
        }
        result = G_incr_void_ptr(result, out_cell_size);
        patch = G_incr_void_ptr(patch, out_cell_size);
    }
    return more;
}
