
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
#include <grass/N_pde.h>
#include "test_gpde_lib.h"

/* prototypes */
static int test_solvers(void);
static N_les *create_normal_les(int rows);
static N_les *create_sparse_les(int rows);

/* ************************************************************************* */
/* Performe the solver unit tests ****************************************** */
/* ************************************************************************* */
int unit_test_solvers(void)
{
    int sum = 0;

    G_message(_("\n++ Running solver unit tests ++"));

    sum += test_solvers();

    if (sum > 0)
	G_warning(_("\n-- Solver unit tests failure --"));
    else
	G_message(_("\n-- Solver unit tests finished successfully --"));

    return sum;
}

/* *************************************************************** */
/* create a normal matrix with values ** Hilbert matrix ********** */
/* *************************************************************** */
N_les *create_normal_les(int rows)
{
    N_les *les;
    int i, j;
    int size = rows;
    double val;

    les = N_alloc_les(rows, N_NORMAL_LES);
    for (i = 0; i < size; i++) {
	val = 0.0;
	for (j = 0; j < size; j++) {
	    les->A[i][j] = (double)(1.0 / (((double)i + 1.0) +
					   ((double)j + 1.0) - 1.0));
	    val += les->A[i][j];
	}
	les->b[i] = val;
    }

    return les;
}

/* *************************************************************** */
/* create a sparse matrix with values ** Hilbert matrix ********** */
/* *************************************************************** */
N_les *create_sparse_les(int rows)
{
    N_les *les;
    N_spvector *spvector;
    int i, j;
    double val;

    les = N_alloc_les(rows, N_SPARSE_LES);

    for (i = 0; i < rows; i++) {
	spvector = N_alloc_spvector(rows);
	val = 0;

	for (j = 0; j < rows; j++) {
	    spvector->values[j] =
		(double)(1.0 / (((double)i + 1.0) + ((double)j + 1.0) - 1.0));
	    spvector->index[j] = j;
	    val += spvector->values[j];
	}

	N_add_spvector_to_les(les, spvector, i);
	les->b[i] = val;
    }


    return les;
}


/* *************************************************************** */
/* Test all implemented solvers for sparse and normal matrices *** */
/* *************************************************************** */
int test_solvers(void)
{
    N_les *les;
    N_les *sples;

    G_message("\t * testing jacobi solver\n");

    les = create_normal_les(TEST_N_NUM_ROWS);
    sples = create_sparse_les(TEST_N_NUM_ROWS);

    N_solver_jacobi(les, 100, 1, 0.1e-4);
    /*N_print_les(les); */
    N_solver_jacobi(sples, 100, 1, 0.1e-4);
    /*N_print_les(sples); */

    N_free_les(les);
    N_free_les(sples);


    G_message("\t * testing SOR solver\n");

    les = create_normal_les(TEST_N_NUM_ROWS);
    sples = create_sparse_les(TEST_N_NUM_ROWS);

    N_solver_SOR(les, 100, 1, 0.1e-4);
    /*N_print_les(les); */
    N_solver_SOR(sples, 100, 1, 0.1e-4);
    /*N_print_les(sples); */

    N_free_les(les);
    N_free_les(sples);

    G_message("\t * testing cg solver\n");

    les = create_normal_les(TEST_N_NUM_ROWS);
    sples = create_sparse_les(TEST_N_NUM_ROWS);

    N_solver_cg(les, 100, 0.1e-8);
    /*N_print_les(les); */
    N_solver_cg(sples, 100, 0.1e-8);
    /*N_print_les(sples); */

    N_free_les(les);
    N_free_les(sples);

    G_message("\t * testing pcg solver with N_DIAGONAL_PRECONDITION\n");

    les = create_normal_les(TEST_N_NUM_ROWS);
    sples = create_sparse_les(TEST_N_NUM_ROWS);

    N_solver_pcg(les, 100, 0.1e-8, N_DIAGONAL_PRECONDITION);
    N_print_les(les);
    N_solver_pcg(sples, 100, 0.1e-8, N_DIAGONAL_PRECONDITION);
    N_print_les(sples);

    N_free_les(les);
    N_free_les(sples);

    G_message
	("\t * testing pcg solver with N_ROWSCALE_EUKLIDNORM_PRECONDITION\n");

    les = create_normal_les(TEST_N_NUM_ROWS);
    sples = create_sparse_les(TEST_N_NUM_ROWS);

    N_solver_pcg(les, 100, 0.1e-8, N_ROWSCALE_EUKLIDNORM_PRECONDITION);
    N_print_les(les);
    N_solver_pcg(sples, 100, 0.1e-8, N_ROWSCALE_EUKLIDNORM_PRECONDITION);
    N_print_les(sples);

    N_free_les(les);
    N_free_les(sples);

    G_message
	("\t * testing pcg solver with N_ROWSCALE_ABSSUMNORM_PRECONDITION\n");

    les = create_normal_les(TEST_N_NUM_ROWS);
    sples = create_sparse_les(TEST_N_NUM_ROWS);

    N_solver_pcg(les, 100, 0.1e-8, N_ROWSCALE_ABSSUMNORM_PRECONDITION);
    N_print_les(les);
    N_solver_pcg(sples, 100, 0.1e-8, N_ROWSCALE_ABSSUMNORM_PRECONDITION);
    N_print_les(sples);

    N_free_les(les);
    N_free_les(sples);


    G_message("\t * testing bicgstab solver\n");

    les = create_normal_les(TEST_N_NUM_ROWS);
    sples = create_sparse_les(TEST_N_NUM_ROWS);

    N_solver_bicgstab(les, 100, 0.1e-8);
    /*N_print_les(les); */
    N_solver_bicgstab(sples, 100, 0.1e-8);
    /*N_print_les(sples); */

    N_free_les(les);
    N_free_les(sples);

    G_message("\t * testing gauss elimination solver\n");

    les = create_normal_les(TEST_N_NUM_ROWS);

     /*GAUSS*/ N_solver_gauss(les);
    N_print_les(les);

    N_free_les(les);

    G_message("\t * testing lu decomposition solver\n");

    les = create_normal_les(TEST_N_NUM_ROWS);

     /*LU*/ N_solver_lu(les);
    N_print_les(les);

    N_free_les(les);

    les = create_normal_les(TEST_N_NUM_ROWS);

    /*cholesky */ N_solver_cholesky(les);
    N_print_les(les);

    N_free_les(les);


    return 0;
}
