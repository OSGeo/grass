/*****************************************************************************
 *
 * MODULE:       Grass PDE Numerical Library
 * AUTHOR(S):    Soeren Gebbert, Berlin (GER) Dec 2006
 *                 soerengebbert <at> gmx <dot> de
 *
 * PURPOSE:      heatflow tests
 *
 * SPDX-FileCopyrightText: 2000 Other GRASS authors
 * SPDX-License-Identifier: GPL-2.0-or-later
 *****************************************************************************/

#include <grass/gis.h>
#include <grass/N_pde.h>
// #include <grass/N_heatflow.h>
#include "test_gpde_lib.h"

static int integration_test_heatflow(void);

/* *************************************************************** */
/* Perform the heat flow integration tests ********************** */
/* *************************************************************** */
int integration_test_heatflow(void)
{
    int sum = 0;

    G_message("\n++ Running heat flow integration tests ++");

    G_message("\t 1. testing 2d heat flow");

    G_message("\t 2. testing 3d heat flow");

    if (sum > 0)
        G_warning("\n-- heat flow integration tests failure --");
    else
        G_message("\n-- heat flow integration tests finished successfully --");

    return sum;
}

