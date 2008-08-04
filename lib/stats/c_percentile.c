#include <math.h>
#include <grass/gis.h>
#include <grass/stats.h>

static void percentile(DCELL * result, DCELL * values, int n, double percent)
{
    double k;
    int i0, i1;

    n = sort_cell(values, n);

    if (n < 1) {
	G_set_d_null_value(result, 1);
	return;
    }

    k = n * percent / 100;
    i0 = (int)floor(k);
    i1 = (int)ceil(k);

    *result = (i0 == i1)
	? values[i0]
	: values[i0] * (i1 - k) + values[i1] * (k - i0);
}

void c_quart1(DCELL * result, DCELL * values, int n)
{
    percentile(result, values, n, 25.0);
}

void c_quart3(DCELL * result, DCELL * values, int n)
{
    percentile(result, values, n, 75.0);
}

void c_perc90(DCELL * result, DCELL * values, int n)
{
    percentile(result, values, n, 90.0);
}

static void percentile_w(DCELL * result, DCELL(*values)[2], int n,
			 double percent)
{
    DCELL total;
    int i;
    DCELL k;

    n = sort_cell_w(values, n);

    if (n < 1) {
	G_set_d_null_value(result, 1);
	return;
    }

    total = 0.0;
    for (i = 0; i < n; i++)
	total += values[i][1];

    k = 0.0;
    for (i = 0; i < n; i++) {
	k += values[i][1];
	if (k >= total * percent / 100)
	    break;
    }

    *result = values[i][0];
}

void w_quart1(DCELL * result, DCELL(*values)[2], int n)
{
    percentile_w(result, values, n, 25.0);
}

void w_quart3(DCELL * result, DCELL(*values)[2], int n)
{
    percentile_w(result, values, n, 75.0);
}

void w_perc90(DCELL * result, DCELL(*values)[2], int n)
{
    percentile_w(result, values, n, 90.0);
}
