
/***************************************************************************/

/***************************************************************************/
/*
   CRS.C - Center for Remote Sensing rectification routines

   Written By: Brian J. Buckley

   At: The Center for Remote Sensing
   Michigan State University
   302 Berkey Hall
   East Lansing, MI  48824
   (517)353-7195

   Written: 12/19/91

   Last Update: 12/26/91 Brian J. Buckley
   Last Update:  1/24/92 Brian J. Buckley
   Added printout of trnfile. Triggered by BDEBUG.
   Last Update:  1/27/92 Brian J. Buckley
   Fixed bug so that only the active control points were used.
 */

/***************************************************************************/

/***************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <grass/gis.h>

/*
   #define MSDOS 1
 */

/*
   #define BDEBUG
 */
#ifdef BDEBUG
FILE *fp;
#endif

#ifdef MSDOS

#include "stdlib.h"
#include "dummy.h"

typedef double DOUBLE;

#else

#include <grass/imagery.h>

typedef double DOUBLE;

#endif

#include "crs.h"

/* STRUCTURE FOR USE INTERNALLY WITH THESE FUNCTIONS.  THESE FUNCTIONS EXPECT
   SQUARE MATRICES SO ONLY ONE VARIABLE IS GIVEN (N) FOR THE MATRIX SIZE */

struct MATRIX
{
    int n;			/* SIZE OF THIS MATRIX (N x N) */
    DOUBLE *v;
};

/* CALCULATE OFFSET INTO ARRAY BASED ON R/C */

#define M(row,col) m->v[(((row)-1)*(m->n))+(col)-1]

/***************************************************************************/
/*
 */

/***************************************************************************/

#define MSUCCESS     1		/* SUCCESS */
#define MNPTERR      0		/* NOT ENOUGH POINTS */
#define MUNSOLVABLE -1		/* NOT SOLVABLE */
#define MMEMERR     -2		/* NOT ENOUGH MEMORY */
#define MPARMERR    -3		/* PARAMETER ERROR */
#define MINTERR     -4		/* INTERNAL ERROR */

/***************************************************************************/
/*
   FUNCTION PROTOTYPES FOR STATIC (INTERNAL) FUNCTIONS
 */

/***************************************************************************/

#ifdef MSDOS

static int calccoef(struct Control_Points *, double *, double *, int);
static int calcls(struct Control_Points *, struct MATRIX *,
		  DOUBLE *, DOUBLE *, double *, double *);
static int exactdet(struct Control_Points *, struct MATRIX *,
		    DOUBLE *, DOUBLE *, double *, double *);
static int solvemat(struct MATRIX *, DOUBLE *, DOUBLE *, double *, double *);
static DOUBLE term(int, double, double);

#ifdef BDEBUG
static int checkgeoref(struct Control_Points *, double *, double *, int, int,
		       FILE * fp);
#endif

#else

static int calccoef();
static int calcls();
static int exactdet();
static int solvemat();
static DOUBLE term();

#ifdef BDEBUG
static int checkgeoref();
#endif

#endif

/***************************************************************************/
/*
   USE THIS TRANSFORMATION FUNCTION IF YOU WANT TO DO ARRAYS.
 */

/***************************************************************************/

#ifdef BETTERGEOREF

extern void CRS_georef(int, DOUBLE *, DOUBLE *, DOUBLE *, DOUBLE *, int);

