/*
 *
 * Original program and various modifications:
 * Lubos Mitas 
 *
 * GRASS4.1 version of the program and GRASS4.2 modifications:
 * H. Mitasova,
 * I. Kosinovsky, D. Gerdes
 * D. McCauley 
 *
 * Copyright 1993, 1995:
 * L. Mitas ,
 * H. Mitasova ,
 * I. Kosinovsky,
 * D.Gerdes 
 * D. McCauley 
 *
 * modified by McCauley in August 1995
 * modified by Mitasova in August 1995, Nov. 1996
 *
 */

#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/interpf.h>
#include <grass/gmath.h>

int IL_matrix_create(struct interp_params *params, struct triple *points,	/* points for interpolation */
		     int n_points,	/* number of points */
		     double **matrix,	/* matrix */
		     int *indx)
/*
   Creates system of linear equations represented by matrix using given points
   and interpolating function interp()
 */
{
    double xx, yy;
    double rfsta2, r;
    double d;
    int n1, k1, k2, k, i1, l, m, i, j;
    double fstar2 = params->fi * params->fi / 4.;
    static double *A = NULL;
    double RO, amaxa;
    double rsin = 0, rcos = 0, teta, scale = 0;	/*anisotropy parameters - added by JH 2002 */
    double xxr, yyr;

    if (params->theta) {
	teta = params->theta / 57.295779;	/* deg to rad */
	rsin = sin(teta);
	rcos = cos(teta);
    }
    if (params->scalex)
	scale = params->scalex;


    n1 = n_points + 1;

    if (!A) {
	if (!
	    (A =
	     G_alloc_vector((params->KMAX2 + 2) * (params->KMAX2 + 2) + 1))) {
	    fprintf(stderr, "Cannot allocate memory for A\n");
	    return -1;
	}
    }

    /*
       C
       C      GENERATION OF MATRIX
       C
       C      FIRST COLUMN
       C
     */
    A[1] = 0.;
    for (k = 1; k <= n_points; k++) {
	i1 = k + 1;
	A[i1] = 1.;
    }
    /*
       C
       C      OTHER COLUMNS
       C
     */
    RO = -params->rsm;
    /*    fprintf (stderr,"sm[%d]=%f,ro=%f\n",1,points[1].smooth,RO); */
    for (k = 1; k <= n_points; k++) {
	k1 = k * n1 + 1;
	k2 = k + 1;
	i1 = k1 + k;
	if (params->rsm < 0.) {	/*indicates variable smoothing */
	    A[i1] = -points[k - 1].sm;	/* added by Mitasova nov. 96 */
	    /*           fprintf (stderr,"sm[%d]=%f,a=%f\n",k,points[k-1].sm,A[i1]); */
	}
	else {
	    A[i1] = RO;		/* constant smoothing */
	}
	/*        if (i1 == 100) fprintf (stderr,"A[%d]=%f\n",i1,A[i1]); */

	/*      A[i1] = RO; */
	for (l = k2; l <= n_points; l++) {
	    xx = points[k - 1].x - points[l - 1].x;
	    yy = points[k - 1].y - points[l - 1].y;

	    if ((params->theta) && (params->scalex)) {
		/* re run anisotropy */
		xxr = xx * rcos + yy * rsin;
		yyr = yy * rcos - xx * rsin;
		xx = xxr;
		yy = yyr;
		r = scale * xx * xx + yy * yy;
		rfsta2 = fstar2 * (scale * xx * xx + yy * yy);
	    }
	    else {
		r = xx * xx + yy * yy;
		rfsta2 = fstar2 * (xx * xx + yy * yy);
	    }

	    if (rfsta2 == 0.) {
		fprintf(stderr, "ident. points in segm.  \n");
		fprintf(stderr, "x[%d]=%f,x[%d]=%f,y[%d]=%f,y[%d]=%f\n",
			k - 1, points[k - 1].x, l - 1, points[l - 1].x, k - 1,
			points[k - 1].y, l - 1, points[l - 1].y);
		return -1;
	    }
	    i1 = k1 + l;
	    A[i1] = params->interp(r, params->fi);
	}
    }
    /*
       C
       C       SYMMETRISATION
       C
     */
    amaxa = 1.;
    for (k = 1; k <= n1; k++) {
	k1 = (k - 1) * n1;
	k2 = k + 1;
	for (l = k2; l <= n1; l++) {
	    m = (l - 1) * n1 + k;
	    A[m] = A[k1 + l];
	    amaxa = amax1(A[m], amaxa);
	}
    }
    m = 0;
    for (i = 0; i <= n_points; i++) {
	for (j = 0; j <= n_points; j++) {
	    m++;
	    matrix[i][j] = A[m];
	}
    }

    if (G_ludcmp(matrix, n_points + 1, indx, &d) <= 0) {	/* find the inverse of the mat
								   rix */
	fprintf(stderr, "G_ludcmp() failed! n=%d\n", n_points);
	return -1;
    }
    /*
       G_free_vector(A);
     */
    return 1;
}
