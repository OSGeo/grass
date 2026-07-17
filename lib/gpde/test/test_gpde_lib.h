/*****************************************************************************
 *
 * MODULE:       Grass PDE Numerical Library
 * AUTHOR(S):    Soeren Gebbert, Berlin (GER) Dec 2006
 *                 soerengebbert <at> gmx <dot> de
 *
 * PURPOSE:      Unit and Integration tests
 *
 * SPDX-FileCopyrightText: 2000 Other GRASS authors
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 *****************************************************************************/

#ifndef _N_TEST_GPDE_H_
#define _N_TEST_PDE_H_

#define TEST_N_NUM_ROWS   10
#define TEST_N_NUM_COLS   10
#define TEST_N_NUM_DEPTHS 10

/* Array test functions */
extern int unit_test_arrays(void);

/* matrix assembling */
extern int unit_test_assemble(void);

/* gradient creation and handling tests */
extern int unit_test_gradient(void);

/* test the meth tools of gpde */
extern int unit_test_tools(void);

/* geom_data struct tests */
extern int unit_test_geom_data(void);

/* les creation */
extern int unit_test_les_creation(void);

/*gwflow */
extern int integration_test_gwflow(void);

/* solute transport */
extern int integration_test_solute_transport(void);

#endif
