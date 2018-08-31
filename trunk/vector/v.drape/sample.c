#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/vector.h>
#include <grass/glocale.h>

/* Samples raster map */
int sample_raster(int fdrast, const struct Cell_head *window,
		  struct line_pnts *Points,
                  INTERP_TYPE method, double scale,
                  int null_defined, double null_val)
{
    double estimated_elevation;
    int j;

    /* loop through each point in a line */
    for (j = 0; j < Points->n_points; j++) {
        /* sample raster at this point, and update the z-coordinate
         * (note that input vector should not be 3D!) */
        estimated_elevation = scale * Rast_get_sample(fdrast, window, NULL,
                                                      Points->y[j], Points->x[j],
                                                      0, method);
        
        if (Rast_is_d_null_value(&estimated_elevation)) {
            if (null_defined)
                estimated_elevation = null_val;
            else
                return 0;
        }
        
        /* update the elevation value for each data point */
        Points->z[j] = estimated_elevation;
    }
 
   return 1;
}
