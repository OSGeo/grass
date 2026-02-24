/****************************************************************************
 *
 * MODULE:       test.octree.lib
 *
 * AUTHOR(S):    Corey White
 *
 * PURPOSE:      Unit tests for the octree library
 *
 * COPYRIGHT:    (C) 2026 by the GRASS Development Team
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 *****************************************************************************/

#include <stdlib.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/glocale.h>

#include "test_octree.h"

/*- Parameters and global variables -----------------------------------------*/
typedef struct {
    struct Flag *testunit, *full;
} paramType;

paramType param;

/*- prototypes --------------------------------------------------------------*/
static void set_params(void);

/* ************************************************************************* */
/* Set up the arguments we are expecting ********************************** */
/* ************************************************************************* */
void set_params(void)
{
    param.testunit = G_define_flag();
    param.testunit->key = 'u';
    param.testunit->description = _("Run all unit tests");

    param.full = G_define_flag();
    param.full->key = 'a';
    param.full->description = _("Run all unit and integration tests");
}

/* ************************************************************************* */
/* ************************************************************************* */
/* ************************************************************************* */
int main(int argc, char *argv[])
{
    struct GModule *module;
    int returnstat = 0;

    /* Initialize GRASS */
    G_gisinit(argv[0]);

    module = G_define_module();
    module->description = _("Performs unit tests for the octree library");

    /* Get parameters from user */
    set_params();

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    /* Run all unit tests */
    if (param.testunit->answer || param.full->answer) {
        returnstat += unit_test_create_node();
        returnstat += unit_test_insert_point();
        returnstat += unit_test_free_octree();
    }

    if (returnstat != 0)
        G_warning(_("Errors detected while testing the octree lib"));
    else
        G_message(_("\n-- octree lib tests finished successfully --"));

    return returnstat;
}
