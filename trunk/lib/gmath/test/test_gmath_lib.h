
/*****************************************************************************
*
* MODULE:       Grass gmath Library
* AUTHOR(S):    Soeren Gebbert, Berlin (GER) Oct 2007
* 		soerengebbert <at> gmx <dot> de
*               
* PURPOSE:	Unit and Integration tests
*
* COPYRIGHT:    (C) 2000 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*               License (>=v2). Read the file COPYING that comes with GRASS
*               for details.
*
*****************************************************************************/

#ifndef _TEST_GMATH_LIB_H_
#define _TEST_GMATH_LIB_H_

#include <grass/gmath.h>
#include <grass/gis.h>

#define TEST_NUM_ROWS 10  
#define TEST_NUM_COLS 9
#define TEST_NUM_DEPTHS 8

#define G_MATH_NORMAL_LES 0
#define G_MATH_SPARSE_LES 1

struct timeval;

typedef struct
{
    double *x;			/*the value vector */
    double *b;			/*the right side of Ax = b */
    double **A;			/*the normal quadratic matrix */
    double *data;		/*the pointer to the quadratic matrix data*/
    G_math_spvector **Asp;		/*the sparse matrix */
    int rows;			/*number of rows */
    int cols;			/*number of cols */
    int quad;			/*is the matrix quadratic (1-quadratic, 0 not)*/
    int type;			/*the type of the les, normal == 0, sparse == 1 */
    int bandwidth;		/*the bandwidth of the matrix (0 < bandwidth <= cols)*/
    int symm;			/*0 if matrix unsymmetric, 1 if symmetric*/
} G_math_les;

typedef struct
{
    float *x;			/*the value vector */
    float *b;			/*the right side of Ax = b */
    float **A;			/*the normal quadratic matrix */
    float *data;		/*the pointer to the quadratic matrix data*/
    int rows;			/*number of rows */
    int cols;			/*number of cols */
    int quad;			/*is the matrix quadratic (1-quadratic, 0 not)*/
    int type;			/*the type of the les, normal == 0, sparse == 1 */
    int bandwidth;		/*the bandwidth of the matrix (0 < bandwidth <= cols)*/
    int symm;			/*0 if matrix unsymmetric, 1 if symmetric*/
} G_math_f_les;

extern G_math_les *G_math_alloc_nquad_les(int cols, int rows, int type);
extern G_math_les *G_math_alloc_nquad_les_Ax(int cols, int rows, int type);
extern G_math_les *G_math_alloc_nquad_les_A(int cols, int rows, int type);
extern G_math_les *G_math_alloc_nquad_les_Ax_b(int cols, int rows, int type);
extern G_math_les *G_math_alloc_les(int rows, int type);
extern G_math_les *G_math_alloc_les_Ax(int rows, int type);
extern G_math_les *G_math_alloc_les_A(int rows, int type);
extern G_math_les *G_math_alloc_les_Ax_b(int rows, int type);
extern G_math_les *G_math_alloc_les_param(int cols, int rows, int type, int parts);
extern int G_math_add_spvector_to_les(G_math_les * les, G_math_spvector * spvector, int row);
extern void G_math_print_les(G_math_les * les);
extern void G_math_free_les(G_math_les * les);


extern void fill_d_vector_range_1(double *x, double a, int rows);
extern void fill_f_vector_range_1(float *x, float a, int rows);
extern void fill_i_vector_range_1(int *x, int a, int rows);
extern void fill_d_vector_range_2(double *x, double a, int rows);
extern void fill_f_vector_range_2(float *x, float a, int rows);
extern void fill_i_vector_range_2(int *x, int a, int rows);
extern void fill_d_vector_scalar(double *x, double a, int rows);
extern void fill_f_vector_scalar(float *x, float a, int rows);
extern void fill_i_vector_scalar(int *x, int a, int rows);

extern G_math_les *create_normal_symmetric_les(int rows);
extern G_math_les *create_symmetric_band_les(int rows);
extern G_math_les *create_normal_symmetric_pivot_les(int rows);
extern G_math_les *create_normal_unsymmetric_les(int rows);
extern G_math_les *create_sparse_symmetric_les(int rows);
extern G_math_les *create_sparse_unsymmetric_les(int rows);
extern G_math_les *create_normal_unsymmetric_nquad_les_A(int rows, int cols);


/*float*/
extern G_math_f_les *G_math_alloc_f_les(int rows, int type);
extern G_math_f_les *G_math_alloc_f_nquad_les_A(int rows, int cols, int type);
extern G_math_f_les *G_math_alloc_f_les_param(int cols, int rows, int type, int parts);
extern void G_math_free_f_les(G_math_f_les * les);
extern G_math_f_les *create_normal_symmetric_f_les(int rows);
extern G_math_f_les *create_normal_unsymmetric_f_les(int rows);
extern G_math_f_les *create_normal_unsymmetric_f_nquad_les_A(int rows, int cols);

/* direct and iterative solvers */
extern int unit_test_solvers(void);

/* Test the matrix conversion dense -> band ->sparse and vis versa */
extern int unit_test_matrix_conversion(void);

/* ccmath wrapper tests*/
int unit_test_ccmath_wrapper(void);

/* blas level 1 routines */
extern int unit_test_blas_level_1(void);

/* blas level 2 routines */
extern int unit_test_blas_level_2(void);

/* blas level 3 routines */
extern int unit_test_blas_level_3(void);

/* benchmarking iterative krylov solvers */
extern int bench_solvers_krylov(int);

/* benchmarking direct solvers */
extern int bench_solvers_direct(int);

/* benchmarking level 2 blas functions */
int bench_blas_level_2(int rows); 

/* benchmarking level 3 blas functions */
int bench_blas_level_3(int rows); 

/* Compute time difference */
extern double compute_time_difference(struct timeval start, struct timeval end);

#endif
