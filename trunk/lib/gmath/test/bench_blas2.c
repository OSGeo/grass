
/*****************************************************************************
*
* MODULE:       Grass PDE Numerical Library
* AUTHOR(S):    Soeren Gebbert, Berlin (GER) Dec 2006
* 		soerengebbert <at> gmx <dot> de
*               
* PURPOSE:      Unit benchs for les creation
*
* COPYRIGHT:    (C) 2000 by the GRASS Development Team
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
#define EPSILON 0.0000001

/* prototypes */
static void bench_blas_level_2_double(int rows);


/* *************************************************************** */
/* Perfrome the blas level 2 unit benchs ************************** */
/* *************************************************************** */
int bench_blas_level_2(int rows)
{
    G_message(_("\n++ Running blas level 2 benchmark ++"));

    bench_blas_level_2_double(rows);

    return 1;
}

/* *************************************************************** */
/* ************** D O U B L E ************************************ */
/* *************************************************************** */
void bench_blas_level_2_double(int rows)
{

    double **A, *x, *y, *z;
    struct timeval tstart;
    struct timeval tend;

    G_math_les *les;
    les = create_normal_unsymmetric_les(rows);
    G_math_les *bles;
    bles = create_symmetric_band_les(rows);
    G_math_les *sples;
    sples = create_sparse_unsymmetric_les(rows);

    x = G_alloc_vector(rows);
    y = G_alloc_vector(rows);
    z = G_alloc_vector(rows);

    A = G_alloc_matrix(rows, rows);

    fill_d_vector_range_1(x, 1, rows);

    gettimeofday(&tstart, NULL);

#pragma omp parallel default(shared)
{
    G_math_Ax_sparse(sples->Asp, x, z, rows);
}
    gettimeofday(&tend, NULL);
    printf("Computation time G_math_Ax_sparse: %g\n", compute_time_difference(tstart, tend));
    gettimeofday(&tstart, NULL);
#pragma omp parallel default(shared)
{
    G_math_Ax_sband(bles->A, x, z, rows, rows);
}
    gettimeofday(&tend, NULL);
    printf("Computation time G_math_Ax_sband: %g\n", compute_time_difference(tstart, tend));
    gettimeofday(&tstart, NULL);
#pragma omp parallel default(shared)
{
    G_math_d_Ax(les->A, x, z, rows, rows);
}
    gettimeofday(&tend, NULL);
    printf("Computation time G_math_d_Ax: %g\n", compute_time_difference(tstart, tend));
    gettimeofday(&tstart, NULL);
#pragma omp parallel default(shared)
{
    G_math_d_aAx_by(les->A, x, y, 3.0, 4.0, z, rows, rows);
}
    gettimeofday(&tend, NULL);
    printf("Computation time G_math_d_Ax_by: %g\n", compute_time_difference(tstart, tend));
    gettimeofday(&tstart, NULL);
#pragma omp parallel default(shared)
{
    G_math_d_x_dyad_y(x, x, A, rows, rows);
}
    gettimeofday(&tend, NULL);
    printf("Computation time G_math_d_x_dyad: %g\n", compute_time_difference(tstart, tend));
    gettimeofday(&tstart, NULL);


    if(x)
      G_free_vector(x);
    if(y)
      G_free_vector(y);
    if(z)
    G_free_vector(z);

    G_math_free_les(les);
    G_math_free_les(bles);
    G_math_free_les(sples);

    if(A)
      G_free_matrix(A);

    return;
}

