#include <grass/gis.h>

void c_min(DCELL * result, DCELL * values, int n, const void *closure)
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

void w_min(DCELL * result, DCELL(*values)[2], int n, const void *closure)
{
    DCELL min;
    int i;

    G_set_d_null_value(&min, 1);

    for (i = 0; i < n; i++) {
	if (G_is_d_null_value(&values[i][0]))
	    continue;

	if (G_is_d_null_value(&min) || min > values[i][0])
	    min = values[i][0];
    }

    if (G_is_d_null_value(&min))
	G_set_d_null_value(result, 1);
    else
	*result = min;
}
