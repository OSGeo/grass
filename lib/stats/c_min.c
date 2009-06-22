#include <grass/gis.h>
#include <grass/raster.h>

void c_min(DCELL * result, DCELL * values, int n, const void *closure)
{
    DCELL min;
    int i;

    Rast_set_d_null_value(&min, 1);

    for (i = 0; i < n; i++) {
	if (Rast_is_d_null_value(&values[i]))
	    continue;

	if (Rast_is_d_null_value(&min) || min > values[i])
	    min = values[i];
    }

    if (Rast_is_d_null_value(&min))
	Rast_set_d_null_value(result, 1);
    else
	*result = min;
}

void w_min(DCELL * result, DCELL(*values)[2], int n, const void *closure)
{
    DCELL min;
    int i;

    Rast_set_d_null_value(&min, 1);

    for (i = 0; i < n; i++) {
	if (Rast_is_d_null_value(&values[i][0]))
	    continue;

	if (Rast_is_d_null_value(&min) || min > values[i][0])
	    min = values[i][0];
    }

    if (Rast_is_d_null_value(&min))
	Rast_set_d_null_value(result, 1);
    else
	*result = min;
}
