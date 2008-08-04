#include <grass/gis.h>

void c_range(DCELL * result, DCELL * values, int n)
{
    DCELL min, max;
    int i;

    G_set_d_null_value(&min, 1);
    G_set_d_null_value(&max, 1);

    for (i = 0; i < n; i++) {
	if (G_is_d_null_value(&values[i]))
	    continue;

	if (G_is_d_null_value(&min) || min > values[i])
	    min = values[i];

	if (G_is_d_null_value(&max) || max < values[i])
	    max = values[i];
    }

    if (G_is_d_null_value(&min) || G_is_d_null_value(&max))
	G_set_d_null_value(result, 1);
    else
	*result = max - min;
}