void CRS_georef2(int order,	/* ORDER OF TRANSFORMATION TO BE PERFORMED, MUST MATCH THE
				   ORDER USED TO CALCULATE THE COEFFICIENTS */
		 double E[],	/* EASTING COEFFICIENTS */
		 double N[],	/* NORTHING COEFFICIENTS */
		 double e[],	/* EASTINGS TO BE TRANSFORMED */
		 double n[],	/* NORTHINGS TO BE TRANSFORMED */
		 int numpts	/* NUMBER OF POINTS TO BE TRANSFORMED */
    )
{
    DOUBLE e3, e2n, e2, en2, en, e1, n3, n2, n1;
    int i;

    switch (order) {
    case 3:

	for (i = 0; i < numpts; i++) {
	    e1 = e[i];
	    n1 = n[i];
	    e2 = e1 * e1;
	    en = e1 * n1;
	    n2 = n1 * n1;
	    e3 = e1 * e2;
	    e2n = e2 * n1;
	    en2 = e1 * n2;
	    n3 = n1 * n2;

	    e[i] = E[0] +
		E[1] * e1 + E[2] * n1 +
		E[3] * e2 + E[4] * en + E[5] * n2 +
		E[6] * e3 + E[7] * e2n + E[8] * en2 + E[9] * n3;
	    n[i] = N[0] +
		N[1] * e1 + N[2] * n1 +
		N[3] * e2 + N[4] * en + N[5] * n2 +
		N[6] * e3 + N[7] * e2n + N[8] * en2 + N[9] * n3;
	}
	break;

    case 2:

	for (i = 0; i < numpts; i++) {
	    e1 = e[i];
	    n1 = n[i];
	    e2 = e1 * e1;
	    n2 = n1 * n1;
	    en = e1 * n1;

	    e[i] = E[0] + E[1] * e1 + E[2] * n1 +
		E[3] * e2 + E[4] * en + E[5] * n2;
	    n[i] = N[0] + N[1] * e1 + N[2] * n1 +
		N[3] * e2 + N[4] * en + N[5] * n2;
	}
	break;

    case 1:

	for (i = 0; i < numpts; i++) {
	    e1 = e[i];
	    n1 = n[i];
	    e[i] = E[0] + E[1] * e1 + E[2] * n1;
	    n[i] = N[0] + N[1] * e1 + N[2] * n1;
	}
	break;
    }
}

#endif

/***************************************************************************/
/*
   TRANSFORM A SINGLE COORDINATE PAIR.
 */

/***************************************************************************/

int CRS_georef(double e1,	/* EASTINGS TO BE TRANSFORMED */
	       double n1,	/* NORTHINGS TO BE TRANSFORMED */
	       double *e,	/* EASTINGS TO BE TRANSFORMED */
	       double *n,	/* NORTHINGS TO BE TRANSFORMED */
	       double E[],	/* EASTING COEFFICIENTS */
	       double N[],	/* NORTHING COEFFICIENTS */
	       int order	/* ORDER OF TRANSFORMATION TO BE PERFORMED, MUST MATCH THE
				   ORDER USED TO CALCULATE THE COEFFICIENTS */
    )
{
    DOUBLE e3, e2n, en2, n3, e2, en, n2;

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

	return (MPARMERR);
	break;
    }

    return (MSUCCESS);
}

/***************************************************************************/
/*
   COMPUTE THE GEOREFFERENCING COEFFICIENTS BASED ON A SET OF CONTROL POINTS
 */

/***************************************************************************/

int
CRS_compute_georef_equations(struct Control_Points *cp, double E12[],
			     double N12[], double E21[], double N21[],
			     int order)
{
    double *tempptr;
    int status;

    if (order < 1 || order > MAXORDER)
	return (MPARMERR);

#ifdef BDEBUG
    fp = fopen("error.dat", "w");
    if (fp == NULL)
	return (-1);
#endif

    /* CALCULATE THE FORWARD TRANSFORMATION COEFFICIENTS */

    status = calccoef(cp, E12, N12, order);
    if (status != MSUCCESS)
	return (status);

#ifdef BDEBUG
    checkgeoref(cp, E12, N12, order, 1, fp);
#endif

    /* SWITCH THE 1 AND 2 EASTING AND NORTHING ARRAYS */

    tempptr = cp->e1;
    cp->e1 = cp->e2;
    cp->e2 = tempptr;
    tempptr = cp->n1;
    cp->n1 = cp->n2;
    cp->n2 = tempptr;

    /* CALCULATE THE BACKWARD TRANSFORMATION COEFFICIENTS */

    status = calccoef(cp, E21, N21, order);

#ifdef BDEBUG
    checkgeoref(cp, E21, N21, order, 0, fp);
    fclose(fp);
#endif

    /* SWITCH THE 1 AND 2 EASTING AND NORTHING ARRAYS BACK */

    tempptr = cp->e1;
    cp->e1 = cp->e2;
    cp->e2 = tempptr;
    tempptr = cp->n1;
    cp->n1 = cp->n2;
    cp->n2 = tempptr;

    return (status);
}

