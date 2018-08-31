#include <stdio.h>
#include "mat.h"
#include "matrixdefs.h"
#include "local_proto.h"

/*
 * m_add: matrix addition (returns c = a + b)
 */

int m_add(MATRIX * a, MATRIX * b, MATRIX * c)
{
    register int nr, nc;
    char message[256];
    register double *ap, *bp, *mp;
    static MATRIX m;

    if (a->nrows == 0)
	return error("+: arg1 not defined\n");
    else if (b->nrows == 0)
	return error("+: arg2 not defined\n");

    /* check for conformity */
    if ((a->nrows != b->nrows) || (a->ncols != b->ncols)) {
	sprintf(message, "+: matrices not conformable, %d x %d + %d x %d\n",
		a->nrows, a->ncols, b->nrows, b->ncols);
	return error(message);
    }

    nr = a->nrows;
    while (nr--) {
	nc = a->ncols;
	ap = &(a->x[nr][0]);
	bp = &(b->x[nr][0]);
	mp = &(m.x[nr][0]);
	while (nc--)
	    *mp++ = *ap++ + *bp++;
    }
    m.nrows = a->nrows;
    m.ncols = a->ncols;
    m_copy(c, &m);
    return 1;
}
