#include <grass/gis.h>

void c_max(DCELL * result, DCELL * values, int n, const void *closure)
{
    DCELL max;
    int i;

    G_set_d_null_value(&max, 1);

    for (i = 0; i < n; i++) {
	if (G_is_d_null_value(&values[i]))
	    continue;

	if (G_is_d_null_value(&max) || max < values[i])
	    max = values[i];
    }

    if (G_is_d_null_value(&max))
	G_set_d_null_value(result, 1);
    else
	*result = max;
}

void w_max(DCELL * result, DCELL(*values)[2], int n, const void *closure)
{
    DCELL max;
    int i;

    G_set_d_null_value(&max, 1);

    for (i = 0; i < n; i++) {
	if (G_is_d_null_value(&values[i][0]))
	    continue;

	if (G_is_d_null_value(&max) || max < values[i][0])
	    max = values[i][0];
    }

    if (G_is_d_null_value(&max))
	G_set_d_null_value(result, 1);
    else
	*result = max;
}
