
/****************************************************************************
 *
 * MODULE:       imagery library
 * AUTHOR(S):    Markus Metz
 *
 * PURPOSE:      Image processing library
 * COPYRIGHT:    (C) 2013 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/imagery.h>
#include <grass/glocale.h>
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

#ifndef MAX
#define MAX(x,y) ((x) > (y) ? (x) : (y))
#endif
#ifndef MIN
#define MIN(x,y) ((x) < (y) ? (x) : (y))
#endif

/***********************************************************************

  FUNCTION PROTOTYPES FOR STATIC (INTERNAL) FUNCTIONS

************************************************************************/

static int calccoef(struct Control_Points *, double **, double **);
static int calcls(struct Control_Points *, struct MATRIX *, double *,
		  double *, double *, double *);

static double tps_base_func(const double x1, const double y1,
                           const double x2, const double y2);
static int solvemat(struct MATRIX *, double *, double *, double *, double *);

/***********************************************************************

  TRANSFORM A SINGLE COORDINATE PAIR.

************************************************************************/

int I_georef_tps(double e1,    /* EASTING TO BE TRANSFORMED */
	         double n1,    /* NORTHING TO BE TRANSFORMED */
	         double *e,    /* EASTING, TRANSFORMED */
	         double *n,    /* NORTHING, TRANSFORMED */
	         double *E,    /* EASTING COEFFICIENTS */
	         double *N,    /* NORTHING COEFFICIENTS */
		 struct Control_Points *cp,
		 int fwd
    )
{
    int  i, j;
    double dist, *pe, *pn;

    if (fwd) {
	pe = cp->e1;
	pn = cp->n1;
    }
    else {
	pe = cp->e2;
	pn = cp->n2;
    }

    /* global affine (1st order poly) */
    *e = E[0] + e1 * E[1] + n1 * E[2];
    *n = N[0] + e1 * N[1] + n1 * N[2];

    
    for (i = 0, j = 0; i < cp->count; i++) {
	if (cp->status[i] > 0) {

	    dist = tps_base_func(e1, n1, pe[i], pn[i]);

	    *e += E[j + 3] * dist;
	    *n += N[j + 3] * dist;
	    j++;
	}
    }

    return MSUCCESS;
}

/***********************************************************************

  COMPUTE THE FORWARD AND BACKWARD GEOREFFERENCING COEFFICIENTS
  BASED ON A SET OF CONTROL POINTS

************************************************************************/

int I_compute_georef_equations_tps(struct Control_Points *cp,
                                   double **E12tps, double **N12tps,
				   double **E21tps, double **N21tps)
{
    double *tempptr;
    int numactive;		/* NUMBER OF ACTIVE CONTROL POINTS */
    int status, i;
    double xmax, xmin, ymax, ymin;
    double delx, dely;
    double xx, yy;
    double sumx, sumy, sumx2, sumy2, sumxy;
    double SSxx, SSyy, SSxy;

    /* CALCULATE THE NUMBER OF VALID CONTROL POINTS */

    for (i = numactive = 0; i < cp->count; i++) {
	if (cp->status[i] > 0)
	    numactive++;
    }

    if (numactive < 3)
	return MNPTERR;

    if (numactive > 100000)    /* arbitrary, admittedly */
	return MNPTERR;

    xmin = xmax = cp->e1[0];
    ymin = ymax = cp->n1[0];

    sumx = sumy = sumx2 = sumy2 = sumxy = 0.0;

    for (i = 0; i < cp->count; i++ ) {
	if (cp->status[i] > 0) {
	    xx = cp->e1[i];
	    yy = cp->n1[i];
		    
	    xmax = MAX(xmax, xx);
	    xmin = MIN(xmin, xx);
	    ymax = MAX(ymax, yy);
	    ymin = MIN(ymin, yy);
		    
	    sumx  += xx;
	    sumx2 += xx * xx;
	    sumy  += yy;
	    sumy2 += yy * yy;
	    sumxy += xx * yy;
	}
    }

    delx = xmax - xmin;
    dely = ymax - ymin;

    SSxx = sumx2 - sumx * sumx / numactive;
    SSyy = sumy2 - sumy * sumy / numactive;
    SSxy = sumxy - sumx * sumy / numactive;
	
    if (delx < 0.001 * dely || dely < 0.001 * delx || 
        fabs(SSxy * SSxy / (SSxx * SSyy)) > 0.99) {
	    /* points are colinear */
	    return MUNSOLVABLE;
    }

    xmin = xmax = cp->e2[0];
    ymin = ymax = cp->n2[0];

    sumx = sumy = sumx2 = sumy2 = sumxy = 0.0;
    for (i = 0; i < cp->count; i++ ) {
	if (cp->status[i] > 0) {
	    xx = cp->e2[i];
	    yy = cp->n2[i];
		    
	    xmax = MAX(xmax, xx);
	    xmin = MIN(xmin, xx);
	    ymax = MAX(ymax, yy);
	    ymin = MIN(ymin, yy);
		    
	    sumx  += xx;
	    sumx2 += xx * xx;
	    sumy  += yy;
	    sumy2 += yy * yy;
	    sumxy += xx * yy;
	}
    }

    delx = xmax - xmin;
    dely = ymax - ymin;

    SSxx = sumx2 - sumx * sumx / numactive;
    SSyy = sumy2 - sumy * sumy / numactive;
    SSxy = sumxy - sumx * sumy / numactive;
	
    if (delx < 0.001 * dely || dely < 0.001 * delx || 
        fabs(SSxy * SSxy / (SSxx * SSyy)) > 0.99) {
	    /* points are colinear */
	    return MUNSOLVABLE;
    }

    /* CALCULATE THE FORWARD TRANSFORMATION COEFFICIENTS */

    G_message(_("Calculating forward transformation coefficients"));
    status = calccoef(cp, E12tps, N12tps);

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

    G_message(_("Calculating backward transformation coefficients"));
    status = calccoef(cp, E21tps, N21tps);

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

static int calccoef(struct Control_Points *cp, double **E, double **N)
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

    /* INITIALIZE MATRIX */
    
    m.n = numactive + 3;

    m.v = G_calloc(m.n * m.n, sizeof(double));
    if (m.v == NULL)
	G_fatal_error(_("%s: out of memory"), "I_compute_georef_equations_tps()");
    a = G_calloc(m.n, sizeof(double));
    if (a == NULL)
	G_fatal_error(_("%s: out of memory"), "I_compute_georef_equations_tps()");
    b = G_calloc(m.n, sizeof(double));
    if (b == NULL)
	G_fatal_error(_("%s: out of memory"), "I_compute_georef_equations_tps()");

    /* equation coefficients */
    *E = G_calloc(m.n, sizeof(double));
    if (*E == NULL)
	G_fatal_error(_("%s: out of memory"), "I_compute_georef_equations_tps()");
    *N = G_calloc(m.n, sizeof(double));
    if (*N == NULL)
	G_fatal_error(_("%s: out of memory"), "I_compute_georef_equations_tps()");

    status = calcls(cp, &m, a, b, *E, *N);

    G_free(m.v);
    G_free(a);
    G_free(b);

    return status;
}


