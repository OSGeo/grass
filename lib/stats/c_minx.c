#include <grass/gis.h>
#include <grass/raster.h>

void c_minx(DCELL * result, DCELL * values, int n, const void *closure)
{
    DCELL min, minx;
    int i;

    Rast_set_d_null_value(&min, 1);
    Rast_set_d_null_value(&minx, 1);

    for (i = 0; i < n; i++) {
	if (Rast_is_d_null_value(&values[i]))
	    continue;

	if (Rast_is_d_null_value(&min) || min > values[i]) {
	    min = values[i];
	    minx = i;
	}
    }

    if (Rast_is_d_null_value(&minx))
	Rast_set_d_null_value(result, 1);
    else
	*result = minx;
}
