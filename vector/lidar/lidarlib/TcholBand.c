#include <stdlib.h>				/* imported libraries */
#include <stdio.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/PolimiFunct.h>

/*--------------------------------------------------------------------------------------*/
/* Tcholetsky decomposition -> T= Lower Triangular Matrix */

void tcholDec (double **N, double **T, int n, int BW)
{
	int		i, j, k;
	double		somma;
	
	for (i = 0; i < n; i++) {
	    for (j = 0; j < BW; j++) {
		somma = N[i][j];
		for (k = 1; k < BW; k++) 
		    if (((i-k) >= 0) && ((j+k) < BW)) somma -= T[i-k][k] * T[i-k][j+k]; 
		if  (j == 0) {
		    if (somma <= 0.0) G_fatal_error (_("Decomposition failed"));
		    T[i][0] = sqrt (somma);
		}
		else T[i][j] = somma / T[i][0];
	    }
	}
	
	return;
}

/*--------------------------------------------------------------------------------------*/
/* Tcholetsky matrix solution */

void tcholSolve (double **N, double *TN, double *parVect, int n, int BW) 
{

	double		**T;
	int		i, j;

	/*--------------------------------------*/
	T = G_alloc_matrix (n, BW);

	/*--------------------------------------*/
	tcholDec (N, T, n, BW);					/* T computation		*/

	/* Forward substitution */
	parVect[0] = TN[0] / T[0][0];
	for (i = 1; i < n; i++) {
		parVect[i] = TN[i];
		for (j = 0; j < i; j++)
			if ((i-j) < BW) parVect[i] -= T[j][i-j] * parVect[j];
		parVect[i] = parVect[i] / T[i][0];
	}

	/* Backward substitution */
	parVect[n-1] = parVect[n-1] / T[n-1][0];
	for (i = n-2; i >= 0; i--) {
		for (j = i+1; j < n; j++)
			if ((j-i) < BW) parVect[i] -= T[i][j-i] * parVect[j];
		parVect[i] = parVect[i] / T[i][0];
	}

	/*--------------------------------------*/
	G_free_matrix (T);
	
	return;
}


/*--------------------------------------------------------------------------------------*/
/* Soluzione con Tcholetsky -> la matrice T triangolare viene passata come paramtero e 
	non calcolata internamente alla procedura -> T = dmatrix (0, n-1, 0, BW-1) */

void tcholSolve2 (double **N, double *TN, double **T, double *parVect, int n, int BW) 
{

	int		i, j;

	/* Forward substitution */
	parVect[0] = TN[0] / T[0][0];
	for (i = 1; i < n; i++) {
		parVect[i] = TN[i];
		for (j = 0; j < i; j++)
			if ((i-j) < BW) parVect[i] -= T[j][i-j] * parVect[j];
		parVect[i] = parVect[i] / T[i][0];
	}

	/* Backward substitution */
	parVect[n-1] = parVect[n-1] / T[n-1][0];
	for (i = n-2; i >= 0; i--) {
		for (j = i+1; j < n; j++)
			if ((j-i) < BW) parVect[i] -= T[i][j-i] * parVect[j];
		parVect[i] = parVect[i] / T[i][0];
	}
	
	return;
}

/*--------------------------------------------------------------------------------------*/
/* Tcholetsky matrix invertion*/

void tcholInv (double **N, double *invNdiag, int n, int BW)
{
	double		**T=NULL; 
	double 		*vect=NULL;
	int		i, j, k;
	double		somma;

	/*--------------------------------------*/
	G_alloc_matrix (n,BW);
	G_alloc_vector (n);

	/* T computation		*/
	tcholDec (N, T, n, BW);

	/* T Diagonal invertion */
	for (i = 0; i < n; i++) {
		T[i][0] = 1.0 / T[i][0];
	}

	/* N Diagonal invertion */
	for (i = 0; i < n; i++) {
		vect[0] = T[i][0];
		invNdiag[i] = vect[0] * vect[0];
		for (j = i+1; j < n; j++) {
			somma = 0.0;
			for (k = i; k < j; k++) { 
				if ((j-k) < BW) somma -= vect[k-i] * T[k][j-k]; 
			}
			vect[j-i] = somma * T[j][0];
			invNdiag[i] += vect[j-i] * vect[j-i];
		}
	}

	/*--------------------------------------*/
	G_free_matrix (T);
	G_free_vector (vect);
	
	return;
}

/*--------------------------------------------------------------------------------------*/
/* Tcholetsky matrix solution and invertion*/

void tcholSolveInv (double **N, double *TN, double *invNdiag, double *parVect, int n, int BW) 
{

	double		**T=NULL;
	double		*vect=NULL;
	int		i, j, k;
	double		somma;

	/*--------------------------------------*/
	G_alloc_matrix (n, BW);
	G_alloc_vector (n);

	/* T computation		*/
	tcholDec (N, T, n, BW);

	/* Forward substitution */
	parVect[0] = TN[0] / T[0][0];
	for (i = 1; i < n; i++) {
		parVect[i] = TN[i];
		for (j = 0; j < i; j++)
			if ((i-j) < BW) parVect[i] -= T[j][i-j] * parVect[j];
		parVect[i] = parVect[i] / T[i][0];
	}

	/* Backward substitution */
	parVect[n-1] = parVect[n-1] / T[n-1][0];
	for (i = n-2; i >= 0; i--) {
		for (j = i+1; j < n; j++)
			if ((j-i) < BW) parVect[i] -= T[i][j-i] * parVect[j];
		parVect[i] = parVect[i] / T[i][0];
	}

	/* T Diagonal invertion */
	for (i = 0; i < n; i++) {
		T[i][0] = 1.0 / T[i][0];
	}

	/* N Diagonal invertion */
	for (i = 0; i < n; i++) {
		vect[0] = T[i][0];
		invNdiag[i] = vect[0] * vect[0];
		for (j = i+1; j < n; j++) {
			somma = 0.0;
			for (k = i; k < j; k++) { 
				if ((j-k) < BW) somma -= vect[k-i] * T[k][j-k]; 
			}
			vect[j-i] = somma * T[j][0];
			invNdiag[i] += vect[j-i] * vect[j-i];
		}
	}

	/*--------------------------------------*/
	G_free_matrix (T);
	G_free_vector (vect);
	
	return;
}

