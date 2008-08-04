#include <grass/gis.h>

void c_count(DCELL * result, DCELL * values, int n)
{
    int count;
    int i;

    count = 0;

    for (i = 0; i < n; i++)
	if (!G_is_d_null_value(&values[i]))
	    count++;

    *result = count;
}

void w_count(DCELL * result, DCELL(*values)[2], int n)
{
    DCELL count;
    int i;

    count = 0.0;

    for (i = 0; i < n; i++)
	if (!G_is_d_null_value(&values[i][0]))
	    count += values[i][1];

    *result = count;
}
