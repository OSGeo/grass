/***********************************************************************
 *
 * MODULE:       lidarlib
 *
 * AUTHOR(S):    Roberto Antolin
 *
 * PURPOSE:      LIDAR Spline Interpolation
 *
 * COPYRIGHT:    (C) 2006 by Politecnico di Milano -
 *                           Polo Regionale di Como
 *
 *               This program is free software under the
 *               GNU General Public License (>=v2).
 *               Read the file COPYING that comes with GRASS
 *               for details.
 *
 **************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <math.h>
#include <string.h>

#include "PolimiFunct.h"

/*----------------------------------------------------------------------------*/
/* Abscissa node index computation */

void node_x (double x, int *i_x, double *csi_x,double xMin,double deltaX) {

	*i_x   = (int)    ((x - xMin) / deltaX);
	*csi_x = (double) ((x - xMin) - (*i_x * deltaX));

	return;
}

/*----------------------------------------------------------------------------*/
/* Ordinate node index computation */

void node_y (double y, int *i_y, double *csi_y,double yMin,double deltaY) {

	*i_y   = (int)    ((y - yMin) / deltaY);
	*csi_y = (double) ((y - yMin) - (*i_y * deltaY));
	
	return;
}

/*----------------------------------------------------------------------------*/
/* Node order computation */

int order (int i_x, int i_y,int yNum) {

	return (i_y + i_x * yNum);
}

/*----------------------------------------------------------------------------*/
/* Design matrix coefficients computation */

double phi_3 (double csi) {

	return ((pow (2 - csi, 3.) - pow (1 - csi, 3.) * 4) / 6.);
}

double phi_4 (double csi) {

	return (pow (2 - csi, 3.) / 6.);
}

double phi_33 (double csi_x, double csi_y) {

	return (phi_3 (csi_x) * phi_3 (csi_y));
}

double phi_34 (double csi_x, double csi_y) {

	return (phi_3 (csi_x) * phi_4 (csi_y));
}

double phi_43 (double csi_x, double csi_y) {

	return (phi_4 (csi_x) * phi_3 (csi_y));
}

double phi_44 (double csi_x, double csi_y) {

	return (phi_4 (csi_x) * phi_4 (csi_y));
}

double phi (double csi_x, double csi_y) {

	return ((1-csi_x)*(1-csi_y));
}

/*----------------------------------------------------------------------------*/
/* Normal system computation for bicubic spline interpolation */

