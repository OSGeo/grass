/*  @(#)inverse.c       2.1  6/26/87  */
#include <math.h>
#include <grass/libtrans.h>

#define EPSILON 1.0e-16

/* DIM_matrix is defined in "libtrans.h" */
#define N	DIM_matrix

/*
 * inverse: invert a square matrix (puts pivot elements on main diagonal).
 *          returns arg2 as the inverse of arg1.
 *
 *  This routine is based on a routine found in Andrei Rogers, "Matrix
 *  Methods in Urban and Regional Analysis", (1971), pp. 143-153.
 */
int inverse(double m[N][N])
{
    int i, j, k, l, ir = 0, ic = 0;
    int ipivot[N], itemp[N][2];
    double pivot[N], t;
    double fabs();


    if (isnull(m))
	return (-1);


    /* initialization */
    for (i = 0; i < N; i++)
	ipivot[i] = 0;

    for (i = 0; i < N; i++) {
	t = 0.0;		/* search for pivot element */

	for (j = 0; j < N; j++) {
	    if (ipivot[j] == 1)	/* found pivot */
		continue;

	    for (k = 0; k < N; k++)
		switch (ipivot[k] - 1) {
		case 0:
		    break;
		case -1:
		    if (fabs(t) < fabs(m[j][k])) {
			ir = j;
			ic = k;
			t = m[j][k];
		    }
		    break;
		case 1:
		    return (-1);
		    break;
		default:	/* shouldn't get here */
		    return (-1);
		    break;
		}
	}

	ipivot[ic] += 1;
	if (ipivot[ic] > 1) {	/* check for dependency */
	    return (-1);
	}

	/* interchange rows to put pivot element on diagonal */
	if (ir != ic)
	    for (l = 0; l < N; l++) {
		t = m[ir][l];
		m[ir][l] = m[ic][l];
		m[ic][l] = t;
	    }

	itemp[i][0] = ir;
	itemp[i][1] = ic;
	pivot[i] = m[ic][ic];

	/* check for zero pivot */
	if (fabs(pivot[i]) < EPSILON) {
	    return (-1);
	}

	/* divide pivot row by pivot element */
	m[ic][ic] = 1.0;

	for (j = 0; j < N; j++)
	    m[ic][j] /= pivot[i];

	/* reduce nonpivot rows */
	for (k = 0; k < N; k++)
	    if (k != ic) {
		t = m[k][ic];
		m[k][ic] = 0.0;

		for (l = 0; l < N; l++)
		    m[k][l] -= (m[ic][l] * t);
	    }
    }

    /* interchange columns */
    for (i = 0; i < N; i++) {
	l = N - i - 1;
	if (itemp[l][0] == itemp[l][1])
	    continue;

	ir = itemp[l][0];
	ic = itemp[l][1];

	for (k = 0; k < N; k++) {
	    t = m[k][ir];
	    m[k][ir] = m[k][ic];
	    m[k][ic] = t;
	}
    }

    return 1;
}




#define ZERO 1.0e-8

/*
 * isnull: returns 1 if matrix is null, else 0.
 */

int isnull(double a[N][N])
{
    register int i, j;
    double fabs();


    for (i = 0; i < N; i++)
	for (j = 0; j < N; j++)
	    if ((fabs(a[i][j]) - ZERO) > ZERO)
		return 0;

    return 1;
}
