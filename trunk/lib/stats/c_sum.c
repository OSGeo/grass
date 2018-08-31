#include <grass/gis.h>
#include <grass/raster.h>

void c_sum(DCELL * result, DCELL * values, int n, const void *closure)
{
    DCELL sum;
    int count;
    int i;

    sum = 0.0;
    count = 0;

    for (i = 0; i < n; i++) {
	if (Rast_is_d_null_value(&values[i]))
	    continue;

	sum += values[i];
	count++;
    }

    if (count == 0)
	Rast_set_d_null_value(result, 1);
    else
	*result = sum;
}

void w_sum(DCELL * result, DCELL(*values)[2], int n, const void *closure)
{
    DCELL sum;
    DCELL count;
    int i;

    sum = 0.0;
    count = 0.0;

    for (i = 0; i < n; i++) {
	if (Rast_is_d_null_value(&values[i][0]))
	    continue;

	sum += values[i][0] * values[i][1];
	count += values[i][1];
    }

    if (count == 0)
	Rast_set_d_null_value(result, 1);
    else
	*result = sum;
}