/***************************************************************************/
/*
   COMPUTE THE GEOREFFERENCING COEFFICIENTS BASED ON A SET OF CONTROL POINTS
 */

/***************************************************************************/

static int
calccoef(struct Control_Points *cp, double E[], double N[], int order)
{
    struct MATRIX m;
    DOUBLE *a;
    DOUBLE *b;
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
	return (MNPTERR);

    /* INITIALIZE MATRIX */

    m.v = (DOUBLE *) G_calloc(m.n * m.n, sizeof(DOUBLE));
    if (m.v == NULL) {
	return (MMEMERR);
    }
    a = (DOUBLE *) G_calloc(m.n, sizeof(DOUBLE));
    if (a == NULL) {
	G_free((char *)m.v);
	return (MMEMERR);
    }
    b = (DOUBLE *) G_calloc(m.n, sizeof(DOUBLE));
    if (b == NULL) {
	G_free((char *)m.v);
	G_free((char *)a);
	return (MMEMERR);
    }

    if (numactive == m.n)
	status = exactdet(cp, &m, a, b, E, N);
    else
	status = calcls(cp, &m, a, b, E, N);

    G_free((char *)m.v);
    G_free((char *)a);
    G_free((char *)b);

    return (status);
}

/***************************************************************************/
/*
   CALCULATE THE TRANSFORMATION COEFFICIENTS WITH EXACTLY THE MINIMUM
   NUMBER OF CONTROL POINTS REQUIRED FOR THIS TRANSFORMATION.
 */

/***************************************************************************/

static int exactdet(struct Control_Points *cp, struct MATRIX *m, DOUBLE a[], DOUBLE b[], double E[],	/* EASTING COEFFICIENTS */
		    double N[]	/* NORTHING COEFFICIENTS */
    )
{
    int pntnow, currow, j;

    currow = 1;
    for (pntnow = 0; pntnow < cp->count; pntnow++) {
	if (cp->status[pntnow] > 0) {
	    /* POPULATE MATRIX M */

#ifdef BDEBUG
	    fprintf(fp, "%2d ", pntnow + 1);
#endif

	    for (j = 1; j <= m->n; j++) {
		M(currow, j) = term(j, cp->e1[pntnow], cp->n1[pntnow]);
#ifdef BDEBUG
		fprintf(fp, "%+14.7le ", M(currow, j));
		if (j == 5)
		    fprintf(fp, "\n   ");
#endif
	    }
#ifdef BDEBUG
	    fprintf(fp, "\n");
#endif

	    /* POPULATE MATRIX A AND B */

	    a[currow - 1] = cp->e2[pntnow];
	    b[currow - 1] = cp->n2[pntnow];
#ifdef BDEBUG
	    fprintf(fp, "   %+14.7le ", a[currow - 1]);
	    fprintf(fp, "%+14.7le\n", b[currow - 1]);
#endif

	    currow++;
	}
#ifdef BDEBUG
	else {
	    fprintf(fp, "%2d UNUSED\n", pntnow + 1);
	}
#endif
    }

    if (currow - 1 != m->n)
	return (MINTERR);

    return (solvemat(m, a, b, E, N));
}

/***************************************************************************/
/*
   CALCULATE THE TRANSFORMATION COEFFICIENTS WITH MORE THAN THE MINIMUM
   NUMBER OF CONTROL POINTS REQUIRED FOR THIS TRANSFORMATION.  THIS
   ROUTINE USES THE LEAST SQUARES METHOD TO COMPUTE THE COEFFICIENTS.
 */

/***************************************************************************/

static int calcls(struct Control_Points *cp, struct MATRIX *m, DOUBLE a[], DOUBLE b[], double E[],	/* EASTING COEFFICIENTS */
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
	return (MINTERR);

    /* TRANSPOSE VALUES IN UPPER HALF OF M TO OTHER HALF */

    for (i = 2; i <= m->n; i++) {
	for (j = 1; j < i; j++)
	    M(i, j) = M(j, i);
    }

    return (solvemat(m, a, b, E, N));
}