/***********************************************************************

  CALCULATE THE TRANSFORMATION COEFFICIENTS FOR THIN PLATE SPLINE 
  INTERPOLATION.
  THIS ROUTINE USES THE LEAST SQUARES METHOD TO COMPUTE THE COEFFICIENTS.

************************************************************************/

static int calcls(struct Control_Points *cp, struct MATRIX *m,
                  double a[], double b[],
		  double E[],	/* EASTING COEFFICIENTS */
		  double N[]	/* NORTHING COEFFICIENTS */
    )
{
    int i, j, n, o, numactive = 0;
    double dist = 0.0, dx, dy, regularization;

    /* INITIALIZE THE MATRIX AND THE TWO COLUMN VECTORS */

    for (i = 1; i <= m->n; i++) {
	for (j = i; j <= m->n; j++) {
	    M(i, j) = 0.0;
	    if (i != j)
		M(j, i) = 0.0;
	}
	a[i - 1] = b[i - 1] = 0.0;
    }

    /* SUM THE UPPER HALF OF THE MATRIX AND THE COLUMN VECTORS ACCORDING TO
       THE LEAST SQUARES METHOD OF SOLVING OVER DETERMINED SYSTEMS */

    for (n = 0; n < cp->count; n++) {
	if (cp->status[n] > 0) {

	    a[numactive + 3] = cp->e2[n];
	    b[numactive + 3] = cp->n2[n];

	    numactive++;
	    M(1, numactive + 3) = 1.0;
	    M(2, numactive + 3) = cp->e1[n];
	    M(3, numactive + 3) = cp->n1[n];

	    M(numactive + 3, 1) = 1.0;
	    M(numactive + 3, 2) = cp->e1[n];
	    M(numactive + 3, 3) = cp->n1[n];
	}
    }

    if (numactive < m->n - 3)
	return MINTERR;

    i = 0;
    for (n = 0; n < cp->count; n++) {
	if (cp->status[n] > 0) {
	    i++;

	    j = 0;
	    for (o = 0; o <= n; o++) {
		if (cp->status[o] > 0) {
		    j++;
		    M(i + 3, j + 3) = tps_base_func(cp->e1[n], cp->n1[n],
		                                    cp->e1[o], cp->n1[o]);

		    if (i != j)
			M(j + 3, i + 3) = M(i + 3, j + 3);
		 
		    dx = cp->e1[n] - cp->e1[o];
		    dy = cp->n1[n] - cp->n1[o];
		    dist += sqrt(dx * dx + dy * dy);
		}
	    }
	}
    }
    
    /* regularization */
    dist /= (numactive * numactive);
    regularization = 0.01 * dist * dist;
    
    /* set diagonal to regularization, but not the first 3x3 (global affine) */

    return solvemat(m, a, b, E, N);
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
	G_percent(i - 1, m->n, 4);
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
    G_percent(1, 1, 1);

    /* SINCE ALL OTHER VALUES IN THE MATRIX ARE ZERO NOW, CALCULATE THE
       COEFFICIENTS BY DIVIDING THE COLUMN VECTORS BY THE DIAGONAL VALUES. */

    for (i = 1; i <= m->n; i++) {
	E[i - 1] = a[i - 1] / M(i, i);
	N[i - 1] = b[i - 1] / M(i, i);
    }

    return MSUCCESS;
}

static double tps_base_func(const double x1, const double y1,
                           const double x2, const double y2)
{
    /* official: r * r * log(r) */
    double dist;

    if ((x1 == x2) && (y1 == y2))
	    return 0.0;

    dist  = (x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1);

    return dist * log(dist) * 0.5;
}
