#include <grass/gis.h>
#include <grass/raster.h>

void c_count(DCELL * result, DCELL * values, int n, const void *closure)
{
    int count;
    int i;

    count = 0;

    for (i = 0; i < n; i++)
	if (!Rast_is_d_null_value(&values[i]))
	    count++;

    *result = count;
}

void w_count(DCELL * result, DCELL(*values)[2], int n, const void *closure)
{
    DCELL count;
    int i;

    count = 0.0;

    for (i = 0; i < n; i++)
	if (!Rast_is_d_null_value(&values[i][0]))
	    count += values[i][1];

    *result = count;
}