/***************************************************************************/
/*
   CALCULATE THE X/Y TERM BASED ON THE TERM NUMBER

   ORDER\TERM   1    2    3    4    5    6    7    8    9   10
   1        e0n0 e1n0 e0n1
   2        e0n0 e1n0 e0n1 e2n0 e1n1 e0n2
   3        e0n0 e1n0 e0n1 e2n0 e1n1 e0n2 e3n0 e2n1 e1n2 e0n3
 */

/***************************************************************************/

static DOUBLE term(int term, double e, double n)
{
    switch (term) {
    case 1:
	return ((DOUBLE) 1.0);
    case 2:
	return ((DOUBLE) e);
    case 3:
	return ((DOUBLE) n);
    case 4:
	return ((DOUBLE) (e * e));
    case 5:
	return ((DOUBLE) (e * n));
    case 6:
	return ((DOUBLE) (n * n));
    case 7:
	return ((DOUBLE) (e * e * e));
    case 8:
	return ((DOUBLE) (e * e * n));
    case 9:
	return ((DOUBLE) (e * n * n));
    case 10:
	return ((DOUBLE) (n * n * n));
    }
    return ((DOUBLE) 0.0);
}

/***************************************************************************/
/*
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
 */

/***************************************************************************/

static int solvemat(struct MATRIX *m,
		    DOUBLE a[], DOUBLE b[], double E[], double N[])
{
    int i, j, i2, j2, imark;
    DOUBLE factor, temp;
    DOUBLE pivot;		/* ACTUAL VALUE OF THE LARGEST PIVOT CANDIDATE */

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
	    return (MUNSOLVABLE);

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

    return (MSUCCESS);
}

/***************************************************************************/
/*
 */

/***************************************************************************/

#ifdef BDEBUG

static int checkgeoref(struct Control_Points *cp,
		       double E[], double N[], int order, int forward,
		       FILE * fp)
{
    DOUBLE xrms, yrms, dx, dy, dx2, dy2, totaldist, dist;
    double tempx, tempy;
    int i, n, numactive;

    n = ((order + 1) * (order + 2)) / 2;

    if (forward)
	fprintf(fp, "FORWARD:\n");
    else
	fprintf(fp, "BACKWARD:\n");

    fprintf(fp, "%d order\n", order);
    for (i = 0; i < n; i++)
	fprintf(fp, "%+.17E %+.17E\n", E[i], N[i]);

    xrms = yrms = dx2 = dy2 = totaldist = 0.0;
    numactive = 0;
    for (i = 0; i < cp->count; i++) {
	fprintf(fp, "\nCONTROL POINT: %d\n", i + 1);

	fprintf(fp, "%20s: %+.20lE %+.20lE\n", "ORIGINAL POINT",
		cp->e1[i], cp->n1[i]);
	fprintf(fp, "%20s: %+.20lE %+.20lE\n", "DESIRED POINT",
		cp->e2[i], cp->n2[i]);

	if (cp->status[i] > 0) {
	    numactive++;
	    CRS_georef(cp->e1[i], cp->n1[i], &tempx, &tempy, E, N, order);

	    fprintf(fp, "%20s: %+.20lE %+.20lE\n", "CALCULATED POINT", tempx,
		    tempy);
	    dx = tempx - cp->e2[i];
	    dy = tempy - cp->n2[i];
	    fprintf(fp, "%20s: %+.20lE %+.20lE\n", "RESIDUAL ERROR", dx, dy);
	    dx2 = dx * dx;
	    dy2 = dy * dy;
	    dist = sqrt(dx2 + dy2);
	    fprintf(fp, "%20s: %+.20lE\n", "DISTANCE (RMS) ERROR", dist);

	    xrms += dx2;
	    yrms += dy2;

	    totaldist += dist;
	}
	else
	    fprintf(fp, "NOT USED\n");
    }
    xrms = sqrt(xrms / (DOUBLE) numactive);
    yrms = sqrt(yrms / (DOUBLE) numactive);

    fprintf(fp, "\n%20s: %+.20lE %+.20lE\n", "RMS ERROR", xrms, yrms);

    fprintf(fp, "\n%20s: %+.20lE\n", "TOTAL RMS ERROR",
	    sqrt(xrms * xrms + yrms * yrms));

    fprintf(fp, "\n%20s: %+.20lE\n", "AVG. DISTANCE ERROR",
	    totaldist / numactive);

    return (0);
}

#endif
