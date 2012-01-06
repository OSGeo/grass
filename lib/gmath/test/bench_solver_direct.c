/*****************************************************************************
 *
 * MODULE:       Grass PDE Numerical Library
 * AUTHOR(S):    Soeren Gebbert, Berlin (GER) Dec 2006
 * 		soerengebbert <at> gmx <dot> de
 *               
 * PURPOSE:      benchmarking the direct solvers
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
#include <sys/time.h>

/* prototypes */
static int bench_solvers(int rows);


/* ************************************************************************* */
/* Performe the solver unit tests ****************************************** */

/* ************************************************************************* */
int bench_solvers_direct(int rows) {
    G_message(_("\n++ Running direct solver benchmark ++"));

    bench_solvers(rows);

    return 1;
}


/* *************************************************************** */
/* Test all implemented solvers for sparse and normal matrix *** */

/* *************************************************************** */
int bench_solvers(int rows) {
    G_math_les *les;
    struct timeval tstart;
    struct timeval tend;

    G_message("\t * benchmarking gmath lu decomposition solver with unsymmetric matrix\n");

    les = create_normal_unsymmetric_les(rows);
    gettimeofday(&tstart, NULL);
    G_math_solver_lu(les->A, les->x, les->b, les->rows);
    gettimeofday(&tend, NULL);
    printf("Computation time gmath lu decomposition: %g\n", compute_time_difference(tstart, tend));
    G_math_free_les(les);

    G_message("\t * benchmarking lu ccmath decomposition solver with unsymmetric matrix\n");

    les = create_normal_unsymmetric_les(rows);
    gettimeofday(&tstart, NULL);
    G_math_solv(les->A, les->b, les->rows);
    gettimeofday(&tend, NULL);
    printf("Computation time ccmath lu decomposition: %g\n", compute_time_difference(tstart, tend));
    G_math_free_les(les);


    G_message("\t * benchmarking gauss elimination solver with unsymmetric matrix\n");

    les = create_normal_unsymmetric_les(rows);
    gettimeofday(&tstart, NULL);
    G_math_solver_gauss(les->A, les->x, les->b, les->rows);
    gettimeofday(&tend, NULL);
    printf("Computation time gauss elimination: %g\n", compute_time_difference(tstart, tend));
    G_math_free_les(les);

    G_message("\t * benchmarking gmath cholesky decomposition solver with symmetric matrix\n");

    les = create_normal_symmetric_les(rows);
    gettimeofday(&tstart, NULL);
    G_math_solver_cholesky(les->A, les->x, les->b, les->rows, les->rows);
    gettimeofday(&tend, NULL);
    printf("Computation time gmath cholesky decomposition: %g\n", compute_time_difference(tstart, tend));
    G_math_free_les(les);

    G_message("\t * benchmarking ccmath cholesky decomposition solver with symmetric matrix\n");

    les = create_normal_symmetric_les(rows);
    gettimeofday(&tstart, NULL);
    G_math_solvps(les->A, les->b, les->rows);
    gettimeofday(&tend, NULL);
    printf("Computation time ccmath cholesky decomposition: %g\n", compute_time_difference(tstart, tend));
    G_math_free_les(les);

    G_message("\t * benchmarking gmath cholesky band matrix decomposition solver with symmetric band matrix\n");

    les = create_symmetric_band_les(rows);
    gettimeofday(&tstart, NULL);
    G_math_solver_cholesky_sband(les->A, les->x, les->b, les->rows, les->rows);
    gettimeofday(&tend, NULL);
    printf("Computation time cholesky band matrix decomposition: %g\n", compute_time_difference(tstart, tend));
    G_math_free_les(les);


    return 1;
}

