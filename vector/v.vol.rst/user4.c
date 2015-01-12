/*
 ****************************************************************************
 *
 * MODULE:       v.vol.rst: program for 3D (volume) interpolation and geometry
 *               analysis from scattered point data using regularized spline
 *               with tension
 *
 * AUTHOR(S):    Original program (1989) and various modifications:
 *               Lubos Mitas
 *
 *               GRASS 4.2, GRASS 5.0 version and modifications:
 *               H. Mitasova,  I. Kosinovsky, D. Gerdes, J. Hofierka
 *
 * PURPOSE:      v.vol.rst interpolates the values to 3-dimensional grid from
 *               point data (climatic stations, drill holes etc.) given in a
 *               3D vector point input. Output grid3 file is elev. 
 *               Regularized spline with tension is used for the
 *               interpolation.
 *
 * COPYRIGHT:    (C) 1989, 1993, 2000 L. Mitas,  H. Mitasova,
 *               I. Kosinovsky, D. Gerdes, J. Hofierka
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <grass/gis.h>

#include "oct.h"
#include "surf.h"
#include "dataoct.h"
#include "userextern.h"
#include "userglobs.h"

/*
   fi - tension parameter
   a - matrix of system of linear equations

 */

void clean()
{
    if (Tmp_fd_z) {
	fclose(Tmp_fd_z);
	unlink(Tmp_file_z);
    }
    if (Tmp_fd_dx) {
	fclose(Tmp_fd_dx);
	unlink(Tmp_file_dx);
    }
    if (Tmp_fd_dy) {
	fclose(Tmp_fd_dy);
	unlink(Tmp_file_dy);
    }
    if (Tmp_fd_dz) {
	fclose(Tmp_fd_dz);
	unlink(Tmp_file_dz);
    }
    if (Tmp_fd_xx) {
	fclose(Tmp_fd_xx);
	unlink(Tmp_file_xx);
    }
    if (Tmp_fd_yy) {
	fclose(Tmp_fd_yy);
	unlink(Tmp_file_yy);
    }
    if (Tmp_fd_xy) {
	fclose(Tmp_fd_xy);
	unlink(Tmp_file_xy);
    }
}



int min1(int arg1, int arg2)
{
    int res;

    if (arg1 <= arg2) {
	res = arg1;
    }
    else {
	res = arg2;
    }
    return res;
}


int max1(int arg1, int arg2)
{
    int res;

    if (arg1 >= arg2) {
	res = arg1;
    }
    else {
	res = arg2;
    }
    return res;
}


double amax1(double arg1, double arg2)
{
    double res;

    if (arg1 >= arg2) {
	res = arg1;
    }
    else {
	res = arg2;
    }
    return res;
}



double amin1(double arg1, double arg2)
{
    double res;

    if (arg1 <= arg2) {
	res = arg1;
    }
    else {
	res = arg2;
    }
    return res;
}

#define XA  0.8

double erfr(double rf2)
/*
   approximation of erf for x>XA 
 */
{
    static double a[5] = { 0.254829592, -0.284496736, 1.421413741,
	-1.453152027, 1.061405429
    };
    static double p = 0.3275911;
    double erf, t;


    if (rf2 > 20.)
	erf = 1.;
    else {
	t = 1. / (1. + p * rf2);
	erf = t * (a[0] + t * (a[1] + t * (a[2] + t * (a[3] + t * a[4]))));
	erf = 1. - erf * exp(-rf2 * rf2);
    }
    return (erf);
}



double crs(double x)
/*
   generating function - completely regularized spline with tension (d=3)
 */
{

    static double a[10] = {
      1.,
      -1. / 3.,
      1. / 10.,
      -1. / 42.,
      1. / (24. * 9.),
      -1. / (120. * 11.),
      1. / (720. * 13.),
      -1. / (5040. * 15.),
      1. / (40320. * 17.),
      -1. / (362880. * 19.),
    };
    static double tp = 1.1283791671 / 2.;
    double xx, res;

    if (x < XA) {
	xx = x * x;
	res =
	    tp * xx * (a[1] +
		       xx * (a[2] +
			     xx * (a[3] +
				   xx * (a[4] +
					 xx * (a[5] +
					       xx * (a[6] +
						     xx * (a[7] +
							   xx * (a[8] +
								 xx *
								 a[9]))))))));
    }
    else {
	res = erf(x) / x;
	res = res / 2. - tp;
    }
    return (res);
}