void normalDefBicubic (double **N,double *TN,double *Q,double **obsVect,double deltaX,
		       double deltaY,int xNum,int yNum,double xMin,double yMin,
		       int obsNum,int parNum,int BW) {

	int 	i, k, h, m, n, n0;			/* counters		*/
	double	alpha[4][4];				/* coefficients */

	int 	i_x;					/* x = (xMin + (i_x * deltaX) + csi_x) */
	double	csi_x;

	int 	i_y;					/* y = (yMin + (i_y * deltaY) + csi_y) */
	double	csi_y;

	/*--------------------------------------*/
	for (k = 0; k < parNum; k++) {
		for (h = 0; h < BW; h++) 
			N[k][h] = 0.;			/* Normal matrix inizialization */
		TN[k] = 0.;				/* Normal vector inizialization */
	}
	/*--------------------------------------*/

	for (i = 0; i < obsNum; i++) {

	    node_x (obsVect[i][0], &i_x, &csi_x, xMin, deltaX);
	    node_y (obsVect[i][1], &i_y, &csi_y, yMin, deltaY);

	    if ((i_x >= -2) && (i_x <= xNum) && (i_y >= -2) && (i_y <= yNum)) {

		csi_x = csi_x / deltaX;
		csi_y = csi_y / deltaY;

		alpha[0][0] = phi_44 (1 + csi_x, 1 + csi_y);
		alpha[0][1] = phi_43 (1 + csi_x, csi_y);
		alpha[0][2] = phi_43 (1 + csi_x, 1 - csi_y);
		alpha[0][3] = phi_44 (1 + csi_x, 2 - csi_y);

		alpha[1][0] = phi_34 (csi_x, 1 + csi_y);
		alpha[1][1] = phi_33 (csi_x, csi_y);
		alpha[1][2] = phi_33 (csi_x, 1 - csi_y);
		alpha[1][3] = phi_34 (csi_x, 2 - csi_y);

		alpha[2][0] = phi_34 (1 - csi_x, 1 + csi_y);
		alpha[2][1] = phi_33 (1 - csi_x, csi_y);
		alpha[2][2] = phi_33 (1 - csi_x, 1 - csi_y);
		alpha[2][3] = phi_34 (1 - csi_x, 2 - csi_y);

		alpha[3][0] = phi_44 (2 - csi_x, 1 + csi_y);
		alpha[3][1] = phi_43 (2 - csi_x, csi_y);
		alpha[3][2] = phi_43 (2 - csi_x, 1 - csi_y);
		alpha[3][3] = phi_44 (2 - csi_x, 2 - csi_y);

		for (k = -1; k <= 2; k++) {
		    for (h = -1; h <= 2; h++) {

			if (((i_x + k) >= 0) && ((i_x + k) < xNum) && ((i_y + h) >= 0) && ((i_y + h) < yNum)) {
			    for (m = k; m <= 2; m++) {

				if (m == k) n0 = h;
				else n0 = -1;

				for (n = n0; n <= 2; n++) {
				    if (((i_x + m) >= 0) && ((i_x + m) < xNum) && ((i_y + n) >= 0) && ((i_y + n) < yNum)) {
					N[order(i_x + k, i_y + h,yNum)][order(i_x + m, i_y + n,yNum) - \
					order(i_x + k, i_y + h,yNum)] += alpha[k + 1][h + 1] *(1 / Q[i]) * alpha[m + 1][n + 1];
					/* 1/Q[i] only refers to the variances */
				    }
				}
			    }
			    TN[order(i_x + k, i_y + h,yNum)] += obsVect[i][2] * (1 / Q[i]) * alpha[k + 1][h + 1];
			}
		    }
		}
	    }
	}
	
	return;
}

/*----------------------------------------------------------------------------*/
/* Normal system correction - Introduzione della correzione dovuta alle 
	pseudosservazioni (Tykonov) - LAPALCIANO - */

