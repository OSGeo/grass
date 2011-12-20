
/****************************************************************************
 *
 * MODULE:       imagery library
 * AUTHOR(S):    Original author(s) name(s) unknown - written by CERL
 *               Written By: Brian J. Buckley
 *
 *               At: The Center for Remote Sensing
 *               Michigan State University
 *
 * PURPOSE:      Image processing library
 * COPYRIGHT:    (C) 1999, 2005 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

/*
 *  Written: 12/19/91
 *
 *  Last Update: 12/26/91 Brian J. Buckley
 *  Last Update:  1/24/92 Brian J. Buckley
 *  Added printout of trnfile. Triggered by BDEBUG.
 *  Last Update:  1/27/92 Brian J. Buckley
 *  Fixed bug so that only the active control points were used.
 * 
 */


#include <stdlib.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/imagery.h>
#include <signal.h>

/* STRUCTURE FOR USE INTERNALLY WITH THESE FUNCTIONS.  THESE FUNCTIONS EXPECT
   SQUARE MATRICES SO ONLY ONE VARIABLE IS GIVEN (N) FOR THE MATRIX SIZE */

struct MATRIX
{
    int n;			/* SIZE OF THIS MATRIX (N x N) */
    double *v;
};

/* CALCULATE OFFSET INTO ARRAY BASED ON R/C */

#define M(row,col) m->v[(((row)-1)*(m->n))+(col)-1]

#define MSUCCESS     1		/* SUCCESS */
#define MNPTERR      0		/* NOT ENOUGH POINTS */
#define MUNSOLVABLE -1		/* NOT SOLVABLE */
#define MMEMERR     -2		/* NOT ENOUGH MEMORY */
#define MPARMERR    -3		/* PARAMETER ERROR */
#define MINTERR     -4		/* INTERNAL ERROR */

#define MAXORDER 3    /* HIGHEST SUPPORTED ORDER OF TRANSFORMATION */

/***********************************************************************

  FUNCTION PROTOTYPES FOR STATIC (INTERNAL) FUNCTIONS

************************************************************************/

static int calccoef(struct Control_Points *, double *, double *, int);
static int calcls(struct Control_Points *, struct MATRIX *, double *,
		  double *, double *, double *);
static int exactdet(struct Control_Points *, struct MATRIX *, double *,
		    double *, double *, double *);
static int solvemat(struct MATRIX *, double *, double *, double *, double *);
static double term(int, double, double);

/***********************************************************************

  TRANSFORM A SINGLE COORDINATE PAIR.

************************************************************************/

int I_georef(double e1,    /* EASTING TO BE TRANSFORMED */
	     double n1,	   /* NORTHING TO BE TRANSFORMED */
	     double *e,	   /* EASTING, TRANSFORMED */
	     double *n,	   /* NORTHING, TRANSFORMED */
	     double E[],   /* EASTING COEFFICIENTS */
	     double N[],   /* NORTHING COEFFICIENTS */
	     int order	   /* ORDER OF TRANSFORMATION TO BE PERFORMED, MUST MATCH THE
			      ORDER USED TO CALCULATE THE COEFFICIENTS */
    )
{
    double e3, e2n, en2, n3, e2, en, n2;

    switch (order) {
    case 1:
	*e = E[0] + E[1] * e1 + E[2] * n1;
	*n = N[0] + N[1] * e1 + N[2] * n1;
	break;

    case 2:
	e2 = e1 * e1;
	n2 = n1 * n1;
	en = e1 * n1;

	*e = E[0] + E[1] * e1 + E[2] * n1 + E[3] * e2 + E[4] * en + E[5] * n2;
	*n = N[0] + N[1] * e1 + N[2] * n1 + N[3] * e2 + N[4] * en + N[5] * n2;
	break;

    case 3:
	e2 = e1 * e1;
	en = e1 * n1;
	n2 = n1 * n1;
	e3 = e1 * e2;
	e2n = e2 * n1;
	en2 = e1 * n2;
	n3 = n1 * n2;

	*e = E[0] +
	    E[1] * e1 + E[2] * n1 +
	    E[3] * e2 + E[4] * en + E[5] * n2 +
	    E[6] * e3 + E[7] * e2n + E[8] * en2 + E[9] * n3;
	*n = N[0] +
	    N[1] * e1 + N[2] * n1 +
	    N[3] * e2 + N[4] * en + N[5] * n2 +
	    N[6] * e3 + N[7] * e2n + N[8] * en2 + N[9] * n3;
	break;

    default:
	return MPARMERR;
    }

    return MSUCCESS;
}

/***********************************************************************

  COMPUTE THE FORWARD AND BACKWARD GEOREFFERENCING COEFFICIENTS
  BASED ON A SET OF CONTROL POINTS

************************************************************************/

