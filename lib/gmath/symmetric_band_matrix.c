#include <stdlib.h>		
#include <stdio.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/gmath.h>
#include <grass/glocale.h>

/**
 * \brief Convert a symmetrix matrix into a symmetric band matrix
 *
 * \verbatim
 Symmetric matrix with bandwidth of 3

 5 2 1 0
 2 5 2 1
 1 2 5 2
 0 1 2 5

 will be converted into the symmetric band matrix
 
 5 2 1
 5 2 1
 5 2 0
 5 0 0

  \endverbatim

 * \param A (double**) the symmetric matrix
 * \param rows (int)
 * \param bandwidth (int)
 * \return B (double**) new created symmetric band matrix 
 * */

double **G_math_matrix_to_sband_matrix(double **A, int rows, int bandwidth)
{
    int i, j;
    double **B = G_alloc_matrix(rows, bandwidth);
    double tmp;

    for(i = 0; i < rows; i++) {
       for(j = 0; j < bandwidth; j++) {
          if(i + j < rows) {
            tmp = A[i][i + j]; 
            B[i][j] = tmp;
          } else {
            B[i][j] = 0.0;
          }
       }
    }

    return B;
}


/**
 * \brief Convert a symmetric band matrix into a symmetric matrix
 *
 * \verbatim
 Such a symmetric band matrix with banwidth 3
 
 5 2 1
 5 2 1
 5 2 0
 5 0 0

 Will be converted into this symmetric matrix

 5 2 1 0
 2 5 2 1
 1 2 5 2
 0 1 2 5

  \endverbatim
 * \param A (double**) the symmetric band matrix
 * \param rows (int)
 * \param bandwidth (int)
 * \return B (double**) new created symmetric matrix 
 * */

double **G_math_sband_matrix_to_matrix(double **A, int rows, int bandwidth)
{
    int i, j;
    double **B = G_alloc_matrix(rows, rows);
    double tmp;

    for(i = 0; i < rows; i++) {
       for(j = 0; j < bandwidth; j++) {
          tmp = A[i][j];
          if(i + j < rows) {
            B[i][i + j] = tmp; 
          } 
       }
    }

    /*Symmetry*/
    for(i = 0; i < rows; i++) {
       for(j = i; j < rows; j++) {
          B[j][i] = B[i][j]; 
       }
    }

    return B;
}


/*!
 * \brief Compute the matrix - vector product  
 * of symmetric band matrix A and vector x.
 *
 * This function is multi-threaded with OpenMP and can be called within a parallel OpenMP region.
 *
 * y = A * x
 *
 *
 * \param A (double **) 
 * \param x (double) *)
 * \param y (double * )
 * \param rows (int)
 * \param bandwidth (int)
 * \return (void)
 *
 * */
void G_math_Ax_sband(double ** A, double *x, double *y, int rows, int bandwidth)
{
    int i, j;
    double tmp;


#pragma omp for schedule (static) private(i, j, tmp)
    for (i = 0; i < rows; i++) {
	tmp = 0;
	for (j = 0; j < bandwidth; j++) {
	   if((i + j) < rows)
	   	tmp += A[i][j]*x[i + j];
	}
	y[i] = tmp;
    }

#pragma omp single
    {
    for (i = 0; i < rows; i++) {
	tmp = 0;
	for (j = 1; j < bandwidth; j++) {
	   	if(i + j < rows)
			y[i + j] += A[i][j]*x[i];
	}
    }
    }
    return;
}
