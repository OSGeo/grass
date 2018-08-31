#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/stats.h>

void c_median(DCELL * result, DCELL * values, int n, const void *closure)
{
    n = sort_cell(values, n);

    if (n < 1)
	Rast_set_d_null_value(result, 1);
    else
	*result = (values[(n - 1) / 2] + values[n / 2]) / 2;
}

void w_median(DCELL * result, DCELL(*values)[2], int n, const void *closure)
{
    DCELL total;
    int i;
    DCELL k;

    n = sort_cell_w(values, n);

    if (n < 1) {
	Rast_set_d_null_value(result, 1);
	return;
    }

    total = 0.0;
    for (i = 0; i < n; i++)
	total += values[i][1];

    k = 0.0;
    for (i = 0; i < n; i++) {
	k += values[i][1];
	if (k >= total / 2)
	    break;
    }

    *result = values[i][0];
}
