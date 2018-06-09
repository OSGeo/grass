#include <grass/gis.h>
#include <grass/raster.h>

void c_kurt(DCELL * result, DCELL * values, int n, const void *closure)
{
    DCELL sum, ave, sumsq, sumqt, var;
    int count;
    int i;

    sum = 0.0;
    count = 0;

    for (i = 0; i < n; i++) {
	if (Rast_is_d_null_value(&values[i]))
	    continue;

	sum += values[i];
	count++;
    }

    if (count == 0) {
	Rast_set_d_null_value(result, 1);
	return;
    }

    ave = sum / count;

    sumsq = 0;
    sumqt = 0;

    for (i = 0; i < n; i++) {
	DCELL d;

	if (Rast_is_d_null_value(&values[i]))
	    continue;

	d = values[i] - ave;
	sumsq += d * d;
	sumqt += d * d * d * d;
    }

    var = sumsq / count;

    *result = sumqt / (count * var * var) - 3;
}

void w_kurt(DCELL * result, DCELL(*values)[2], int n, const void *closure)
{
    DCELL sum, ave, sumsq, sumqt, var;
    DCELL count;
    int i;

    sum = 0.0;
    count = 0;

    for (i = 0; i < n; i++) {
	if (Rast_is_d_null_value(&values[i][0]))
	    continue;

	sum += values[i][0] * values[i][1];
	count += values[i][1];
    }

    if (count == 0) {
	Rast_set_d_null_value(result, 1);
	return;
    }

    ave = sum / count;

    sumsq = 0;
    sumqt = 0;

    for (i = 0; i < n; i++) {
	DCELL d;

	if (Rast_is_d_null_value(&values[i][0]))
	    continue;

	d = values[i][0] - ave;
	sumsq += d * d * values[i][1];
	sumqt += d * d * d * values[i][1];
    }

    var = sumsq / count;

    *result = sumqt / (count * var * var) - 3;
}