void crs_full(double x, double fi, double *crs, double *crsd, double *crsdr2, double *crsdd){
    static double a[10] = { 1., -1. / 3., 1. / 10., -1. / 42., 1. / (24. * 9.), -1. / (120. * 11.),
      1. / (720. * 13.), -1. / (5040. * 15.), 1. / (40320. * 17.), -1. / (362880. * 19.) };
    static double b[10] = { 0, -2. / 3., 4. / 10., -6. / 42., 8. / (24. * 9.), -10. / (120. * 11.),
      12. / (720. * 13.), -14. / (5040. * 15.), 16. / (40320. * 17.), -18. / (362880. * 19.) };
    static double c[10] = { 0, 0, 8. / 10., -24. / 42., 48. / (24. * 9.), -80. / (120. * 11.),
      120. / (720. * 13.), -168. / (5040. * 15.), 16. * 14. / (40320. * 17.), -18. * 16. / (362880. * 19.) };
    static double tp = 1.1283791671 / 2.;
    double xx, r, r2, fi2, fi4, fi8, tmp1, tmp2;

    fi2 = fi / 2.;
    fi4 = fi2 * fi2;
    fi8 = fi4 * fi4;

    if (x < XA) {
	xx = x * x;
	*crs =
	    tp * xx * (a[1] +
			xx * (a[2] +
			      xx * (a[3] +
				    xx * (a[4] +
					  xx * (a[5] +
						xx * (a[6] +
						      xx * (a[7] +
							    xx * (a[8] +
								  xx *
                                                                      a[9]))))))));
	if(crsd!=NULL){
          *crsd =
              tp * fi4 * (b[1] +
                          xx * (b[2] +
                                xx * (b[3] +
                                      xx * (b[4] +
                                            xx * (b[5] +
                                                  xx * (b[6] +
                                                        xx * (b[7] +
                                                              xx * (b[8] +
								  xx *
                                                                    b[9]))))))));
    }
	if(crsdr2!=NULL){
	  *crsdr2 = *crsd;
    }
	if(crsdd!=NULL){
          *crsdd =
              tp * fi8 * (c[2] +
                          xx * (c[3] +
                                xx * (c[4] +
                                      xx * (c[5] +
                                            xx * (c[6] +
                                                  xx * (c[7] +
                                                        xx * (c[8] +
                                                              xx * c[9])))))));
        }
    } else {
        tmp1 = erf(x) / x;
	*crs = tmp1 / 2. - tp;
        if((crsd!=NULL)||(crsdr2!=NULL)||(crsdd!=NULL)){
    r = 2. * x / fi;
    r2 = r * r;
          tmp2 = exp(-x * x);
    }
        if(crsd!=NULL){ *crsd   = (2. * tp * tmp2 - tmp1) / (2. * r2); }
	if(crsdr2!=NULL){ *crsdr2 = *crsd / r2; }
	/* Jarov vzorec */
	if(crsdd!=NULL){ *crsdd  = (tmp1 / r2 - tmp2 * (2 / r2 + fi * fi / 2) * tp) / (r2 * r2); }
    }
    return;
}



/*********solution of system of lin. equations*********/

