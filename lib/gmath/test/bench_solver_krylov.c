/*****************************************************************************
 *
 * MODULE:       Grass PDE Numerical Library
 * AUTHOR(S):    Soeren Gebbert, Berlin (GER) Dec 2006
 * 		soerengebbert <at> gmx <dot> de
 *               
 * PURPOSE:      benchmarking the krylov subspace solvers
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
int bench_solvers_krylov(int rows) {
    G_message(_("\n++ Running krylov solver benchmark ++"));

    bench_solvers(rows);

    return 1;
}


/* *************************************************************** */
/* Test all implemented solvers for sparse and normal matrix *** */

/* *************************************************************** */
int bench_solvers(int rows) {
    G_math_les *les;
    G_math_les *sples;
    struct timeval tstart;
    struct timeval tend;

    G_message("\t * benchmarking pcg solver with symmetric matrix and preconditioner 1\n");

    les = create_normal_symmetric_les(rows);
    sples = create_sparse_symmetric_les(rows);

    gettimeofday(&tstart, NULL);
    G_math_solver_pcg(les->A, les->x, les->b, les->rows, 250, 0.1e-9, 1);
    gettimeofday(&tend, NULL);
    printf("Computation time pcg normal matrix: %g\n", compute_time_difference(tstart, tend));

    gettimeofday(&tstart, NULL);
    G_math_solver_sparse_pcg(sples->Asp, sples->x, sples->b, les->rows, 250,
            0.1e-9, 1);
    gettimeofday(&tend, NULL);
    printf("Computation time pcg sparse matrix: %g\n", compute_time_difference(tstart, tend));

    G_math_free_les(les);
    G_math_free_les(sples);

    G_message("\t * benchmark cg solver with symmetric matrix\n");

    les = create_normal_symmetric_les(rows);
    sples = create_sparse_symmetric_les(rows);
    
    gettimeofday(&tstart, NULL);
    G_math_solver_cg(les->A, les->x, les->b, les->rows, 250, 0.1e-9);
    gettimeofday(&tend, NULL);
    printf("Computation time cg normal matrix: %g\n", compute_time_difference(tstart, tend));
    
    gettimeofday(&tstart, NULL);
    G_math_solver_sparse_cg(sples->Asp, sples->x, sples->b, les->rows, 250,
            0.1e-9);
    gettimeofday(&tend, NULL);
    printf("Computation time cg sparse matrix: %g\n", compute_time_difference(tstart, tend));
    
    G_math_free_les(les);
    G_math_free_les(sples);

    G_message("\t * benchmark cg solver with symmetric band matrix\n");

    les = create_symmetric_band_les(rows);
    
    gettimeofday(&tstart, NULL);
    G_math_solver_cg_sband(les->A, les->x, les->b, les->rows, les->rows, 250, 0.1e-9);
    gettimeofday(&tend, NULL);
    printf("Computation time cg symmetric band matrix: %g\n", compute_time_difference(tstart, tend));
    
    G_math_free_les(les);

    G_message("\t * benchmark bicgstab solver with unsymmetric matrix\n");

    les = create_normal_unsymmetric_les(rows);
    sples = create_sparse_unsymmetric_les(rows);
    
    gettimeofday(&tstart, NULL);
    G_math_solver_bicgstab(les->A, les->x, les->b, les->rows, 250, 0.1e-9);
    gettimeofday(&tend, NULL);
    printf("Computation time bicgstab normal matrix: %g\n", compute_time_difference(tstart, tend));
    
    gettimeofday(&tstart, NULL);
    G_math_solver_sparse_bicgstab(sples->Asp, sples->x, sples->b, les->rows,
            250, 0.1e-9);
    gettimeofday(&tend, NULL);
    printf("Computation time bicgstab sparse matrix: %g\n", compute_time_difference(tstart, tend));

    G_math_free_les(les);
    G_math_free_les(sples);

    return 1;
}