int I_compute_georef_equations(struct Control_Points *cp, double E12[],
			       double N12[], double E21[], double N21[],
			       int order)
{
    double *tempptr;
    int status;

    if (order < 1 || order > MAXORDER)
	return MPARMERR;

    /* CALCULATE THE FORWARD TRANSFORMATION COEFFICIENTS */

    status = calccoef(cp, E12, N12, order);

    if (status != MSUCCESS)
	return status;

    /* SWITCH THE 1 AND 2 EASTING AND NORTHING ARRAYS */

    tempptr = cp->e1;
    cp->e1 = cp->e2;
    cp->e2 = tempptr;
    tempptr = cp->n1;
    cp->n1 = cp->n2;
    cp->n2 = tempptr;

    /* CALCULATE THE BACKWARD TRANSFORMATION COEFFICIENTS */

    status = calccoef(cp, E21, N21, order);

    /* SWITCH THE 1 AND 2 EASTING AND NORTHING ARRAYS BACK */

    tempptr = cp->e1;
    cp->e1 = cp->e2;
    cp->e2 = tempptr;
    tempptr = cp->n1;
    cp->n1 = cp->n2;
    cp->n2 = tempptr;

    return status;
}

/***********************************************************************

  COMPUTE THE GEOREFFERENCING COEFFICIENTS
  BASED ON A SET OF CONTROL POINTS

************************************************************************/

static int calccoef(struct Control_Points *cp, double E[], double N[],
		    int order)
{
    struct MATRIX m;
    double *a;
    double *b;
    int numactive;		/* NUMBER OF ACTIVE CONTROL POINTS */
    int status, i;

    /* CALCULATE THE NUMBER OF VALID CONTROL POINTS */

    for (i = numactive = 0; i < cp->count; i++) {
	if (cp->status[i] > 0)
	    numactive++;
    }

    /* CALCULATE THE MINIMUM NUMBER OF CONTROL POINTS NEEDED TO DETERMINE
       A TRANSFORMATION OF THIS ORDER */

    m.n = ((order + 1) * (order + 2)) / 2;

    if (numactive < m.n)
	return MNPTERR;

    /* INITIALIZE MATRIX */

    m.v = G_calloc(m.n * m.n, sizeof(double));
    a = G_calloc(m.n, sizeof(double));
    b = G_calloc(m.n, sizeof(double));

    if (numactive == m.n)
	status = exactdet(cp, &m, a, b, E, N);
    else
	status = calcls(cp, &m, a, b, E, N);

    G_free(m.v);
    G_free(a);
    G_free(b);

    return status;
}

/***********************************************************************

  CALCULATE THE TRANSFORMATION COEFFICIENTS WITH EXACTLY THE MINIMUM
  NUMBER OF CONTROL POINTS REQUIRED FOR THIS TRANSFORMATION.

************************************************************************/

static int exactdet(struct Control_Points *cp, struct MATRIX *m,
                    double a[], double b[],
		    double E[],	/* EASTING COEFFICIENTS */
		    double N[]	/* NORTHING COEFFICIENTS */
    )
{
    int pntnow, currow, j;

    currow = 1;
    for (pntnow = 0; pntnow < cp->count; pntnow++) {
	if (cp->status[pntnow] > 0) {
	    /* POPULATE MATRIX M */

	    for (j = 1; j <= m->n; j++)
		M(currow, j) = term(j, cp->e1[pntnow], cp->n1[pntnow]);

	    /* POPULATE MATRIX A AND B */

	    a[currow - 1] = cp->e2[pntnow];
	    b[currow - 1] = cp->n2[pntnow];

	    currow++;
	}
    }

    if (currow - 1 != m->n)
	return MINTERR;

    return solvemat(m, a, b, E, N);
}

/***********************************************************************

  CALCULATE THE TRANSFORMATION COEFFICIENTS WITH MORE THAN THE MINIMUM
  NUMBER OF CONTROL POINTS REQUIRED FOR THIS TRANSFORMATION.  THIS
  ROUTINE USES THE LEAST SQUARES METHOD TO COMPUTE THE COEFFICIENTS.

************************************************************************/

