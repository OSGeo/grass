/*****************************************************************************
 *
 * MODULE:       Grass PDE Numerical Library
 * AUTHOR(S):    Soeren Gebbert, Berlin (GER) Dec 2006
 * 		soerengebbert <at> gmx <dot> de
 *               
 * PURPOSE:      Unit tests for les solving
 *
 * COPYRIGHT:    (C) 2000 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grass/glocale.h>
#include <grass/gmath.h>
#include "test_gmath_lib.h"

#define EPSILON_DIRECT 1.0E-10
#define EPSILON_ITER 1.0E-4

/* prototypes */
static int test_matrix_conversion(void);

/* ************************************************************************* */
/* Performe the solver unit tests ****************************************** */
/* ************************************************************************* */
int unit_test_matrix_conversion(void)
{
	int sum = 0;

	G_message(_("\n++ Running matrix conversion unit tests ++"));

	sum += test_matrix_conversion();

	if (sum > 0)
		G_warning(_("\n-- Matrix conversion unit tests failure --"));
	else
		G_message(_("\n-- Matrix conversion unit tests finished successfully --"));

	return sum;
}

/* ************************************************************************* */
/* ************************************************************************* */
/* ************************************************************************* */

static void print_matrix(double **A, int rows, int cols)
{
   int i, j;
   for(i = 0; i < rows; i++) {
      for(j = 0; j < cols; j++) {
	printf("%g ", A[i][j]);
      }
      printf("\n");
   }
}

/* *************************************************************** */
/* Test all implemented solvers for sparse and normal matrix *** */
/* *************************************************************** */
int test_matrix_conversion(void)
{
	int sum = 0;
	int i, j;
	double val = 0.0;
	double **A;
	double **B;
	double **C;
	double **D;
	double **E;
	double **F;
	G_math_spvector **Asp;
	G_math_spvector **Asp2;

	A = G_alloc_matrix(5,5);
	F = G_alloc_matrix(5,5);
	G_message("\t * Creating symmetric matrix\n");

	A[0][0] = 8;
	A[1][1] = 7;
	A[2][2] = 6;
	A[3][3] = 5;
	A[4][4] = 4;
	
	A[0][1] = 3;
	A[0][2] = 0;
	A[0][3] = 1;
	A[0][4] = 0;
	
	A[1][2] = 3;
	A[1][3] = 0;
	A[1][4] = 1;
	
	A[2][3] = 3;
	
	A[3][4] = 3;

	for(i = 0; i < 5; i++)
	   for(j = 0; j < 5; j++)
	      A[j][i] = A[i][j];

	print_matrix(A, 5, 5);

	G_message("\t * Test matrix to band matrix conversion\n");

        B = G_math_matrix_to_sband_matrix(A, 5, 4);

	print_matrix(B, 5, 4);

	G_message("\t * Test matrix to sparse matrix conversion\n");

        Asp = G_math_A_to_Asp(A, 5, 0.0);

	G_math_print_spmatrix(Asp, 5);

	G_message("\t * Test sparse matrix to matrix conversion\n");

        C = G_math_Asp_to_A(Asp, 5);

	print_matrix(C, 5, 5);

	G_message("\t * Test sparse matrix to band matrix conversion\n");

        D = G_math_Asp_to_sband_matrix(Asp, 5, 4);

	print_matrix(D, 5, 4);

	/*Check the band matrix results*/
	G_math_d_aA_B(B, D, -1, F, 5, 4);
	G_math_d_asum_norm(F[0], &val, 20);
	   
	if (val != 0)
	{
		G_warning("Error in band matrix conversion");
		sum++;
	}

	G_message("\t * Test band matrix to matrix conversion\n");

        E = G_math_sband_matrix_to_matrix(D, 5, 4);

	print_matrix(E, 5, 5);

	/*Check the matrix results*/
	G_math_d_aA_B(A, E, -1, F, 5, 5);
	G_math_d_asum_norm(F[0], &val, 25);

	if (val != 0)
	{
		G_warning("Error in matrix conversion");
		sum++;
	}

	G_message("\t * Test band matrix to sparse matrix conversion\n");

        Asp2 = G_math_sband_matrix_to_Asp(D, 5, 4, 0.0);
	G_math_print_spmatrix(Asp2, 5);

	return sum;
}

