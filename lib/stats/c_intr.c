#include <grass/gis.h>
#include <grass/stats.h>

void c_intr(DCELL * result, DCELL * values, int n)
{
    DCELL center;
    int count;
    int diff;
    int i;

    if (G_is_d_null_value(&values[n / 2])) {
	G_set_d_null_value(result, 1);
	return;
    }

    center = values[n / 2];
    count = 0;
    diff = 0;

    for (i = 0; i < n; i++) {
	if (G_is_d_null_value(&values[i]))
	    continue;

	count++;
	if (values[i] != center)
	    diff++;
    }

    count--;

    if (count <= 0)
	*result = 0;
    else
	*result = (diff * 100.0 + (count / 2)) / count + 1;
}