int LINEQS(int DIM1, int N1, int N2, int *NERROR, double *DETERM)
/*
   solution of linear equations
   dim1 ... # of lines in matrix
   n1   ... # of columns
   n2   ... # of right hand side vectors to be solved
   a(dim1*(n1+n2)) ... matrix end rhs vector vs matrix and solutions
 */
{
    int N0, IROW, LPIV, MAIN1, N, NMIN1, I, I1, I2, I3, I4, I5;
    int DIM, PIVCOL, PIVCO1, TOPX, ENDX, TOPCOL, ENDCOL, EMAT;
    double DETER, PIVOT, SWAP;



    if (N1 == 1) {
	*NERROR = 0;
	*DETERM = A[1];
	if (A[1] == 0.) {
	    *NERROR = -1;
	    return 1;
	}
	A[2] = A[2] / A[1];
	return 1;
    }
    DIM = DIM1;
    DETER = 1.0;
    N = N1;
    EMAT = N + N2;
    NMIN1 = N - 1;
    PIVCOL = -DIM;
    /*
       MAIN LOOP TO CREATE TRIANGULAR
     */
    for (MAIN1 = 1; MAIN1 <= N; MAIN1++) {
	PIVOT = 0.;
	PIVCOL = PIVCOL + DIM + 1;
	PIVCO1 = PIVCOL + N - MAIN1;
	/*     SEARCH PIVOT     */
	for (I1 = PIVCOL; I1 <= PIVCO1; I1++) {
	    if ((fabs(A[I1]) - fabs(PIVOT)) > 0.) {
		PIVOT = A[I1];
		LPIV = I1;
	    }
	}
	/*
	   IS PIVOT DIFFERENT FROM ZERO
	 */
	if (PIVOT == 0) {
	    /*
	       ERROR EXIT
	     */
	    *NERROR = -1;
	    *DETERM = DETER;
	    return 1;
	}
	/*
	   IS IT NECESSARY TO BRING PIVOT TO DIAGONAL
	 */
	if ((LPIV - PIVCOL) != 0) {
	    DETER = -DETER;
	    LPIV = LPIV - DIM;
	    I1 = PIVCOL - DIM;
	    for (I2 = MAIN1; I2 <= EMAT; I2++) {
		LPIV = LPIV + DIM;
		I1 = I1 + DIM;
		SWAP = A[I1];
		A[I1] = A[LPIV];
		A[LPIV] = SWAP;
	    }
	}
	DETER = DETER * PIVOT;
	if (MAIN1 != N) {
	    PIVOT = 1. / PIVOT;
	    /*
	       MODIFY PIVOT COLUMN
	     */
	    I1 = PIVCOL + 1;
	    for (I2 = I1; I2 <= PIVCO1; I2++)
		A[I2] = A[I2] * PIVOT;
	    /*
	       CONVERT THE SUBMATRIX AND RIGHT SIDES
	     */
	    I3 = PIVCOL;
	    IROW = MAIN1 + 1;
	    for (I1 = IROW; I1 <= N; I1++) {
		I3 = I3 + 1;
		I4 = PIVCOL;
		I5 = I3;
		for (I2 = IROW; I2 <= EMAT; I2++) {
		    I4 = I4 + DIM;
		    I5 = I5 + DIM;
		    A[I5] = A[I5] - A[I4] * A[I3];
		}
	    }
	}
    }
    *DETERM = DETER;
    *NERROR = 0;
    /*
       COMPUTE THE SOLUTIONS
     */
    N0 = N + 1;
    TOPX = NMIN1 * DIM + 1;
    for (I = N0; I <= EMAT; I++) {
	TOPX = TOPX + DIM;
	ENDX = TOPX + N;
	TOPCOL = N * DIM + 1;
	ENDCOL = TOPCOL + NMIN1;
	for (I1 = 1; I1 <= NMIN1; I1++) {
	    ENDX = ENDX - 1;
	    TOPCOL = TOPCOL - DIM;
	    ENDCOL = ENDCOL - DIM - 1;
	    A[ENDX] = A[ENDX] / A[ENDCOL + 1];
	    SWAP = A[ENDX];
	    I3 = TOPX - 1;
	    for (I2 = TOPCOL; I2 <= ENDCOL; I2++) {
		I3 = I3 + 1;
		A[I3] = A[I3] - A[I2] * SWAP;
	    }
	}
	A[TOPX] = A[TOPX] / A[1];
    }
    /*
       LEFTADJUST THE SOLUTIONS
     */
    I = -DIM;
    TOPX = NMIN1 * DIM + 1;
    ENDX = TOPX + NMIN1;
    for (I1 = N0; I1 <= EMAT; I1++) {
	TOPX = TOPX + DIM;
	ENDX = ENDX + DIM;
	I = I + DIM;
	I3 = I;
	for (I2 = TOPX; I2 <= ENDX; I2++) {
	    I3 = I3 + 1;
	    A[I3] = A[I2];
	}
    }
    return 1;
}
