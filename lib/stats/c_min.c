#include <grass/gis.h>

void c_min(DCELL * result, DCELL * values, int n)
{
    DCELL min;
    int i;

    G_set_d_null_value(&min, 1);

    for (i = 0; i < n; i++) {
	if (G_is_d_null_value(&values[i]))
	    continue;

	if (G_is_d_null_value(&min) || min > values[i])
	    min = values[i];
    }

    if (G_is_d_null_value(&min))
	G_set_d_null_value(result, 1);
    else
	*result = min;
}
