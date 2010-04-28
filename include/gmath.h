/******************************************************************************
 * gmath.h
 * Top level header file for gmath units

 * @Copyright David D.Gray <ddgray@armadce.demon.co.uk>
 * 27th. Sep. 2000
 * Last updated: $Id$
 *

 * This file is part of GRASS GIS. It is free software. You can 
 * redistribute it and/or modify it under the terms of 
 * the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 ******************************************************************************/

#ifndef GMATH_H_
#define GMATH_H_

#include <grass/config.h>
#if defined(HAVE_LIBLAPACK) && defined(HAVE_LIBBLAS) && defined(HAVE_G2C_H)
 /* only include if available */
#include <grass/la.h>
#endif
#include <stddef.h>

/*solver names */
#define G_MATH_SOLVER_DIRECT_GAUSS "gauss"
#define G_MATH_SOLVER_DIRECT_LU "lu"
#define G_MATH_SOLVER_DIRECT_CHOLESKY "cholesky"
#define G_MATH_SOLVER_ITERATIVE_JACOBI "jacobi"
#define G_MATH_SOLVER_ITERATIVE_SOR "sor"
#define G_MATH_SOLVER_ITERATIVE_CG "cg"
#define G_MATH_SOLVER_ITERATIVE_PCG "pcg"
#define G_MATH_SOLVER_ITERATIVE_BICGSTAB "bicgstab"

/*preconditioner */
#define G_MATH_DIAGONAL_PRECONDITION 1
#define G_MATH_ROWSCALE_ABSSUMNORM_PRECONDITION 2
#define G_MATH_ROWSCALE_EUKLIDNORM_PRECONDITION 3
#define G_MATH_ROWSCALE_MAXNORM_PRECONDITION 4

/* dalloc.c */
double *G_alloc_vector(size_t);
double **G_alloc_matrix(int, int);
float  *G_alloc_fvector(size_t);
float  **G_alloc_fmatrix(int, int);
void G_free_vector(double *);
void G_free_matrix(double **);
void G_free_fvector(float *);
void G_free_fmatrix(float **);

/* ialloc.c */
int *G_alloc_ivector(size_t);
int **G_alloc_imatrix(int, int);
void G_free_ivector(int *);
void G_free_imatrix(int **);

/* fft.c */
extern int fft(int, double *[2], int, int, int);
extern int fft2(int, double (*)[2], int, int, int);

/* gauss.c */
extern double G_math_rand_gauss(int, double);

/* max_pow2.c */
extern long G_math_max_pow2 (long n);
extern long G_math_min_pow2 (long n);

/* rand1.c */
extern float G_math_rand(int);

/* del2g.c */
extern int del2g(double *[2], int, double);

/* getg.c */
extern int getg(double, double *[2], int);

/* eigen_tools.c */
extern int G_math_egvorder(double *, double **, long);

/* mult.c */
extern int G_math_complex_mult (double *v1[2], int size1, double *v2[2], int size2, double *v3[2], int size3);

/* lu.c*/
extern int G_ludcmp(double **, int, int *, double *);
extern void G_lubksb(double **a, int n, int *indx, double b[]);

/* findzc.c */
extern int G_math_findzc(double conv[], int size, double zc[], double thresh, int num_orients);


/* *************************************************************** */
/* ***** WRAPPER FOR CCMATH FUNCTIONS USED IN GRASS ************** */
/* *************************************************************** */
extern int G_math_solv(double **,double *,int);
extern int G_math_solvps(double **,double *,int);
extern void G_math_solvtd(double *,double *,double *,double *,int);
extern int G_math_solvru(double **,double *,int);
extern int G_math_minv(double **,int);
extern int G_math_psinv(double **,int);
extern int G_math_ruinv(double **,int);
extern void G_math_eigval(double **,double *,int);
extern void G_math_eigen(double **,double *,int);
extern double G_math_evmax(double **,double *,int);
extern int G_math_svdval(double *,double **,int,int);
extern int G_math_sv2val(double *,double **,int,int);
extern int G_math_svduv(double *,double **,double **, int,double **,int);
extern int G_math_sv2uv(double *,double **,double **,int,double **,int);
extern int G_math_svdu1v(double *,double **,int,double **,int);


/* *************************************************************** */
/* *************** LINEARE EQUATION SYSTEM PART ****************** */
/* *************************************************************** */

/*!
 * \brief The row vector of the sparse matrix
 * */
typedef struct
{
    double *values;		/*The non null values of the row */
    unsigned int cols;		/*Number of entries */
    unsigned int *index;	/*the index number */
} G_math_spvector;

/* Sparse matrix and sparse vector functions
 * */
extern G_math_spvector *G_math_alloc_spvector(int );
extern G_math_spvector **G_math_alloc_spmatrix(int );
extern void G_math_free_spmatrix(G_math_spvector ** , int );
extern void G_math_free_spvector(G_math_spvector * );
extern int G_math_add_spvector(G_math_spvector **, G_math_spvector * , int );
extern G_math_spvector **G_math_A_to_Asp(double **, int, double);
extern double **G_math_Asp_to_A(G_math_spvector **, int);
extern double **G_math_Asp_to_sband_matrix(G_math_spvector **, int, int);
extern G_math_spvector **G_math_sband_matrix_to_Asp(double **, int, int, double);
extern void G_math_print_spmatrix(G_math_spvector **, int);
extern void G_math_Ax_sparse(G_math_spvector **, double *, double *, int );

