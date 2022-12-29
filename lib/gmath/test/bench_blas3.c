
/*****************************************************************************
*
* MODULE:       Grass PDE Numerical Library
* AUTHOR(S):    Soeren Gebbert, Berlin (GER) Dec 2007
* 		soerengebbert <at> gmx <dot> de
*               
* PURPOSE:      Unit benchs for les creation
*
* COPYRIGHT:    (C) 2007 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*               License (>=v2). Read the file COPYING that comes with GRASS
*               for details.
*
*****************************************************************************/

#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/gmath.h>
#include <math.h>
#include "test_gmath_lib.h"
#include <sys/time.h>

/* prototypes */
static void bench_blas_level_3_double(int rows);


/* *************************************************************** */
/* Perfrome the blas level 3 benchs ****************************** */
/* *************************************************************** */
int bench_blas_level_3(int rows)
{
    G_message(_("\n++ Running blas level 3 benchmark ++"));

    bench_blas_level_3_double(rows);

    return 1;
}

/* *************************************************************** */
/* ************** D O U B L E ************************************ */
/* *************************************************************** */
void bench_blas_level_3_double(int rows)
{
    struct timeval tstart;
    struct timeval tend;
    double **A, **B, **C, *x, *y;

    x = G_alloc_vector(rows);
    y = G_alloc_vector(rows);

    A = G_alloc_matrix(rows, rows);
    B = G_alloc_matrix(rows, rows);
    C = G_alloc_matrix(rows, rows);

    fill_d_vector_range_1(x, 1, rows);
    fill_d_vector_range_1(y, 1, rows);

    fill_d_vector_range_1(A[0], 1, rows*rows);
    fill_d_vector_range_1(B[0], 1, rows*rows);


    gettimeofday(&tstart, NULL);
#pragma omp parallel default(shared)
{
    G_math_d_aA_B(A, B, 4.0 , C, rows , rows);
}
    gettimeofday(&tend, NULL);
    printf("Computation time G_math_d_aA_B: %g\n", compute_time_difference(tstart, tend));
    gettimeofday(&tstart, NULL);
#pragma omp parallel default(shared)
{
    G_math_d_AB(A, B, C, rows , rows , rows);
}
    gettimeofday(&tend, NULL);
    printf("Computation time G_math_d_AB: %g\n", compute_time_difference(tstart, tend));


    if(x)
      G_free_vector(x);
    if(y)
      G_free_vector(y);

    if(A)
      G_free_matrix(A);
    if(B)
      G_free_matrix(B);
    if(C)
      G_free_matrix(C);

    return;
}
