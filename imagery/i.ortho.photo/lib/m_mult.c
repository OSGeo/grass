#include <stdio.h>
#include "mat.h"
#include "matrixdefs.h"
#include "local_proto.h"

/*
 * m_mult: matrix multiplication (return c = a * b)
 */

int m_mult(MATRIX * a, MATRIX * b, MATRIX * c)
{
    register int i, j, k, nr, nc, ncols;
    char message[256];

    if (a->nrows == 0)
	return error("*: arg1 not defined\n");
    else if (b->nrows == 0)
	return error("*: arg2 not defined\n");

    /* check for conformity */
    if (a->ncols != b->nrows) {
	sprintf(message, "*: matrices not conformable, %d x %d * %d x %d\n",
		a->nrows, a->ncols, b->nrows, b->ncols);
	fprintf(stderr, message);
	return error(message);
    }

    ncols = a->ncols;
    nr = a->nrows;
    nc = b->ncols;
    for (i = 0; i < nr; i++)
	for (j = 0; j < nc; j++) {
	    c->x[i][j] = 0.0;
	    for (k = 0; k < ncols; k++)
		c->x[i][j] += (a->x[i][k] * b->x[k][j]);
	}

    c->nrows = nr;
    c->ncols = nc;
    return 1;
}
