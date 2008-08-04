#include <math.h>
#include <grass/imagery.h>
#include "mat.h"
#include "local_proto.h"

#define ZERO 1.0e-8

/*
 * isnull: returns 1 if matrix is null, else 0.
 */

int isnull(MATRIX * a)
{
    register int i, j, rows, cols;

    if (a->nrows == 0)
	return error("isnull: argument not defined.\n");

    rows = a->nrows;
    cols = a->ncols;

    for (i = 0; i < rows; i++)
	for (j = 0; j < cols; j++)
	    if ((fabs(a->x[i][j]) - ZERO) > ZERO)
		return 0;

    return 1;
}
