#include <math.h>

#include <grass/gis.h>
#include <grass/raster.h>

void c_thresh(DCELL * result, DCELL * values, int n, const void *closure)
{
    DCELL thresh, threshx;
    double tval = *(const double *)closure;
    double epsilon = GRASS_EPSILON;
    int i;

    Rast_set_d_null_value(&thresh, 1);
    Rast_set_d_null_value(&threshx, 1);

    for (i = 0; i < n; i++) {
	/* already found */
	if (! Rast_is_d_null_value(&threshx))
	    continue;

	if (Rast_is_d_null_value(&values[i]))
	    continue;

	G_debug(2, "values[%d] %f, tval %f", i, values[i], tval);
	/* for GDD */
	epsilon = 10.;
	if (fabs(tval - values[i]) < epsilon ) {
	    thresh = values[i];
	    threshx = i + 1;
	    G_debug(2, "values[%d] %f, thresh %f, threshx %f, diff %f", i, values[i], thresh, threshx, tval - values[i]);
	}
    }

    if (Rast_is_d_null_value(&threshx))
	Rast_set_d_null_value(result, 1);
    else
	*result = threshx;
}
