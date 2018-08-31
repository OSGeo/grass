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
static int test_solvers(void);

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
/* Test all implemented solvers for sparse and normal matrix *** */
/* *************************************************************** */
int test_solvers(void)
{
	G_math_les *les;
	G_math_les *sples;
	int sum = 0;
	double val = 0.0;

	G_message("\t * testing jacobi solver with symmetric matrix\n");

	les = create_normal_symmetric_les(TEST_NUM_ROWS);
	sples = create_sparse_symmetric_les(TEST_NUM_ROWS);

	G_math_solver_jacobi(les->A, les->x, les->b, les->rows, 250, 1, 0.1e-10);
	G_math_d_asum_norm(les->x, &val, les->rows);
	if ((val - (double)les->rows) > EPSILON_ITER)
	{
		G_warning("Error in G_math_solver_jacobi abs %2.20f != %i", val,
				les->rows);
		sum++;
	}
	G_math_solver_sparse_jacobi(sples->Asp, sples->x, sples->b, les->rows, 250,
			1, 0.1e-10);
	G_math_d_asum_norm(sples->x, &val, sples->rows);
	if ((val - (double)sples->rows) > EPSILON_ITER)
	{
		G_warning("Error in G_math_solver_sparse_jacobi abs %2.20f != %i", val,
				sples->rows);
		sum++;
	}

	G_math_free_les(les);
	G_math_free_les(sples);

	G_message("\t * testing jacobi solver with unsymmetric matrix\n");

	les = create_normal_unsymmetric_les(TEST_NUM_ROWS);
	sples = create_sparse_unsymmetric_les(TEST_NUM_ROWS);

	G_math_solver_jacobi(les->A, les->x, les->b, les->rows, 250, 1, 0.1e-10);
	G_math_d_asum_norm(les->x, &val, les->rows);
	if ((val - (double)les->rows) > EPSILON_ITER)
	{
		G_warning("Error in G_math_solver_jacobi abs %2.20f != %i", val,
				les->rows);
		sum++;
	}

	G_math_solver_sparse_jacobi(sples->Asp, sples->x, sples->b, les->rows, 250,
			1, 0.1e-10);
	G_math_d_asum_norm(sples->x, &val, sples->rows);
	if ((val - (double)sples->rows) > EPSILON_ITER)
	{
		G_warning("Error in G_math_solver_sparse_jacobi abs %2.20f != %i", val,
				sples->rows);
		sum++;
	}

	G_math_free_les(les);
	G_math_free_les(sples);

	G_message("\t * testing gauss seidel solver with symmetric matrix\n");

	les = create_normal_symmetric_les(TEST_NUM_ROWS);
	sples = create_sparse_symmetric_les(TEST_NUM_ROWS);

	G_math_solver_gs(les->A, les->x, les->b, les->rows, 150, 1, 0.1e-9);
	G_math_d_asum_norm(les->x, &val, les->rows);
	if ((val - (double)les->rows) > EPSILON_ITER)
	{
		G_warning("Error in G_math_solver_gs abs %2.20f != %i", val, les->rows);
		sum++;
	}

	G_math_solver_sparse_gs(sples->Asp, sples->x, sples->b, les->rows, 150, 1,
			0.1e-9);
	G_math_d_asum_norm(sples->x, &val, sples->rows);
	if ((val - (double)sples->rows) > EPSILON_ITER)
	{
		G_warning("Error in G_math_solver_sparse_gs abs %2.20f != %i", val,
				sples->rows);
		sum++;
	}

	G_math_free_les(les);
	G_math_free_les(sples);

	G_message("\t * testing gauss seidel solver with unsymmetric matrix\n");

	les = create_normal_unsymmetric_les(TEST_NUM_ROWS);
	sples = create_sparse_unsymmetric_les(TEST_NUM_ROWS);

	G_math_solver_gs(les->A, les->x, les->b, les->rows, 150, 1, 0.1e-9);
	G_math_d_asum_norm(les->x, &val, les->rows);
	if ((val - (double)les->rows) > EPSILON_ITER)
	{
		G_warning("Error in G_math_solver_gs abs %2.20f != %i", val, les->rows);
		sum++;
	}

	G_math_solver_sparse_gs(sples->Asp, sples->x, sples->b, les->rows, 150, 1,
			0.1e-9);
	G_math_d_asum_norm(sples->x, &val, sples->rows);
	if ((val - (double)sples->rows) > EPSILON_ITER)
	{
		G_warning("Error in G_math_solver_sparse_gs abs %2.20f != %i", val,
				sples->rows);
		sum++;
	}

	G_math_free_les(les);
	G_math_free_les(sples);

	G_message("\t * testing pcg solver with symmetric bad conditioned matrix and preconditioner 3\n");

	les = create_normal_symmetric_pivot_les(TEST_NUM_ROWS);

	G_math_solver_pcg(les->A, les->x, les->b, les->rows, 250, 0.1e-9, 3);
	G_math_d_asum_norm(les->x, &val, les->rows);
	if ((val - (double)les->rows) > EPSILON_ITER)
	{
		G_warning("Error in G_math_solver_pcg abs %2.20f != %i", val, les->rows);
		sum++;
	}
	G_math_print_les(les);
	G_math_free_les(les);

	G_message("\t * testing pcg solver with symmetric matrix and preconditioner 1\n");

	les = create_normal_symmetric_les(TEST_NUM_ROWS);
	sples = create_sparse_symmetric_les(TEST_NUM_ROWS);

	G_math_solver_pcg(les->A, les->x, les->b, les->rows, 250, 0.1e-9, 1);
	G_math_d_asum_norm(les->x, &val, les->rows);
	if ((val - (double)les->rows) > EPSILON_ITER)
	{
		G_warning("Error in G_math_solver_pcg abs %2.20f != %i", val, les->rows);
		sum++;
	}
	G_math_print_les(les);

	G_math_solver_sparse_pcg(sples->Asp, sples->x, sples->b, les->rows, 250,
			0.1e-9, 1);
	G_math_d_asum_norm(sples->x, &val, sples->rows);
	if ((val - (double)sples->rows) > EPSILON_ITER)
	{
		G_warning("Error in G_math_solver_sparse_pcg abs %2.20f != %i", val,
				sples->rows);
		sum++;
	}
	G_math_print_les(sples);

	G_math_free_les(les);
	G_math_free_les(sples);

	G_message("\t * testing pcg solver with symmetric matrix and preconditioner 2\n");

	les = create_normal_symmetric_les(TEST_NUM_ROWS);
	sples = create_sparse_symmetric_les(TEST_NUM_ROWS);

	G_math_solver_pcg(les->A, les->x, les->b, les->rows, 250, 0.1e-9, 2);
	G_math_d_asum_norm(les->x, &val, les->rows);
	if ((val - (double)les->rows) > EPSILON_ITER)
	{
		G_warning("Error in G_math_solver_pcg abs %2.20f != %i", val, les->rows);
		sum++;
	}
	G_math_print_les(les);

	G_math_solver_sparse_pcg(sples->Asp, sples->x, sples->b, les->rows, 250,
			0.1e-9, 2);
	G_math_d_asum_norm(sples->x, &val, sples->rows);
	if ((val - (double)sples->rows) > EPSILON_ITER)
	{
		G_warning("Error in G_math_solver_sparse_pcg abs %2.20f != %i", val,
				sples->rows);
		sum++;
	}
	G_math_print_les(sples);

	G_math_free_les(les);
	G_math_free_les(sples);

	G_message("\t * testing pcg solver with symmetric matrix and preconditioner 3\n");

	les = create_normal_symmetric_les(TEST_NUM_ROWS);
	sples = create_sparse_symmetric_les(TEST_NUM_ROWS);

	G_math_solver_pcg(les->A, les->x, les->b, les->rows, 250, 0.1e-9, 3);
	G_math_d_asum_norm(les->x, &val, les->rows);
	if ((val - (double)les->rows) > EPSILON_ITER)
	{
		G_warning("Error in G_math_solver_pcg abs %2.20f != %i", val, les->rows);
		sum++;
	}
	G_math_print_les(les);

	G_math_solver_sparse_pcg(sples->Asp, sples->x, sples->b, les->rows, 250,
			0.1e-9, 3);
	G_math_d_asum_norm(sples->x, &val, sples->rows);
	if ((val - (double)sples->rows) > EPSILON_ITER)
	{
		G_warning("Error in G_math_solver_sparse_pcg abs %2.20f != %i", val,
				sples->rows);
		sum++;
	}
	G_math_print_les(sples);

	G_math_free_les(les);
	G_math_free_les(sples);

	G_message("\t * testing cg solver with symmetric matrix\n");

	les = create_normal_symmetric_les(TEST_NUM_ROWS);
	sples = create_sparse_symmetric_les(TEST_NUM_ROWS);

	G_math_solver_cg(les->A, les->x, les->b, les->rows, 250, 0.1e-9);
	G_math_d_asum_norm(les->x, &val, les->rows);
	if ((val - (double)les->rows) > EPSILON_ITER)
	{
		G_warning("Error in G_math_solver_cg abs %2.20f != %i", val, les->rows);
		sum++;
	}
	G_math_print_les(les);

	G_math_solver_sparse_cg(sples->Asp, sples->x, sples->b, les->rows, 250,
			0.1e-9);
	G_math_d_asum_norm(sples->x, &val, sples->rows);
	if ((val - (double)sples->rows) > EPSILON_ITER)
	{
		G_warning("Error in G_math_solver_sparse_cg abs %2.20f != %i", val,
				sples->rows);
		sum++;
	}
	G_math_print_les(sples);

	G_math_free_les(les);
	G_math_free_les(sples);


	G_message("\t * testing cg solver with symmetric bad conditioned matrix\n");

	les = create_normal_symmetric_pivot_les(TEST_NUM_ROWS);

	G_math_solver_cg(les->A, les->x, les->b, les->rows, 250, 0.1e-9);
	G_math_d_asum_norm(les->x, &val, les->rows);
	if ((val - (double)les->rows) > EPSILON_ITER)
	{
		G_warning("Error in G_math_solver_cg abs %2.20f != %i", val, les->rows);
		sum++;
	}
	G_math_print_les(les);
	G_math_free_les(les);

	G_message("\t * testing bicgstab solver with symmetric matrix\n");

	les = create_normal_symmetric_les(TEST_NUM_ROWS);
	sples = create_sparse_symmetric_les(TEST_NUM_ROWS);

	G_math_solver_bicgstab(les->A, les->x, les->b, les->rows, 250, 0.1e-9);
	G_math_d_asum_norm(les->x, &val, les->rows);
	if ((val - (double)les->rows) > EPSILON_ITER)
	{
		G_warning("Error in G_math_solver_bicgstab abs %2.20f != %i", val,
				les->rows);
		sum++;
	}
	G_math_print_les(les);

	G_math_solver_sparse_bicgstab(sples->Asp, sples->x, sples->b, les->rows,
			250, 0.1e-9);
	G_math_d_asum_norm(sples->x, &val, sples->rows);
	if ((val - (double)sples->rows) > EPSILON_ITER)
	{
		G_warning("Error in G_math_solver_sparse_bicgstab abs %2.20f != %i",
				val, sples->rows);
		sum++;
	}
	G_math_print_les(sples);

	G_math_free_les(les);
	G_math_free_les(sples);

	G_message("\t * testing bicgstab solver with unsymmetric matrix\n");

	les = create_normal_unsymmetric_les(TEST_NUM_ROWS);
	sples = create_sparse_unsymmetric_les(TEST_NUM_ROWS);

	G_math_solver_bicgstab(les->A, les->x, les->b, les->rows, 250, 0.1e-9);
	G_math_d_asum_norm(les->x, &val, les->rows);
	if ((val - (double)les->rows) > EPSILON_ITER)
	{
		G_warning("Error in G_math_solver_bicgstab abs %2.20f != %i", val,
				les->rows);
		sum++;
	}
	G_math_print_les(les);

	G_math_solver_sparse_bicgstab(sples->Asp, sples->x, sples->b, les->rows,
			250, 0.1e-9);
	G_math_d_asum_norm(sples->x, &val, sples->rows);
	if ((val - (double)les->rows) > EPSILON_ITER)
	{
		G_warning("Error in G_math_solver_sparse_bicgstab abs %2.20f != %i",
				val, sples->rows);
		sum++;
	}

	G_math_print_les(sples);
	G_math_free_les(les);
	G_math_free_les(sples);

	G_message("\t * testing gauss elimination solver with symmetric matrix\n");
	les = create_normal_symmetric_les(TEST_NUM_ROWS);
	G_math_solver_gauss(les->A, les->x, les->b, les->rows);
	G_math_d_asum_norm(les->x, &val, les->rows);
	if ((val - (double)les->rows) > EPSILON_DIRECT)
	{
		G_warning("Error in G_math_solver_gauss abs %2.20f != %i", val,
				les->rows);
		sum++;
	}
	G_math_print_les(les);
	G_math_free_les(les);

	G_message("\t * testing lu decomposition solver with symmetric matrix\n");
	les = create_normal_symmetric_les(TEST_NUM_ROWS);
	G_math_solver_lu(les->A, les->x, les->b, les->rows);
	G_math_d_asum_norm(les->x, &val, les->rows);
	if ((val - (double)les->rows) > EPSILON_DIRECT)
	{
		G_warning("Error in G_math_solver_gauss abs %2.20f != %i", val,
				les->rows);
		sum++;
	}
	G_math_print_les(les);
	G_math_free_les(les);

	G_message("\t * testing gauss elimination solver with unsymmetric matrix\n");
	les = create_normal_unsymmetric_les(TEST_NUM_ROWS);
	G_math_solver_gauss(les->A, les->x, les->b, les->rows);
	G_math_d_asum_norm(les->x, &val, les->rows);
	if ((val - (double)les->rows) > EPSILON_DIRECT)
	{
		G_warning("Error in G_math_solver_gauss abs %2.20f != %i", val,
				les->rows);
		sum++;
	}
	G_math_print_les(les);
	G_math_free_les(les);

	G_message("\t * testing lu decomposition solver with unsymmetric matrix\n");
	les = create_normal_unsymmetric_les(TEST_NUM_ROWS);
	G_math_solver_lu(les->A, les->x, les->b, les->rows);

	G_math_d_asum_norm(les->x, &val, les->rows);
	if ((val - (double)les->rows) > EPSILON_DIRECT)
	{
		G_warning("Error in G_math_solver_gauss abs %2.20f != %i", val,
				les->rows);
		sum++;
	}
	G_math_print_les(les);
	G_math_free_les(les);

	G_message("\t * testing gauss elimination solver with symmetric bad conditioned matrix\n");
	les = create_normal_symmetric_pivot_les(TEST_NUM_ROWS);
	G_math_solver_gauss(les->A, les->x, les->b, les->rows);
	G_math_d_asum_norm(les->x, &val, les->rows);
	if ((val - (double)les->rows) > EPSILON_DIRECT)
	{
		G_warning("Error in G_math_solver_gauss abs %2.20f != %i", val,
				les->rows);
		sum++;
	}
	G_math_print_les(les);
	G_math_free_les(les);

	G_message("\t * testing lu decomposition solver with symmetric bad conditioned matrix\n");
	les = create_normal_symmetric_pivot_les(TEST_NUM_ROWS);
	G_math_solver_lu(les->A, les->x, les->b, les->rows);
	G_math_d_asum_norm(les->x, &val, les->rows);
	if ((val - (double)les->rows) > EPSILON_DIRECT)
	{
		G_warning("Error in G_math_solver_lu abs %2.20f != %i", val,
				les->rows);
		sum++;
	}
	G_math_print_les(les);
	G_math_free_les(les);

	G_message("\t * testing cholesky decomposition solver with symmetric matrix\n");
	les = create_normal_symmetric_les(TEST_NUM_ROWS);
	/*cholesky*/G_math_solver_cholesky(les->A, les->x, les->b, les->rows,
			les->rows);
	G_math_d_asum_norm(les->x, &val, les->rows);
	if ((val - (double)les->rows) > EPSILON_DIRECT)
	{
		G_warning("Error in G_math_solver_solver_cholesky abs %2.20f != %i", val,
				les->rows);
		sum++;
	}
	G_math_print_les(les);
	G_math_free_les(les);

	G_message("\t * testing cholesky band decomposition solver with symmetric band matrix 1\n");
	les = create_normal_symmetric_les(TEST_NUM_ROWS);
	G_math_print_les(les);
	/* Create a band matrix*/
	G_message("\t * Creating symmetric band matrix\n");
	les->A = G_math_matrix_to_sband_matrix(les->A, les->rows, les->rows);
	G_math_print_les(les);

	/*cholesky*/G_math_solver_cholesky_sband(les->A, les->x, les->b, les->rows,les->rows);
	G_math_d_asum_norm(les->x, &val, les->rows);
	if ((val - (double)les->rows) > EPSILON_DIRECT)
	{
		G_warning("Error in G_math_solver_solver_cholesky_band abs %2.20f != %i", val,
				les->rows);
		sum++;
	}
	G_math_print_les(les);
	G_math_free_les(les);

	G_message("\t * testing cholesky band decomposition solver with symmetric band matrix 2\n");
        les = create_symmetric_band_les(TEST_NUM_ROWS);
	G_math_print_les(les);

	/*cholesky*/G_math_solver_cholesky_sband(les->A, les->x, les->b, les->rows,les->rows);
	G_math_d_asum_norm(les->x, &val, les->rows);
	if ((val - (double)les->rows) > EPSILON_DIRECT)
	{
		G_warning("Error in G_math_solver_solver_cholesky_band abs %2.20f != %i", val,
				les->rows);
		sum++;
	}
	G_math_print_les(les);
	G_math_free_les(les);

	G_message("\t * testing cg solver with symmetric band matrix\n");

	les = create_symmetric_band_les(TEST_NUM_ROWS);

	G_math_solver_cg_sband(les->A, les->x, les->b, les->rows, les->rows, 250, 0.1e-9);
	G_math_d_asum_norm(les->x, &val, les->rows);
	if ((val - (double)les->rows) > EPSILON_ITER)
	{
		G_warning("Error in G_math_solver_cg_sband abs %2.20f != %i", val, les->rows);
		sum++;
	}
	G_math_print_les(les);
	G_math_free_les(les);

	return sum;
}

