#include <stdlib.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/stats.h>

static int ascending(const void *aa, const void *bb)
{
    const DCELL *a = aa, *b = bb;

    if (*a < *b)
	return -1;
    return (*a > *b);


    if (Rast_is_d_null_value((DCELL *) a) && Rast_is_d_null_value((DCELL *) b))
	return 0;

    if (Rast_is_d_null_value((DCELL *) a))
	return 1;

    if (Rast_is_d_null_value((DCELL *) b))
	return -1;

    return (*a < *b) ? -1 : (*a > *b) ? 1 : 0;
}

int sort_cell(DCELL * array, int n)
{
    int i, j;

    j = 0;
    for (i = 0; i < n; i++) {
	if (!Rast_is_d_null_value(&array[i])) {
	    array[j] = array[i];
	    j++;
	}
    }
    n = j;

    if (n > 0)
	qsort(array, n, sizeof(DCELL), ascending);

    return n;
}

int sort_cell_w(DCELL(*array)[2], int n)
{
    int i, j;

    j = 0;
    for (i = 0; i < n; i++) {
	if (!Rast_is_d_null_value(&array[i][0]) &&
	    !Rast_is_d_null_value(&array[i][1])) {
	    array[j][0] = array[i][0];
	    array[j][1] = array[i][1];
	    j++;
	}
    }
    n = j;

    if (n > 0)
	qsort(array, n, 2 * sizeof(DCELL), ascending);

    return n;
}
