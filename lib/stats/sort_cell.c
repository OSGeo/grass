#include <stdlib.h>
#include <grass/gis.h>
#include <grass/stats.h>

static int ascending(const void *aa, const void *bb)
{
    const DCELL *a = aa, *b = bb;

    if (G_is_d_null_value((DCELL *) a) && G_is_d_null_value((DCELL *) b))
	return 0;

    if (G_is_d_null_value((DCELL *) a))
	return 1;

    if (G_is_d_null_value((DCELL *) b))
	return -1;

    return (*a < *b) ? -1 : (*a > *b) ? 1 : 0;
}

int sort_cell(DCELL * array, int n)
{
    int i;

    qsort(array, n, sizeof(DCELL), ascending);

    for (i = 0; i < n; i++)
	if (G_is_d_null_value(&array[i]))
	    break;

    return i;
}

int sort_cell_w(DCELL(*array)[2], int n)
{
    int i;

    qsort(array, n, 2 * sizeof(DCELL), ascending);

    for (i = 0; i < n; i++)
	if (G_is_d_null_value(&array[i][0]))
	    break;

    return i;
}