void nCorrectLapl (double **N,double lambda,int xNum,int yNum,double deltaX,double deltaY) {

	int	i_x, i_y;			/* counters	*/
	int	k, h, m, n, n0;			/* counters	*/
	
	double	alpha[5][5];			/* coefficients */

	double	lambdaX, lambdaY;

	/*--------------------------------------*/
	lambdaX = lambda * (deltaY / deltaX);
	lambdaY = lambda * (deltaX / deltaY);

	alpha[0][0] =   0;
	alpha[0][1] =   lambdaX * (1 / 36.);		/* There is lambda because Q^(-1) contains 1/(1/lambda)*/
	alpha[0][2] =   lambdaX * (1 /  9.);
	alpha[0][3] =   lambdaX * (1 / 36.);
	alpha[0][4] =   0;

	alpha[1][0] =   lambdaY * (1 / 36.);
	alpha[1][1] =   lambdaX * (1 / 18.) + lambdaY * (1 / 18.);
	alpha[1][2] =   lambdaX * (2 /  9.) - lambdaY * (1 /  6.);
	alpha[1][3] =   lambdaX * (1 / 18.) + lambdaY * (1 / 18.);
	alpha[1][4] =   lambdaY * (1 / 36.);

	alpha[2][0] =   lambdaY * (1 /  9.);
	alpha[2][1] = - lambdaX * (1 /  6.) + lambdaY * (2 /  9.);
	alpha[2][2] = - lambdaX * (2 /  3.) - lambdaY * (2 /  3.);
	alpha[2][3] = - lambdaX * (1 /  6.) + lambdaY * (2 /  9.);
	alpha[2][4] =   lambdaY * (1 /  9.);

	alpha[3][0] =   lambdaY * (1 / 36.);
	alpha[3][1] =   lambdaX * (1 / 18.) + lambdaY * (1 / 18.);
	alpha[3][2] =   lambdaX * (2 /  9.) - lambdaY * (1 /  6.);
	alpha[3][3] =   lambdaX * (1 / 18.) + lambdaY * (1 / 18.);
	alpha[3][4] =   lambdaY * (1 / 36.);

	alpha[4][0] =   0;
	alpha[4][1] =   lambdaX * (1 / 36.);
	alpha[4][2] =   lambdaX * (1 /  9.);
	alpha[4][3] =   lambdaX * (1 / 36.);
	alpha[4][4] =   0;

	for (i_x = 0; i_x < xNum; i_x++) {
	    for (i_y = 0; i_y < yNum; i_y++) {

		for (k = -2; k <= 2; k++) {
		    for (h = -2; h <= 2; h++) {

			if (((i_x + k) >= 0) && ((i_x + k) < xNum) && ((i_y + h) >= 0) && ((i_y + h) < yNum)) {

			    for (m = k; m <= 2; m++) {

				if (m == k) n0 = h;
				else n0 = -2;

				for (n = n0; n <= 2; n++) {
				    if (((i_x + m) >= 0) && ((i_x + m) <= (xNum - 1)) && ((i_y + n) >= 0) && ((i_y + n) <= (yNum - 1))) {

					if ((alpha[k + 2][h + 2] != 0) && (alpha[m + 2][n + 2] != 0)) {
					    N[order(i_x + k, i_y + h,yNum)][order(i_x + m, i_y + n,yNum) -\
					    order(i_x + k, i_y + h,yNum)]+=alpha[k + 2][h + 2] * alpha[m + 2][n + 2];
					}
				    }
				}
			    }
			}
		    }
		}
	    }
	}
	
	return;
}

/*----------------------------------------------------------------------------*/
/* Normal system computation for bilinear spline interpolation */

void normalDefBilin (double **N,double *TN,double *Q,double **obsVect,double deltaX,
		     double deltaY,int xNum,int yNum,double xMin,double yMin,
		     int obsNum,int parNum,int BW) {

	int 	i, k, h, m, n, n0;			/* counters	*/
	double	alpha[2][2];				/* coefficients */

	int 	i_x;					/* x = (xMin + (i_x * deltaX) + csi_x) */
	double	csi_x;

	int 	i_y;					/* y = (yMin + (i_y * deltaY) + csi_y) */
	double	csi_y;

	/*--------------------------------------*/
	for (k = 0; k < parNum; k++) {
		for (h = 0; h < BW; h++) 
			N[k][h] = 0.;			/* Normal matrix inizialization */
		TN[k] = 0.;				/* Normal vector inizialization */
	}
	/*--------------------------------------*/

	for (i = 0; i < obsNum; i++) {

	    node_x (obsVect[i][0], &i_x, &csi_x, xMin, deltaX);
	    node_y (obsVect[i][1], &i_y, &csi_y, yMin, deltaY);

	    if ((i_x >= -1) && (i_x < xNum) && (i_y >= -1) && (i_y < yNum)) {

		csi_x = csi_x / deltaX;
		csi_y = csi_y / deltaY;

		alpha[0][0] = phi (csi_x, csi_y);
		alpha[0][1] = phi (csi_x, 1 - csi_y);
		alpha[1][0] = phi (1 - csi_x, csi_y);
		alpha[1][1] = phi (1 - csi_x, 1 - csi_y);

		for (k = 0; k <= 1; k++) {
		    for (h = 0; h <= 1; h++) {

			if (((i_x + k) >= 0) && ((i_x + k) <= (xNum - 1)) && ((i_y + h) >= 0) && ((i_y + h) <= (yNum - 1))) {

			    for (m = k; m <= 1; m++) {
				if (m == k) n0 = h;
				else n0 = 0;

				for (n = n0; n <= 1; n++) {
				    if (((i_x + m) >= 0) && ((i_x + m) < xNum) && ((i_y + n) >= 0) && ((i_y + n) < yNum)) {
					N[order(i_x + k, i_y + h,yNum)][order(i_x + m, i_y + n,yNum) - \
					order(i_x + k, i_y + h,yNum)] += alpha[k][h]* (1 / Q[i]) * alpha[m][n];
					/* 1/Q[i] only refers to the variances */
				    }
				 }
			    }
			    TN[order(i_x + k, i_y + h,yNum)] += obsVect[i][2]*(1/Q[i])*alpha[k][h];
			}
		    }
		}
	    }
	}
	
	return;
}

		
/*----------------------------------------------------------------------------*/
/* Normal system correction - Introduzione della correzione dovuta alle 
	pseudosservazioni (Tykonov) - GRADIENTE - */
