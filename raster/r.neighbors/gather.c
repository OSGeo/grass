#include <grass/gis.h>
#include "ncb.h"

/*
   given the starting col of the neighborhood,
   copy the cell values from the bufs into the array of values
   and return the number of values copied.
 */

#define sqr(x) ((x) * (x))

void circle_mask(void)
{
    int i, j;

    if (ncb.mask)
	return;

    ncb.mask = G_malloc(ncb.nsize * sizeof(char *));

    for (i = 0; i < ncb.nsize; i++)
	ncb.mask[i] = G_malloc(ncb.nsize);

    for (i = 0; i < ncb.nsize; i++)
	for (j = 0; j < ncb.nsize; j++)
	    ncb.mask[i][j] =
		sqr(i - ncb.dist) + sqr(j - ncb.dist) <= sqr(ncb.dist);
}

void weights_mask(void)
{
    int i, j;

    if (ncb.mask)
	return;

    ncb.mask = G_malloc(ncb.nsize * sizeof(char *));

    for (i = 0; i < ncb.nsize; i++)
	ncb.mask[i] = G_malloc(ncb.nsize);

    for (i = 0; i < ncb.nsize; i++)
	for (j = 0; j < ncb.nsize; j++)
	    ncb.mask[i][j] = ncb.weights[i][j] != 0;
}

int gather(DCELL * values, int offset)
{
    int row, col;
    int n = 0;

    *values = 0;

    for (row = 0; row < ncb.nsize; row++)
	for (col = 0; col < ncb.nsize; col++) {
	    DCELL *c = &ncb.buf[row][offset + col];

	    if (ncb.mask && !ncb.mask[row][col])
		continue;

	    if (G_is_d_null_value(c))
		G_set_d_null_value(&values[n], 1);
	    else
		values[n] = *c;

	    n++;
	}

    return n ? n : -1;
}

int gather_w(DCELL(*values)[2], int offset)
{
    int row, col;
    int n = 0;

    values[0][0] = 0;
    values[0][1] = 1;

    for (row = 0; row < ncb.nsize; row++)
	for (col = 0; col < ncb.nsize; col++) {
	    DCELL *c = &ncb.buf[row][offset + col];

	    if (G_is_d_null_value(c))
		G_set_d_null_value(&values[n][0], 1);
	    else
		values[n][0] = *c;

	    values[n][1] = ncb.weights[row][col];

	    n++;
	}

    return n ? n : -1;
}
