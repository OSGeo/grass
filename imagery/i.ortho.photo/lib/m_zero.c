#include "mat.h"

/*
 * zero: returns arg2 zero filled 
 */

int zero(MATRIX * a)
{
    register int i, j;

    for (i = 0; i < a->nrows; i++)
	for (j = 0; j < a->ncols; j++)
	    a->x[i][j] = 0.0;

    return 1;
}