#ifdef notdef
void nCorrectGrad (double **N,double lambda,int xNum,int yNum,double deltaX,double deltaY) {

	int i;
	int parNum;

	double alpha[3];
	double lambdaX,lambdaY;

	lambdaX = lambda * (deltaY/deltaX);
	lambdaY = lambda * (deltaX/deltaY);

	parNum = xNum * yNum;

	alpha[0] =   lambdaX / 2. + lambdaY / 2.;
	alpha[1] = - lambdaX / 4.;
        alpha[2] = - lambdaY / 4.;
	
	for (i = 0; i < parNum; i++) {
		N[i][0] += alpha[0];

		if ((i + 2) < parNum)
			N[i][2] += alpha[2];
		
		if ((i + 2 * yNum) < parNum)
			N[i][2 * yNum] += alpha[1];
	}
}
#endif

/*1-DELTA discretization*/
void nCorrectGrad (double **N,double lambda,int xNum,int yNum,double deltaX,double deltaY) {

	int i;
	int parNum;

	double alpha[3];
	double lambdaX,lambdaY;

	lambdaX = lambda * (deltaY/deltaX);
	lambdaY = lambda * (deltaX/deltaY);

	parNum = xNum * yNum;

	alpha[0] =   2 * lambdaX  + 2 * lambdaY;
	alpha[1] = - lambdaX;
	alpha[2] = - lambdaY;

	for (i = 0; i < parNum; i++) {
		N[i][0] += alpha[0];

		if ((i + 1) < parNum)
			N[i][1] += alpha[2];

		if ((i + 1 * yNum) < parNum)
			N[i][1 * yNum] += alpha[1];
	}
	
	return;
}

/*----------------------------------------------------------------------------*/
/* Observations estimation */