static int calcls(struct Control_Points *cp, struct MATRIX *m,
                  double a[], double b[],
		  double E[],	/* EASTING COEFFICIENTS */
		  double N[]	/* NORTHING COEFFICIENTS */
    )
{
    int i, j, n, numactive = 0;

    /* INITIALIZE THE UPPER HALF OF THE MATRIX AND THE TWO COLUMN VECTORS */

    for (i = 1; i <= m->n; i++) {
	for (j = i; j <= m->n; j++)
	    M(i, j) = 0.0;
	a[i - 1] = b[i - 1] = 0.0;
    }

    /* SUM THE UPPER HALF OF THE MATRIX AND THE COLUMN VECTORS ACCORDING TO
       THE LEAST SQUARES METHOD OF SOLVING OVER DETERMINED SYSTEMS */

    for (n = 0; n < cp->count; n++) {
	if (cp->status[n] > 0) {
	    numactive++;
	    for (i = 1; i <= m->n; i++) {
		for (j = i; j <= m->n; j++)
		    M(i, j) +=
			term(i, cp->e1[n], cp->n1[n]) * term(j, cp->e1[n],
							     cp->n1[n]);

		a[i - 1] += cp->e2[n] * term(i, cp->e1[n], cp->n1[n]);
		b[i - 1] += cp->n2[n] * term(i, cp->e1[n], cp->n1[n]);
	    }
	}
    }

    if (numactive <= m->n)
	return MINTERR;

    /* TRANSPOSE VALUES IN UPPER HALF OF M TO OTHER HALF */

    for (i = 2; i <= m->n; i++)
	for (j = 1; j < i; j++)
	    M(i, j) = M(j, i);

    return solvemat(m, a, b, E, N);
}

/***********************************************************************

  CALCULATE THE X/Y TERM BASED ON THE TERM NUMBER

  ORDER\TERM   1    2    3    4    5    6    7    8    9   10
  1            e0n0 e1n0 e0n1
  2            e0n0 e1n0 e0n1 e2n0 e1n1 e0n2
  3            e0n0 e1n0 e0n1 e2n0 e1n1 e0n2 e3n0 e2n1 e1n2 e0n3

************************************************************************/

static double term(int term, double e, double n)
{
    switch (term) {
    case 1:
	return 1.0;
    case 2:
	return e;
    case 3:
	return n;
    case 4:
	return e * e;
    case 5:
	return e * n;
    case 6:
	return n * n;
    case 7:
	return e * e * e;
    case 8:
	return e * e * n;
    case 9:
	return e * n * n;
    case 10:
	return n * n * n;
    }

    return 0.0;
}

/***********************************************************************

  SOLVE FOR THE 'E' AND 'N' COEFFICIENTS BY USING A SOMEWHAT MODIFIED
  GAUSSIAN ELIMINATION METHOD.

  | M11 M12 ... M1n | | E0   |   | a0   |
  | M21 M22 ... M2n | | E1   | = | a1   |
  |  .   .   .   .  | | .    |   | .    |
  | Mn1 Mn2 ... Mnn | | En-1 |   | an-1 |

  and

  | M11 M12 ... M1n | | N0   |   | b0   |
  | M21 M22 ... M2n | | N1   | = | b1   |
  |  .   .   .   .  | | .    |   | .    |
  | Mn1 Mn2 ... Mnn | | Nn-1 |   | bn-1 |

************************************************************************/

static int solvemat(struct MATRIX *m, double a[], double b[], double E[],
		    double N[])
{
    int i, j, i2, j2, imark;
    double factor, temp;
    double pivot;		/* ACTUAL VALUE OF THE LARGEST PIVOT CANDIDATE */

    for (i = 1; i <= m->n; i++) {
	j = i;

	/* find row with largest magnitude value for pivot value */

	pivot = M(i, j);
	imark = i;
	for (i2 = i + 1; i2 <= m->n; i2++) {
	    temp = fabs(M(i2, j));
	    if (temp > fabs(pivot)) {
		pivot = M(i2, j);
		imark = i2;
	    }
	}

	/* if the pivot is very small then the points are nearly co-linear */
	/* co-linear points result in an undefined matrix, and nearly */
	/* co-linear points results in a solution with rounding error */

	if (pivot == 0.0)
	    return MUNSOLVABLE;

	/* if row with highest pivot is not the current row, switch them */

	if (imark != i) {
	    for (j2 = 1; j2 <= m->n; j2++) {
		temp = M(imark, j2);
		M(imark, j2) = M(i, j2);
		M(i, j2) = temp;
	    }

	    temp = a[imark - 1];
	    a[imark - 1] = a[i - 1];
	    a[i - 1] = temp;

	    temp = b[imark - 1];
	    b[imark - 1] = b[i - 1];
	    b[i - 1] = temp;
	}

	/* compute zeros above and below the pivot, and compute
	   values for the rest of the row as well */

	for (i2 = 1; i2 <= m->n; i2++) {
	    if (i2 != i) {
		factor = M(i2, j) / pivot;
		for (j2 = j; j2 <= m->n; j2++)
		    M(i2, j2) -= factor * M(i, j2);
		a[i2 - 1] -= factor * a[i - 1];
		b[i2 - 1] -= factor * b[i - 1];
	    }
	}
    }

    /* SINCE ALL OTHER VALUES IN THE MATRIX ARE ZERO NOW, CALCULATE THE
       COEFFICIENTS BY DIVIDING THE COLUMN VECTORS BY THE DIAGONAL VALUES. */

    for (i = 1; i <= m->n; i++) {
	E[i - 1] = a[i - 1] / M(i, i);
	N[i - 1] = b[i - 1] / M(i, i);
    }

    return MSUCCESS;
}
