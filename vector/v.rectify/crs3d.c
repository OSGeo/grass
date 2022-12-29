
/***********************************************************************

   crs3d.c

   written by: Markus Metz

   based on crs.c - Center for Remote Sensing rectification routines

************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>

#include <grass/gis.h>
#include <grass/imagery.h>

#include "crs.h"

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

/***********************************************************************

  FUNCTION PROTOTYPES FOR STATIC (INTERNAL) FUNCTIONS

************************************************************************/

static int calccoef(struct Control_Points_3D *, double *, double *, double *, int);
static int calcls(struct Control_Points_3D *, struct MATRIX *,
                  double *, double *, double *,
		  double *, double *, double *);
static int exactdet(struct Control_Points_3D *, struct MATRIX *,
                    double *, double *, double *,
		    double *, double *, double *);
static int solvemat(struct MATRIX *, double *, double *, double *,
                    double *, double *, double *);
static double term(int, double, double, double);

/***********************************************************************

  TRANSFORM A SINGLE COORDINATE PAIR.

************************************************************************/

int CRS_georef_3d(double e1,	/* EASTING TO BE TRANSFORMED */
	          double n1,	/* NORTHING TO BE TRANSFORMED */
	          double z1,	/* HEIGHT TO BE TRANSFORMED */
	          double *e,	/* EASTING, TRANSFORMED */
	          double *n,	/* NORTHING, TRANSFORMED */
	          double *z,	/* HEIGHT, TRANSFORMED */
	          double E[],	/* EASTING COEFFICIENTS */
	          double N[],	/* NORTHING COEFFICIENTS */
	          double Z[],	/* HEIGHT COEFFICIENTS */
		  int order     /* ORDER OF TRANSFORMATION TO BE PERFORMED, MUST MATCH THE
				   ORDER USED TO CALCULATE THE COEFFICIENTS */
    )
{
    double e2, n2, z2, en, ez, nz,
           e3, n3, z3, e2n, e2z, en2, ez2, n2z, nz2, enz;

    switch (order) {
    case 1:
	*e = E[0] + E[1] * e1 + E[2] * n1 + E[3] * z1;
	*n = N[0] + N[1] * e1 + N[2] * n1 + N[3] * z1;
	*z = Z[0] + Z[1] * e1 + Z[2] * n1 + Z[3] * z1;
	break;

    case 2:
	e2 = e1 * e1;
	en = e1 * n1;
	ez = e1 * z1;
	n2 = n1 * n1;
	nz = n1 * z1;
	z2 = z1 * z1;

	*e = E[0] + E[1] * e1 + E[2] * n1 + E[3] * z1 +
	     E[4] * e2 + E[5] * en + E[6] * ez + E[7] * n2 + E[8] * nz + E[9] * z2;
	*n = N[0] + N[1] * e1 + N[2] * n1 + N[3] * z1 +
	     N[4] * e2 + N[5] * en + N[6] * ez + N[7] * n2 + N[8] * nz + N[9] * z2;
	*z = Z[0] + Z[1] * e1 + Z[2] * n1 + Z[3] * z1 +
	     Z[4] * e2 + Z[5] * en + Z[6] * ez + Z[7] * n2 + Z[8] * nz + Z[9] * z2;
	break;

    case 3:
	e2 = e1 * e1;
	en = e1 * n1;
	ez = e1 * z1;
	n2 = n1 * n1;
	nz = n1 * z1;
	z2 = z1 * z1;

	e3 = e1 * e2;
	e2n = e2 * n1;
	e2z = e2 * z1;
	en2 = e1 * n2;
	enz = e1 * n1 * z1;
	ez2 = e1 * z2;
	n3 = n1 * n2;
	n2z = n2 * z1;
	nz2 = n1 * z2;
	z3 = z1 * z2;

	*e = E[0] + E[1] * e1 + E[2] * n1 + E[3] * z1 +
	     E[4] * e2 + E[5] * en + E[6] * ez + E[7] * n2 + E[8] * nz + E[9] * z2 +
	     E[10] * e3 + E[11] * e2n + E[12] * e2z + E[13] * en2 + E[14] * enz +
	     E[15] * ez2 + E[16] * n3 + E[17] * n2z + E[18] * nz2 + E[19] * z3;
	*n = N[0] + N[1] * e1 + N[2] * n1 + N[3] * z1 +
	     N[4] * e2 + N[5] * en + N[6] * ez + N[7] * n2 + N[8] * nz + N[9] * z2 +
	     N[10] * e3 + N[11] * e2n + N[12] * e2z + N[13] * en2 + N[14] * enz +
	     N[15] * ez2 + N[16] * n3 + N[17] * n2z + N[18] * nz2 + N[19] * z3;
	*z = Z[0] + Z[1] * e1 + Z[2] * n1 + Z[3] * z1 +
	     Z[4] * e2 + Z[5] * en + Z[6] * ez + Z[7] * n2 + Z[8] * nz + Z[9] * z2 +
	     Z[10] * e3 + Z[11] * e2n + Z[12] * e2z + Z[13] * en2 + Z[14] * enz +
	     Z[15] * ez2 + Z[16] * n3 + Z[17] * n2z + Z[18] * nz2 + Z[19] * z3;
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

int CRS_compute_georef_equations_3d(struct Control_Points_3D *cp,
                                    double E12[], double N12[], double Z12[],
				    double E21[], double N21[], double Z21[],
				    int order)
{
    double *tempptr;
    int status;

    if (order < 1 || order > MAXORDER)
	return MPARMERR;

    /* CALCULATE THE FORWARD TRANSFORMATION COEFFICIENTS */

    status = calccoef(cp, E12, N12, Z12, order);

    if (status != MSUCCESS)
	return status;

    /* SWITCH THE 1 AND 2 EASTING, NORTHING, AND HEIGHT ARRAYS */

    tempptr = cp->e1;
    cp->e1 = cp->e2;
    cp->e2 = tempptr;
    tempptr = cp->n1;
    cp->n1 = cp->n2;
    cp->n2 = tempptr;
    tempptr = cp->z1;
    cp->z1 = cp->z2;
    cp->z2 = tempptr;

    /* CALCULATE THE BACKWARD TRANSFORMATION COEFFICIENTS */

    status = calccoef(cp, E21, N21, Z21, order);

    /* SWITCH THE 1 AND 2 EASTING, NORTHING, AND HEIGHT ARRAYS BACK */

    tempptr = cp->e1;
    cp->e1 = cp->e2;
    cp->e2 = tempptr;
    tempptr = cp->n1;
    cp->n1 = cp->n2;
    cp->n2 = tempptr;
    tempptr = cp->z1;
    cp->z1 = cp->z2;
    cp->z2 = tempptr;

    return status;
}

/***********************************************************************

  COMPUTE THE GEOREFFERENCING COEFFICIENTS
  BASED ON A SET OF CONTROL POINTS

************************************************************************/

static int calccoef(struct Control_Points_3D *cp,
                    double E[], double N[], double Z[],
		    int order)
{
    struct MATRIX m;
    double *a;
    double *b;
    double *c;
    int numactive;		/* NUMBER OF ACTIVE CONTROL POINTS */
    int status, i;

    /* CALCULATE THE NUMBER OF VALID CONTROL POINTS */

    for (i = numactive = 0; i < cp->count; i++) {
	if (cp->status[i] > 0)
	    numactive++;
    }

    /* CALCULATE THE MINIMUM NUMBER OF CONTROL POINTS NEEDED TO DETERMINE
       A TRANSFORMATION OF THIS ORDER */

    /*
		    2D    3D
       1st order:    3     4
       2nd order:    6    10
       3rd order:   10	  20
    */

    m.n = numactive + 1;

    if (order == 1)
	m.n = 4;
    else if (order == 2)
	m.n = 10;
    else if (order == 3)
	m.n = 20;
    
    if (numactive < m.n)
	return MNPTERR;

    /* INITIALIZE MATRIX */

    m.v = G_calloc(m.n * m.n, sizeof(double));
    a = G_calloc(m.n, sizeof(double));
    b = G_calloc(m.n, sizeof(double));
    c = G_calloc(m.n, sizeof(double));

    if (numactive == m.n)
	status = exactdet(cp, &m, a, b, c, E, N, Z);
    else
	status = calcls(cp, &m, a, b, c, E, N, Z);

    G_free(m.v);
    G_free(a);
    G_free(b);
    G_free(c);

    return status;
}

/***********************************************************************

  CALCULATE THE TRANSFORMATION COEFFICIENTS WITH EXACTLY THE MINIMUM
  NUMBER OF CONTROL POINTS REQUIRED FOR THIS TRANSFORMATION.

************************************************************************/

static int exactdet(struct Control_Points_3D *cp, struct MATRIX *m,
                    double a[], double b[], double c[],
		    double E[],	/* EASTING COEFFICIENTS */
		    double N[],	/* NORTHING COEFFICIENTS */
		    double Z[]	/* HEIGHT COEFFICIENTS */
    )
{
    int pntnow, currow, j;

    currow = 1;
    for (pntnow = 0; pntnow < cp->count; pntnow++) {
	if (cp->status[pntnow] > 0) {
	    /* POPULATE MATRIX M */

	    for (j = 1; j <= m->n; j++)
		M(currow, j) = term(j, cp->e1[pntnow], cp->n1[pntnow], cp->z1[pntnow]);

	    /* POPULATE MATRIX A AND B */

	    a[currow - 1] = cp->e2[pntnow];
	    b[currow - 1] = cp->n2[pntnow];
	    c[currow - 1] = cp->z2[pntnow];

	    currow++;
	}
    }

    if (currow - 1 != m->n)
	return MINTERR;

    return solvemat(m, a, b, c, E, N, Z);
}

/***********************************************************************

  CALCULATE THE TRANSFORMATION COEFFICIENTS WITH MORE THAN THE MINIMUM
  NUMBER OF CONTROL POINTS REQUIRED FOR THIS TRANSFORMATION.  THIS
  ROUTINE USES THE LEAST SQUARES METHOD TO COMPUTE THE COEFFICIENTS.

************************************************************************/

static int calcls(struct Control_Points_3D *cp, struct MATRIX *m,
                  double a[], double b[], double c[],
		  double E[],	/* EASTING COEFFICIENTS */
		  double N[],	/* NORTHING COEFFICIENTS */
		  double Z[]	/* HEIGHT COEFFICIENTS */
    )
{
    int i, j, n, numactive = 0;

    /* INITIALIZE THE UPPER HALF OF THE MATRIX AND THE TWO COLUMN VECTORS */

    for (i = 1; i <= m->n; i++) {
	for (j = i; j <= m->n; j++)
	    M(i, j) = 0.0;
	a[i - 1] = b[i - 1] = c[i - 1] = 0.0;
    }

    /* SUM THE UPPER HALF OF THE MATRIX AND THE COLUMN VECTORS ACCORDING TO
       THE LEAST SQUARES METHOD OF SOLVING OVER DETERMINED SYSTEMS */

    for (n = 0; n < cp->count; n++) {
	if (cp->status[n] > 0) {
	    numactive++;
	    for (i = 1; i <= m->n; i++) {
		for (j = i; j <= m->n; j++)
		    M(i, j) += term(i, cp->e1[n], cp->n1[n], cp->z1[n]) *
		               term(j, cp->e1[n], cp->n1[n], cp->z1[n]);

		a[i - 1] += cp->e2[n] * term(i, cp->e1[n], cp->n1[n], cp->z1[n]);
		b[i - 1] += cp->n2[n] * term(i, cp->e1[n], cp->n1[n], cp->z1[n]);
		c[i - 1] += cp->z2[n] * term(i, cp->e1[n], cp->n1[n], cp->z1[n]);
	    }
	}
    }

    if (numactive <= m->n)
	return MINTERR;

    /* TRANSPOSE VALUES IN UPPER HALF OF M TO OTHER HALF */

    for (i = 2; i <= m->n; i++)
	for (j = 1; j < i; j++)
	    M(i, j) = M(j, i);

    return solvemat(m, a, b, c, E, N, Z);
}

/***********************************************************************

  CALCULATE THE X/Y TERM BASED ON THE TERM NUMBER

  ORDER\TERM   1      2      3      4      5      6      7      8      9      10
  1            e0n0z0 e1n0z0 e0n1z0 e0n0z1
  2            e0n0z0 e1n0z0 e0n1z0 e0n0z1 e2n0z0 e1n1z0 e1n0z1 e0n2z0 e0n1z1 e0n0z2
  3            e0n0z0 e1n0z0 e0n1z0 e0n0z1 e2n0z0 e1n1z0 e1n0z1 e0n2z0 e0n1z1 e0n0z2

  ORDER\TERM   11      12     13     14     15     16     17     18     19     20
  3            e3n0z0  e2n1z0 e2n0z1 e1n2z0 e1n1z1 e1n0z2 e0n3z0 e0n2z1 e0n1z2 e0n0z3

************************************************************************/

static double term(int term, double e, double n, double z)
{
    switch (term) {
    /* 1st order */
    case 1:
	return 1.0;
    case 2:
	return e;
    case 3:
	return n;
    case 4:
	return z;
    /* 2nd order */
    case 5:
	return e * e;
    case 6:
	return e * n;
    case 7:
	return e * z;
    case 8:
	return n * n;
    case 9:
	return n * z;
    case 10:
	return z * z;
    /* 3rd order */
    case 11:
	return e * e * e;
    case 12:
	return e * e * n;
    case 13:
	return e * e * z;
    case 14:
	return e * n * n;
    case 15:
	return e * n * z;
    case 16:
	return e * z * z;
    case 17:
	return n * n * n;
    case 18:
	return n * n * z;
    case 19:
	return n * z * z;
    case 20:
	return z * z * z;
    }

    return 0.0;
}

/***********************************************************************

  SOLVE FOR THE 'E', 'N' AND 'Z' COEFFICIENTS BY USING A
  SOMEWHAT MODIFIED GAUSSIAN ELIMINATION METHOD.

  | M11 M12 ... M1n | | E0   |   | a0   |
  | M21 M22 ... M2n | | E1   | = | a1   |
  |  .   .   .   .  | | .    |   | .    |
  | Mn1 Mn2 ... Mnn | | En-1 |   | an-1 |

  ,

  | M11 M12 ... M1n | | N0   |   | b0   |
  | M21 M22 ... M2n | | N1   | = | b1   |
  |  .   .   .   .  | | .    |   | .    |
  | Mn1 Mn2 ... Mnn | | Nn-1 |   | bn-1 |

  and

  | M11 M12 ... M1n | | Z0   |   | c0   |
  | M21 M22 ... M2n | | Z1   | = | c1   |
  |  .   .   .   .  | | .    |   | .    |
  | Mn1 Mn2 ... Mnn | | Zn-1 |   | cn-1 |

************************************************************************/

static int solvemat(struct MATRIX *m, double a[], double b[], double c[],
                    double E[], double N[], double Z[])
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

	if (fabs(pivot) < GRASS_EPSILON)
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

	    temp = c[imark - 1];
	    c[imark - 1] = c[i - 1];
	    c[i - 1] = temp;
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
		c[i2 - 1] -= factor * c[i - 1];
	    }
	}
    }

    /* SINCE ALL OTHER VALUES IN THE MATRIX ARE ZERO NOW, CALCULATE THE
       COEFFICIENTS BY DIVIDING THE COLUMN VECTORS BY THE DIAGONAL VALUES. */

    for (i = 1; i <= m->n; i++) {
	E[i - 1] = a[i - 1] / M(i, i);
	N[i - 1] = b[i - 1] / M(i, i);
	Z[i - 1] = c[i - 1] / M(i, i);
    }

    return MSUCCESS;
}
