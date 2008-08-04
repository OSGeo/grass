#include <grass/gis.h>


void c_minx(DCELL * result, DCELL * values, int n)
{
    DCELL min, minx;
    int i;

    G_set_d_null_value(&min, 1);
    G_set_d_null_value(&minx, 1);

    for (i = 0; i < n; i++) {
	if (G_is_d_null_value(&values[i]))
	    continue;

	if (G_is_d_null_value(&min) || min > values[i]) {
	    min = values[i];
	    minx = i;
	}
    }

    if (G_is_d_null_value(&minx))
	G_set_d_null_value(result, 1);
    else
	*result = minx;
}
