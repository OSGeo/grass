/*!
 * \author
 * Lubos Mitas (original program and various modifications)
 *
 * \author
 * H. Mitasova,
 * I. Kosinovsky, D. Gerdes,
 * D. McCauley
 * (GRASS4.1 version of the program and GRASS4.2 modifications)
 *
 * \author
 * L. Mitas,
 * H. Mitasova,
 * I. Kosinovsky,
 * D.Gerdes,
 * D. McCauley
 * (1993, 1995)
 *
 * \author modified by McCauley in August 1995
 * \author modified by Mitasova in August 1995, Nov. 1996
 * 
 * \copyright
 * (C) 1993-1996 by Lubos Mitas and the GRASS Development Team
 *
 * \copyright
 * This program is free software under the  GNU General Public License (>=v2).
 * Read the file COPYING that comes with GRASS for details.
 */


#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/interpf.h>
#include <grass/gmath.h>


int IL_matrix_create(struct interp_params *params,
		     struct triple *points,	/* points for interpolation */
		     int n_points,	/* number of points */
		     double **matrix,	/* matrix */
		     int *indx)
{
    static double *A = NULL;

    if (!A) {
	if (!
	    (A =
	     G_alloc_vector((params->KMAX2 + 2) * (params->KMAX2 + 2) + 1))) {
	    fprintf(stderr, "Cannot allocate memory for A\n");
	    return -1;
	}
    }
    return IL_matrix_create_alloc(params, points, n_points, matrix, indx, A);
}

/*!
 * \brief Creates system of linear equations from interpolated points
 *
 * Creates system of linear equations represented by matrix using given
 * points and interpolating function interp()
 *
 * \param params struct interp_params *
 * \param points points for interpolation as struct triple
 * \param n_points number of points
 * \param[out] matrix the matrix
 * \param indx
 *
 * \return -1 on failure, 1 on success
 */
int IL_matrix_create_alloc(struct interp_params *params,
			   struct triple *points,	/* points for interpolation */
			   int n_points,	/* number of points */
			   double **matrix,	/* matrix */
			   int *indx,
			   double *A /* temporary matrix unique for all threads */)
{
    double xx, yy;
    double rfsta2, r;
    double d;
    int n1, k1, k2, k, i1, l, m, i, j;
    double fstar2 = params->fi * params->fi / 4.;
    double RO, amaxa;
    double rsin = 0, rcos = 0, teta, scale = 0;	/*anisotropy parameters - added by JH 2002 */
    double xxr, yyr;

    if (params->theta) {
	teta = params->theta * (M_PI / 180);	/* deg to rad */
	rsin = sin(teta);
	rcos = cos(teta);
    }
    if (params->scalex)
	scale = params->scalex;


    n1 = n_points + 1;

    /*
       C      GENERATION OF MATRIX
       C      FIRST COLUMN
     */
    A[1] = 0.;
    for (k = 1; k <= n_points; k++) {
	i1 = k + 1;
	A[i1] = 1.;
    }
    /*
       C      OTHER COLUMNS
     */
    RO = -params->rsm;
    /* fprintf (stderr, "sm[%d] = %f,  ro=%f\n", 1, points[1].smooth, RO); */
    for (k = 1; k <= n_points; k++) {
	k1 = k * n1 + 1;
	k2 = k + 1;
	i1 = k1 + k;
	if (params->rsm < 0.) {	/*indicates variable smoothing */
	    A[i1] = -points[k - 1].sm;	/* added by Mitasova nov. 96 */
	    /* G_debug(5, "sm[%d]=%f, a=%f", k, points[k-1].sm, A[i1]); */
	}
	else {
	    A[i1] = RO;		/* constant smoothing */
	}
	/* if (i1 == 100) fprintf (stderr,i "A[%d] = %f\n", i1, A[i1]); */

	/* A[i1] = RO; */
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
		fprintf(stderr, "ident. points in segm.\n");
		fprintf(stderr, "x[%d]=%f, x[%d]=%f, y[%d]=%f, y[%d]=%f\n",
			k - 1, points[k - 1].x, l - 1, points[l - 1].x, k - 1,
			points[k - 1].y, l - 1, points[l - 1].y);
		return -1;
	    }
	    i1 = k1 + l;
	    A[i1] = params->interp(r, params->fi);
	}
    }

    /* C       SYMMETRISATION */
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

    G_debug(3, "calling G_ludcmp()  n=%d indx=%d", n_points, *indx);
    if (G_ludcmp(matrix, n_points + 1, indx, &d) <= 0) {
	/* find the inverse of the matrix */
	fprintf(stderr, "G_ludcmp() failed! n=%d  d=%.2f\n", n_points, d);
	return -1;
    }

    /* G_free_vector(A); */
    return 1;
}