/*Symmetric band matrix handling */
extern double **G_math_matrix_to_sband_matrix(double **, int, int);
extern double **G_math_sband_matrix_to_matrix(double **, int, int);
extern void G_math_Ax_sband(double ** A, double *x, double *y, int rows, int bandwidth);

/*linear equation solver, most of them are multithreaded wih OpenMP*/
extern int G_math_solver_gauss(double **, double *, double *, int );
extern int G_math_solver_lu(double **, double *, double *, int );
extern int G_math_solver_cholesky(double **, double *, double *, int , int );
extern void G_math_solver_cholesky_sband(double **, double *, double *, int, int);
extern int G_math_solver_jacobi(double **, double *, double *, int , int , double , double );
extern int G_math_solver_gs(double **, double *, double *, int , int , double , double );

extern int G_math_solver_pcg(double **, double *, double *, int , int , double , int );
extern int G_math_solver_cg(double **, double *, double *, int , int , double );
extern int G_math_solver_cg_sband(double **, double *, double *, int, int, int, double);
extern int G_math_solver_bicgstab(double **, double *, double *, int , int , double );
extern int G_math_solver_sparse_jacobi(G_math_spvector **, double *, double *, int , int , double , double );
extern int G_math_solver_sparse_gs(G_math_spvector **, double *, double *, int , int , double , double );
extern int G_math_solver_sparse_pcg(G_math_spvector **, double *, double *, int , int , double , int );
extern int G_math_solver_sparse_cg(G_math_spvector **, double *, double *, int , int , double );
extern int G_math_solver_sparse_bicgstab(G_math_spvector **, double *, double *, int , int , double );

/* solver algoithms and helper functions*/
extern void G_math_gauss_elimination(double **, double *, int );
extern void G_math_lu_decomposition(double **, double *, int );
extern int G_math_cholesky_decomposition(double **, int , int );
extern void G_math_cholesky_sband_decomposition(double **, double **, int, int);
extern void G_math_backward_substitution(double **, double *, double *, int );
extern void G_math_forward_substitution(double **, double *, double *, int );
extern void G_math_cholesky_sband_substitution(double **, double *, double *, int, int);

/*BLAS like level 1,2 and 3 functions*/

/*level 1 vector - vector grass implementation with OpenMP thread support*/
extern void G_math_d_x_dot_y(double *, double *, double *, int );
extern void G_math_d_asum_norm(double *, double *, int );
extern void G_math_d_euclid_norm(double *, double *, int );
extern void G_math_d_max_norm(double *, double *, int );
extern void G_math_d_ax_by(double *, double *, double *, double , double , int );
extern void G_math_d_copy(double *, double *, int );

extern void G_math_f_x_dot_y(float *, float *, float *, int );
extern void G_math_f_asum_norm(float *, float *, int );
extern void G_math_f_euclid_norm(float *, float *, int );
extern void G_math_f_max_norm(float *, float *, int );
extern void G_math_f_ax_by(float *, float *, float *, float , float , int );
extern void G_math_f_copy(float *, float *, int );

extern void G_math_i_x_dot_y(int *, int *,  double *, int );
extern void G_math_i_asum_norm(int *,  double *, int );
extern void G_math_i_euclid_norm(int *,  double *,int );
extern void G_math_i_max_norm(int *,  int *, int );
extern void G_math_i_ax_by(int *, int *, int *, int , int , int );
extern void G_math_i_copy(int *, int *, int );

/*ATLAS blas level 1 wrapper*/
extern double G_math_ddot(double *, double *, int );
extern float G_math_sdot(float *, float *, int );
extern float G_math_sdsdot(float *, float *, float , int );
extern double G_math_dnrm2(double *, int );
extern double G_math_dasum(double *, int );
extern double G_math_idamax(double *, int );
extern float  G_math_snrm2(float *, int );
extern float  G_math_sasum(float *, int );
extern float  G_math_isamax(float *, int );
extern void G_math_dscal(double *, double , int );
extern void G_math_sscal(float *, float , int );
extern void G_math_dcopy(double *, double *, int );
extern void G_math_scopy(float *, float *, int );
extern void G_math_daxpy(double *, double *, double , int );
extern void G_math_saxpy(float *, float *, float , int );

/*level 2 matrix - vector grass implementation with OpenMP thread support*/
extern void G_math_d_Ax(double **, double *, double *, int , int );
extern void G_math_f_Ax(float **, float *, float *, int , int );
extern void G_math_d_x_dyad_y(double *, double *, double **, int, int );
extern void G_math_f_x_dyad_y(float *, float *, float **, int, int );
extern void G_math_d_aAx_by(double **, double *, double *, double , double , double *, int , int );
extern void G_math_f_aAx_by(float **, float *, float *, float , float , float *, int , int );
extern int G_math_d_A_T(double **A, int rows);
extern int G_math_f_A_T(float **A, int rows);

/*level 3 matrix - matrix grass implementation with OpenMP thread support*/
extern void G_math_d_aA_B(double **, double **, double , double **, int , int );
extern void G_math_f_aA_B(float **, float **, float , float **, int , int );
extern void G_math_d_AB(double **, double **, double **, int , int , int );
extern void G_math_f_AB(float **,  float **,  float **,  int , int , int );

#endif /* GMATH_H_ */

