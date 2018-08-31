#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/stats.h>

void c_mode(DCELL * result, DCELL * values, int n, const void *closure)
{
    DCELL mode;
    int max;
    DCELL prev;
    int count;
    int i;

    n = sort_cell(values, n);

    max = 0;
    count = 0;

    for (i = 0; i < n; i++) {
	if (max == 0 || values[i] != prev) {
	    prev = values[i];
	    count = 0;
	}

	count++;

	if (count > max) {
	    max = count;
	    mode = prev;
	}
    }

    if (max == 0)
	Rast_set_d_null_value(result, 1);
    else
	*result = mode;
}

void w_mode(DCELL * result, DCELL(*values)[2], int n, const void *closure)
{
    DCELL mode;
    DCELL max;
    DCELL prev;
    DCELL count;
    int i;

    n = sort_cell_w(values, n);

    max = 0.0;
    count = 0.0;

    for (i = 0; i < n; i++) {
	if (max == 0.0 || values[i][0] != prev) {
	    prev = values[i][0];
	    count = 0.0;
	}

	count += values[i][1];

	if (count > max) {
	    max = count;
	    mode = prev;
	}
    }

    if (max == 0.0)
	Rast_set_d_null_value(result, 1);
    else
	*result = mode;
}
