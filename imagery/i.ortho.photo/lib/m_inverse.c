#include <math.h>
#include "mat.h"
#include "matrixdefs.h"
#include "local_proto.h"

#define EPSILON 1.0e-8

/*
 * inverse: invert a square martix (puts pivot elements on main diagonal).
 *          returns arg2 as the inverse of arg1.
 *
 *  This routine is based on a routine found in Andrei Rogers, "Matrix
 *  Methods in Urban and Regional Analysis", (1971), pp. 143-153.
 */

int inverse(MATRIX * a, MATRIX * b)
{
    int i, j, k, l, ir = 0, ic = 0, nr, nc;
    int ipivot[MAXROWS], itemp[MAXROWS][2];
    double pivot[MAXROWS], t;
    static MATRIX m;

    if (a->nrows == 0)
	return matrix_error("inv: arg1 not defined\n");

    if (a->nrows != a->ncols)
	return matrix_error("inv: matrix not square\n");

    if (isnull(a)) {
	/* fprintf (stderr, " inv: matrix is singular\n"); */
	return
	    matrix_error
	    ("inv: matrix is singular. Check camera definitions!\n");
    }
    m_copy(&m, a);
    nr = a->nrows;
    nc = a->ncols;

    /* initialization */
    for (i = 0; i < nr; i++)
	ipivot[i] = 0;

    for (i = 0; i < nr; i++) {
	t = 0.0;		/* search for pivot element */
	for (j = 0; j < nr; j++) {
	    if (ipivot[j] == 1)	/* found pivot */
		continue;
	    for (k = 0; k < nc; k++)
		switch (ipivot[k] - 1) {
		case 0:
		    break;
		case -1:
		    if (fabs(t) < fabs(m.x[j][k])) {
			ir = j;
			ic = k;
			t = m.x[j][k];
		    }
		    break;
		case 1:
		    return
			matrix_error
			("inv: matrix is singular. Check camera definitions!\n");
		    break;
		default:	/* shouldn't get here */
		    return
			matrix_error
			("inv: matrix is singular. Check camera definitions!\n");
		    break;
		}
	}
	ipivot[ic] += 1;
	if (ipivot[ic] > 1)	/* check for dependency */
	    return
		matrix_error
		("inv: matrix is singular. Check camera definitions!\n");
	/* interchange rows to put pivot element on diagonal */
	if (ir != ic)
	    for (l = 0; l < nc; l++) {
		t = m.x[ir][l];
		m.x[ir][l] = m.x[ic][l];
		m.x[ic][l] = t;
	    }

	itemp[i][0] = ir;
	itemp[i][1] = ic;
	pivot[i] = m.x[ic][ic];

	/* check for zero pivot */
	if (fabs(pivot[i]) < EPSILON)
	    return
		matrix_error
		("inv: matrix is singular. Check camera definitions!\n");

	/* divide pivot row by pivot element */
	m.x[ic][ic] = 1.0;
	for (j = 0; j < nc; j++)
	    m.x[ic][j] /= pivot[i];

	/* reduce nonpivot rows */
	for (k = 0; k < nr; k++)
	    if (k != ic) {
		t = m.x[k][ic];
		m.x[k][ic] = 0.0;
		for (l = 0; l < nc; l++)
		    m.x[k][l] -= (m.x[ic][l] * t);
	    }
    }

    /* interchange columns */
    for (i = 0; i < nc; i++) {
	l = nc - i - 1;
	if (itemp[l][0] == itemp[l][1])
	    continue;
	ir = itemp[l][0];
	ic = itemp[l][1];
	for (k = 0; k < nr; k++) {
	    t = m.x[k][ir];
	    m.x[k][ir] = m.x[k][ic];
	    m.x[k][ic] = t;
	}
    }

    b->nrows = nr;
    b->ncols = nc;
    m_copy(b, &m);
    return 1;
}