void obsEstimateBicubic (double **obsV,double *obsE,double *parV,double deltX,double deltY,
		  	 int xNm,int yNm,double xMi,double yMi,int obsN) {

	int		i, k, h;		/* counters	*/
	double		alpha[4][4];		/* coefficients */

	int		i_x;			/* x = (xMin + (i_x * deltaX) + csi_x) */
	double		csi_x;

	int		i_y;			/* y = (yMin + (i_y * deltaY) + csi_y) */
	double		csi_y;

	for (i = 0; i < obsN; i++) {

	    obsE[i] = 0;

	    node_x (obsV[i][0], &i_x, &csi_x, xMi, deltX);
	    node_y (obsV[i][1], &i_y, &csi_y, yMi, deltY);

	    if ((i_x >= -2) && (i_x <= xNm) && (i_y >= -2) && (i_y <= yNm)) {

		csi_x = csi_x / deltX;
		csi_y = csi_y / deltY;

		alpha[0][0] = phi_44 (1 + csi_x, 1 + csi_y);
		alpha[0][1] = phi_43 (1 + csi_x, csi_y);
		alpha[0][2] = phi_43 (1 + csi_x, 1 - csi_y);
		alpha[0][3] = phi_44 (1 + csi_x, 2 - csi_y);

		alpha[1][0] = phi_34 (csi_x, 1 + csi_y);
		alpha[1][1] = phi_33 (csi_x, csi_y);
		alpha[1][2] = phi_33 (csi_x, 1 - csi_y);
		alpha[1][3] = phi_34 (csi_x, 2 - csi_y);

		alpha[2][0] = phi_34 (1 - csi_x, 1 + csi_y);
		alpha[2][1] = phi_33 (1 - csi_x, csi_y);
		alpha[2][2] = phi_33 (1 - csi_x, 1 - csi_y);
		alpha[2][3] = phi_34 (1 - csi_x, 2 - csi_y);

		alpha[3][0] = phi_44 (2 - csi_x, 1 + csi_y);
		alpha[3][1] = phi_43 (2 - csi_x, csi_y);
		alpha[3][2] = phi_43 (2 - csi_x, 1 - csi_y);
		alpha[3][3] = phi_44 (2 - csi_x, 2 - csi_y);

		for (k = -1; k <= 2; k++) {
		    for (h = -1; h <= 2; h++) {
			if (((i_x + k) >= 0) && ((i_x + k) < xNm) && ((i_y + h) >= 0) && ((i_y + h) < yNm)) 
			    obsE[i] += parV[order(i_x + k, i_y + h,yNm)] * alpha[k + 1][h + 1];
		    }
		}
	    }
	}
	
	return;
}


/*--------------------------------------------------------------------------------------*/
/* Data interpolation in a generic point */
 
double dataInterpolateBicubic (double x, double y,double deltaX,double deltaY,int xNum,int yNum,
			       double xMin,double yMin,double *parVect) {

	double		z;			/* abscissa, ordinate and associated value */

	int		k, h;			/* counters		*/
	double		alpha[4][4];		/* coefficients */

	int		i_x, i_y;		/* x = (xMin + (i_x * deltaX) + csi_x) */
	double		csi_x, csi_y;		/* y = (yMin + (i_y * deltaY) + csi_y) */


	z = 0;

	node_x (x, &i_x, &csi_x, xMin, deltaX);
	node_y (y, &i_y, &csi_y, yMin, deltaY);

	if ((i_x >= -2) && (i_x <= xNum) && (i_y >= -2) && (i_y <= yNum)) {

		csi_x = csi_x / deltaX;
		csi_y = csi_y / deltaY;

		alpha[0][0] = phi_44 (1 + csi_x, 1 + csi_y);
		alpha[0][1] = phi_43 (1 + csi_x, csi_y);
		alpha[0][2] = phi_43 (1 + csi_x, 1 - csi_y);
		alpha[0][3] = phi_44 (1 + csi_x, 2 - csi_y);
			
		alpha[1][0] = phi_34 (csi_x, 1 + csi_y);
		alpha[1][1] = phi_33 (csi_x, csi_y);
		alpha[1][2] = phi_33 (csi_x, 1 - csi_y);
		alpha[1][3] = phi_34 (csi_x, 2 - csi_y);
			
		alpha[2][0] = phi_34 (1 - csi_x, 1 + csi_y);
		alpha[2][1] = phi_33 (1 - csi_x, csi_y);
		alpha[2][2] = phi_33 (1 - csi_x, 1 - csi_y);
		alpha[2][3] = phi_34 (1 - csi_x, 2 - csi_y);
			
		alpha[3][0] = phi_44 (2 - csi_x, 1 + csi_y);
		alpha[3][1] = phi_43 (2 - csi_x, csi_y);
		alpha[3][2] = phi_43 (2 - csi_x, 1 - csi_y);
		alpha[3][3] = phi_44 (2 - csi_x, 2 - csi_y);

		for (k = -1; k <= 2; k++) {
		    for (h = -1; h <= 2; h++) {
			if (((i_x + k) >= 0) && ((i_x + k) < xNum) && ((i_y + h) >= 0) && ((i_y + h) < yNum)) 
			    z += parVect[order(i_x + k, i_y + h, yNum)] * alpha[k + 1][h + 1];	
		    }
		}
	}

	return z;
}


