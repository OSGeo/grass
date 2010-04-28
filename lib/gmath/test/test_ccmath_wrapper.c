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
static int test_ccmath_wrapper(void);

/* ************************************************************************* */
/* Performe the solver unit tests ****************************************** */
/* ************************************************************************* */
int unit_test_ccmath_wrapper(void)
{
	int sum = 0;

	G_message(_("\n++ Running ccmath wrapper unit tests ++"));

	sum += test_ccmath_wrapper();

	if (sum > 0)
		G_warning(_("\n-- ccmath wrapper unit tests failure --"));
	else
		G_message(_("\n-- ccmath wrapper unit tests finished successfully --"));

	return sum;
}

/* *************************************************************** */
/* Test all implemented ccmath wrapper  *** */
/* *************************************************************** */
int test_ccmath_wrapper(void)
{
	G_math_les *les;
	int sum = 0;
	double val = 0.0, val2 = 0.0;

	G_message("\t * testing ccmath lu solver with symmetric matrix\n");

	les = create_normal_symmetric_les(TEST_NUM_ROWS);

        G_math_d_copy(les->b, les->x, les->rows);
	G_math_solv(les->A, les->x, les->rows);
        G_math_print_les(les);
	G_math_d_asum_norm(les->x, &val, les->rows);
	if ((val - (double)les->rows) > EPSILON_ITER)
	{
		G_warning("Error in G_math_solv abs %2.20f != %i", val,
				les->rows);
		sum++;
	}

	G_math_free_les(les);

	G_message("\t * testing ccmath lu solver with unsymmetric matrix\n");

	les = create_normal_unsymmetric_les(TEST_NUM_ROWS);

        G_math_d_copy(les->b, les->x, les->rows);
	G_math_solvps(les->A, les->x, les->rows);
        G_math_print_les(les);
	G_math_d_asum_norm(les->x, &val, les->rows);
	if ((val - (double)les->rows) > EPSILON_ITER)
	{
		G_warning("Error in G_math_solv abs %2.20f != %i", val,
				les->rows);
		sum++;
	}

	G_math_free_les(les);

	G_message("\t * testing ccmath positive definite solver with symmetric matrix\n");

	les = create_normal_symmetric_les(TEST_NUM_ROWS);

        G_math_d_copy(les->b, les->x, les->rows);
	G_math_solvps(les->A, les->x, les->rows);
        G_math_print_les(les);
	G_math_d_asum_norm(les->x, &val, les->rows);
	if ((val - (double)les->rows) > EPSILON_ITER)
	{
		G_warning("Error in G_math_solvps abs %2.20f != %i", val,
				les->rows);
		sum++;
	}

	G_math_free_les(les);

	G_message("\t * testing ccmath matrix inversion with symmetric matrix\n");

	les = create_normal_symmetric_les(TEST_NUM_ROWS);

	G_math_minv(les->A, les->rows);
        G_math_d_Ax(les->A, les->b, les->x, les->rows, les->rows);
        G_math_print_les(les);
	G_math_d_asum_norm(les->x, &val, les->rows);
	if ((val - (double)les->rows) > EPSILON_ITER)
	{
		G_warning("Error in G_math_minv abs %2.20f != %i", val,
				les->rows);
		sum++;
	}

	G_math_free_les(les);

	G_message("\t * testing ccmath matrix inversion with unsymmetric matrix\n");

	les = create_normal_unsymmetric_les(TEST_NUM_ROWS);

	G_math_minv(les->A, les->rows);
        G_math_d_Ax(les->A, les->b, les->x, les->rows, les->rows);
        G_math_print_les(les);
	G_math_d_asum_norm(les->x, &val, les->rows);
	if ((val - (double)les->rows) > EPSILON_ITER)
	{
		G_warning("Error in G_math_minv abs %2.20f != %i", val,
				les->rows);
		sum++;
	}

	G_math_free_les(les);


	G_message("\t * testing ccmath positive definite matrix inversion with symmetric matrix\n");

	les = create_normal_symmetric_les(TEST_NUM_ROWS);

	G_math_psinv(les->A, les->rows);
        G_math_d_Ax(les->A, les->b, les->x, les->rows, les->rows);
        G_math_print_les(les);
	G_math_d_asum_norm(les->x, &val, les->rows);
	if ((val - (double)les->rows) > EPSILON_ITER)
	{
		G_warning("Error in G_math_psinv abs %2.20f != %i", val,
				les->rows);
		sum++;
	}

	G_math_free_les(les);


	G_message("\t * testing ccmath eigenvalue solver with symmetric matrix\n");

	les = create_normal_symmetric_les(TEST_NUM_ROWS);
        // Results of the eigenvalue computation with ocatve
        les->b[9] =   0.043264;
        les->b[8] =   0.049529;
        les->b[7] =   0.057406;
        les->b[6] =   0.067696;
        les->b[5] =   0.081639;
        les->b[4] =   0.101357;
        les->b[3] =   0.130298;
        les->b[2] =   0.174596;
        les->b[1] =   0.256157;
        les->b[0] =   0.502549;

	G_math_eigval(les->A, les->x, les->rows);
        G_math_print_les(les);
	G_math_d_asum_norm(les->b, &val, les->rows);
	G_math_d_asum_norm(les->x, &val2, les->rows);
	if ((val  - val2) > EPSILON_ITER)
	{
		G_warning("Error in G_math_eigv abs %2.20f != %f", val,
				val2);
		sum++;
	}

	G_math_free_les(les);


	G_message("\t * testing ccmath eigenvector computation with symmetric matrix\n");

	les = create_normal_symmetric_les(TEST_NUM_ROWS);
        // Results of the eigenvalue computation with ocatve
        les->b[9] =   0.043264;
        les->b[8] =   0.049529;
        les->b[7] =   0.057406;
        les->b[6] =   0.067696;
        les->b[5] =   0.081639;
        les->b[4] =   0.101357;
        les->b[3] =   0.130298;
        les->b[2] =   0.174596;
        les->b[1] =   0.256157;
        les->b[0] =   0.502549;
        
        G_math_eigen(les->A, les->x, les->rows);
        G_math_print_les(les);
	G_math_d_asum_norm(les->b, &val, les->rows);
	G_math_d_asum_norm(les->x, &val2, les->rows);
	if ((val  - val2) > EPSILON_ITER)
	{
		G_warning("Error in G_math_eigen abs %2.20f != %f", val,
				val2);
		sum++;
	}
	G_math_free_les(les);

	G_message("\t * testing ccmath singulare value decomposition with symmetric matrix\n");

	les = create_normal_symmetric_les(TEST_NUM_ROWS);

        G_math_svdval(les->x, les->A, les->rows, les->rows);
        G_math_print_les(les);


	G_math_free_les(les);

	return sum;
}

