#include <grass/gis.h>

void c_maxx(DCELL * result, DCELL * values, int n)
{
    DCELL max, maxx;
    int i;

    G_set_d_null_value(&max, 1);
    G_set_d_null_value(&maxx, 1);

    for (i = 0; i < n; i++) {
	if (G_is_d_null_value(&values[i]))
	    continue;

	if (G_is_d_null_value(&max) || max < values[i]) {
	    max = values[i];
	    maxx = i;
	}
    }

    if (G_is_d_null_value(&maxx))
	G_set_d_null_value(result, 1);
    else
	*result = maxx;
}
