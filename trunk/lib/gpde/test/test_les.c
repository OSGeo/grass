
/*****************************************************************************
*
* MODULE:       Grass PDE Numerical Library
* AUTHOR(S):    Soeren Gebbert, Berlin (GER) Dec 2006
* 		soerengebbert <at> gmx <dot> de
*               
* PURPOSE:      Unit tests for les creation
*
* COPYRIGHT:    (C) 2000 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*               License (>=v2). Read the file COPYING that comes with GRASS
*               for details.
*
*****************************************************************************/

#include <grass/gis.h>
#include <grass/gmath.h>
#include <grass/N_pde.h>
#include "test_gpde_lib.h"


/* prototypes */
static int test_les(void);

/* *************************************************************** */
/* Perfrome the les creation tests ******************************* */
/* *************************************************************** */
int unit_test_les_creation(void)
{
    int sum = 0;

    G_message("\n++ Running les creation unit tests ++");

    sum += test_les();

    if (sum > 0)
	G_warning("\n-- les creation unit tests failure --");
    else
	G_message("\n-- les creation unit tests finished successfully --");

    return sum;
}


/* *************************************************************** */
/* test the les creation of normal and sparse matirces *********** */
/* *************************************************************** */
int test_les(void)
{
    G_math_spvector *spvector = NULL;
    N_les *les = NULL;
    N_les *sples = NULL;
    int i, j;


    les = N_alloc_les(TEST_N_NUM_ROWS, N_NORMAL_LES);
    N_print_les(les);
    N_free_les(les);

    les = N_alloc_les_A(TEST_N_NUM_ROWS, N_NORMAL_LES);
    N_print_les(les);
    N_free_les(les);

    les = N_alloc_les_Ax(TEST_N_NUM_ROWS, N_NORMAL_LES);
    N_print_les(les);
    N_free_les(les);

    les = N_alloc_les_Ax_b(TEST_N_NUM_ROWS, N_NORMAL_LES);
    N_print_les(les);
    N_free_les(les);

    les = N_alloc_nquad_les(6, 3, N_NORMAL_LES);
    N_print_les(les);
    N_free_les(les);

    les = N_alloc_nquad_les_A(6, 3, N_NORMAL_LES);
    N_print_les(les);
    N_free_les(les);

    les = N_alloc_nquad_les_Ax(6, 3, N_NORMAL_LES);
    N_print_les(les);
    N_free_les(les);

    les = N_alloc_nquad_les_Ax_b(6, 3, N_NORMAL_LES);
    N_print_les(les);
    N_free_les(les);


    les = N_alloc_les(TEST_N_NUM_ROWS, N_NORMAL_LES);
    sples = N_alloc_les(TEST_N_NUM_ROWS, N_SPARSE_LES);


    G_message("\t * testing les creation in parallel\n");
#pragma omp parallel for private(i, j) shared(les)
    for (i = 0; i < TEST_N_NUM_ROWS; i++) {
	for (j = 0; j < TEST_N_NUM_ROWS; j++) {
	    if (i != j)
		les->A[i][j] = 2e-2;
	    les->A[i][i] = -1e2 - i;
	}
	les->x[i] = 273.15 + i;
	les->b[i] = 1e2 - i;
    }

#pragma omp parallel for private(i, j) shared(sples, spvector)
    for (i = 0; i < TEST_N_NUM_ROWS; i++) {
	spvector = G_math_alloc_spvector(TEST_N_NUM_ROWS);

	for (j = 0; j < TEST_N_NUM_ROWS; j++)
	    if (i != j)
		spvector->index[j] = 2e-2;

	spvector->index[0] = i;
	spvector->values[0] = -1e2 - i;

	G_math_add_spvector(sples->Asp, spvector, i);
	sples->x[i] = 273.15 + i;
	sples->b[i] = 1e2 - i;
    }

    N_free_les(les);
    N_free_les(sples);

    G_message("\t * testing les creation in serial\n");

    les = N_alloc_les(TEST_N_NUM_ROWS, N_NORMAL_LES);
    sples = N_alloc_les(TEST_N_NUM_ROWS, N_SPARSE_LES);

    for (i = 0; i < TEST_N_NUM_ROWS; i++) {
	for (j = 0; j < TEST_N_NUM_ROWS; j++) {
	    if (i != j)
		les->A[i][j] = 2e-2;
	    les->A[i][i] = -1e2 - i;
	}
	les->x[i] = 273.15 + i;
	les->b[i] = 1e2 - i;
    }

    for (i = 0; i < TEST_N_NUM_ROWS; i++) {
	spvector = G_math_alloc_spvector(TEST_N_NUM_ROWS);

	for (j = 0; j < TEST_N_NUM_ROWS; j++)
	    if (i != j)
		spvector->index[j] = 2e-2;

	spvector->index[0] = i;
	spvector->values[0] = -1e2 - i;

	G_math_add_spvector(sples->Asp, spvector, i);
	sples->x[i] = 273.15 + i;
	sples->b[i] = 1e2 - i;
    }

    N_free_les(les);
    N_free_les(sples);

    return 0;
}
