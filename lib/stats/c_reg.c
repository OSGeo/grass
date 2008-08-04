#include <grass/gis.h>

#define REGRESSION_SLOPE	0
#define REGRESSION_OFFSET	1
#define REGRESSION_COEFF_DET	2

static void regression(DCELL * result, DCELL * values, int n, int which)
{
    DCELL xsum, ysum;
    DCELL xbar, ybar;
    DCELL numer, denom, denom2;
    int count;
    int i;

    xsum = ysum = 0.0;
    count = 0;

    for (i = 0; i < n; i++) {
	if (G_is_d_null_value(&values[i]))
	    continue;

	xsum += i;
	ysum += values[i];
	count++;
    }

    if (count < 2) {
	G_set_d_null_value(result, 1);
	return;
    }

    xbar = xsum / count;
    ybar = ysum / count;

    numer = 0.0;
    for (i = 0; i < n; i++)
	if (!G_is_d_null_value(&values[i]))
	    numer += i * values[i];
    numer -= count * xbar * ybar;

    denom = 0.0;
    for (i = 0; i < n; i++)
	if (!G_is_d_null_value(&values[i]))
	    denom += (DCELL) i *i;

    denom -= count * xbar * xbar;

    if (which == REGRESSION_COEFF_DET) {
	denom2 = 0.0;
	for (i = 0; i < n; i++)
	    if (!G_is_d_null_value(&values[i]))
		denom2 += values[i] * values[i];
	denom2 -= count * ybar * ybar;
    }

    switch (which) {
    case REGRESSION_SLOPE:
	*result = numer / denom;
	break;
    case REGRESSION_OFFSET:
	*result = ybar - xbar * numer / denom;
	break;
    case REGRESSION_COEFF_DET:
	*result = (numer * numer) / (denom * denom2);
	break;
    default:
	G_set_d_null_value(result, 1);
	break;
    }

    /* Check for NaN */
    if (*result != *result)
	G_set_d_null_value(result, 1);
}

void c_reg_m(DCELL * result, DCELL * values, int n)
{
    regression(result, values, n, REGRESSION_SLOPE);
}

void c_reg_c(DCELL * result, DCELL * values, int n)
{
    regression(result, values, n, REGRESSION_OFFSET);
}

void c_reg_r2(DCELL * result, DCELL * values, int n)
{
    regression(result, values, n, REGRESSION_COEFF_DET);
}

static void regression_w(DCELL * result, DCELL(*values)[2], int n, int which)
{
    DCELL xsum, ysum;
    DCELL xbar, ybar;
    DCELL numer, denom, denom2;
    int count;
    int i;

    xsum = ysum = 0.0;
    count = 0;

    for (i = 0; i < n; i++) {
	if (G_is_d_null_value(&values[i][0]))
	    continue;

	xsum += i * values[i][1];
	ysum += values[i][0] * values[i][1];
	count += values[i][1];
    }

    if (count < 2) {
	G_set_d_null_value(result, 1);
	return;
    }

    xbar = xsum / count;
    ybar = ysum / count;

    numer = 0.0;
    for (i = 0; i < n; i++)
	if (!G_is_d_null_value(&values[i][0]))
	    numer += i * values[i][0] * values[i][1];
    numer -= count * xbar * ybar;

    denom = 0.0;
    for (i = 0; i < n; i++)
	if (!G_is_d_null_value(&values[i][0]))
	    denom += (DCELL) i *i * values[i][1];

    denom -= count * xbar * xbar;

    if (which == REGRESSION_COEFF_DET) {
	denom2 = 0.0;
	for (i = 0; i < n; i++)
	    if (!G_is_d_null_value(&values[i][0]))
		denom2 += values[i][0] * values[i][0] * values[i][1];
	denom2 -= count * ybar * ybar;
    }

    switch (which) {
    case REGRESSION_SLOPE:
	*result = numer / denom;
	break;
    case REGRESSION_OFFSET:
	*result = ybar - xbar * numer / denom;
	break;
    case REGRESSION_COEFF_DET:
	*result = (numer * numer) / (denom * denom2);
	break;
    default:
	G_set_d_null_value(result, 1);
	break;
    }

    /* Check for NaN */
    if (*result != *result)
	G_set_d_null_value(result, 1);
}

void w_reg_m(DCELL * result, DCELL(*values)[2], int n)
{
    regression_w(result, values, n, REGRESSION_SLOPE);
}

void w_reg_c(DCELL * result, DCELL(*values)[2], int n)
{
    regression_w(result, values, n, REGRESSION_OFFSET);
}

void w_reg_r2(DCELL * result, DCELL(*values)[2], int n)
{
    regression_w(result, values, n, REGRESSION_COEFF_DET);
}
