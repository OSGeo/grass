#include "mat.h"
#include "local_proto.h"

/*
 * m_copy: matrix equivalency (return a = b).
 */

int m_copy(MATRIX * a, MATRIX * b)
{
    register int r, c;
    register double *ap, *bp;

    if (b->nrows == 0)
	return error("=: arg2 not defined\n");

    r = b->nrows;
    a->nrows = b->nrows;
    a->ncols = b->ncols;
    while (r--) {
	c = b->ncols;
	ap = &(a->x[r][0]);
	bp = &(b->x[r][0]);
	while (c--)
	    *ap++ = *bp++;
    }

    return 1;
}