/*----------------------------------------------------------------------------*/
/* Observations estimation */

void obsEstimateBilin (double **obsV,double *obsE,double *parV,double deltX,double deltY,
		       int xNm,int yNm,double xMi,double yMi,int obsN) {

	int		i, k, h;		/* counters	*/
	double		alpha[2][2];		/* coefficients */

	int		i_x;			/* x = (xMin + (i_x * deltaX) + csi_x) */
	double		csi_x;

	int		i_y;			/* y = (yMin + (i_y * deltaY) + csi_y) */
	double		csi_y;

	for (i = 0; i < obsN; i++) {

	    obsE[i] = 0;

	    node_x (obsV[i][0], &i_x, &csi_x, xMi, deltX);
	    node_y (obsV[i][1], &i_y, &csi_y, yMi, deltY);

	    if ((i_x >= -1) && (i_x < xNm) && (i_y >= -1) && (i_y < yNm)) {

		csi_x = csi_x / deltX;
		csi_y = csi_y / deltY;

		alpha[0][0] = phi (csi_x, csi_y);
		alpha[0][1] = phi (csi_x, 1 - csi_y);
		alpha[1][0] = phi (1 - csi_x, csi_y);
		alpha[1][1] = phi (1 - csi_x, 1 - csi_y);

		for (k = 0; k <= 1; k++) {
		    for (h = 0; h <= 1; h++) {
			if (((i_x + k) >= 0) && ((i_x + k) < xNm) && ((i_y + h) >= 0) && ((i_y + h) < yNm)) 
			    obsE[i] += parV[order(i_x + k, i_y + h,yNm)] * alpha[k][h];   
		    }
		}
	    }
	}
	
	return;
}


/*--------------------------------------------------------------------------------------*/
/* Data interpolation in a generic point */

double dataInterpolateBilin (double x, double y,double deltaX,double deltaY,int xNum,int yNum,
			     double xMin,double yMin,double *parVect) {

	double		z;			/* abscissa, ordinate and associated value */

	int		k, h;			/* counters		*/
	double		alpha[2][2];		/* coefficients */

	int		i_x, i_y;		/* x = (xMin + (i_x * deltaX) + csi_x) */
	double		csi_x, csi_y;		/* y = (yMin + (i_y * deltaY) + csi_y) */

	z = 0;

	node_x (x, &i_x, &csi_x, xMin, deltaX);
	node_y (y, &i_y, &csi_y, yMin, deltaY);

	if ((i_x >= -1) && (i_x < xNum) && (i_y >= -1) && (i_y < yNum)) {

		csi_x = csi_x / deltaX;
		csi_y = csi_y / deltaY;

		alpha[0][0] = phi (csi_x, csi_y);
		alpha[0][1] = phi (csi_x, 1 - csi_y);
			
		alpha[1][0] = phi (1 - csi_x, csi_y);
		alpha[1][1] = phi (1 - csi_x, 1 - csi_y);

		for (k = 0; k <= 1; k++) {
		    for (h = 0; h <= 1; h++) {
			if (((i_x + k) >= 0) && ((i_x + k) < xNum) && ((i_y + h) >= 0) && ((i_y + h) < yNum)) 
			    z += parVect[order(i_x + k, i_y + h, yNum)] * alpha[k][h];
		    }
		}
	}

	return z;
}

